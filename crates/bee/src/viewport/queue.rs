use super::{render_image, Extent, RenderImage, TestImage};
use bee_renderer::{FrameSlot, PreparedFlow, ViewportRenderer};
use bee_scene::RenderSnapshot;
use bee_vulkan::{profile, Backend};
use gpui::BackgroundExecutor;
use std::sync::{Arc, Mutex};
use std::time::Duration;

const CAPACITY: usize = 2;
const SAMPLE_INTERVAL: Duration = Duration::from_millis(16);

#[derive(Clone)]
pub(super) struct Request {
    pub snapshot: RenderSnapshot,
    pub flow: PreparedFlow,
    pub extent: Extent,
    pub generation: u64,
    pub history_epoch: u64,
    pub accumulate: bool,
    #[cfg(test)]
    pub delay: Option<Duration>,
}

pub(super) struct Completed {
    pub request: Request,
    pub sequence: u64,
    pub samples: u32,
    pub image: Result<Arc<RenderImage>, String>,
}

struct State {
    revision: u64,
    input: Option<Request>,
    free: Vec<FrameSlot>,
    output: Option<Completed>,
    newest_output: u64,
    closing: bool,
}

pub(super) struct RenderQueue {
    state: Arc<Mutex<State>>,
    wake: async_channel::Sender<()>,
}

impl RenderQueue {
    pub fn new(
        backend: Arc<Backend>,
        executor: &BackgroundExecutor,
    ) -> (Self, async_channel::Receiver<()>) {
        let state = Arc::new(Mutex::new(State {
            revision: 0,
            input: None,
            free: (0..CAPACITY).map(|_| FrameSlot::default()).collect(),
            output: None,
            newest_output: 0,
            closing: false,
        }));
        let (wake, requests) = async_channel::bounded(1);
        let (ready, completions) = async_channel::bounded(1);
        executor
            .spawn(run(
                backend,
                executor.clone(),
                state.clone(),
                wake.clone(),
                requests,
                ready,
            ))
            .detach();
        (Self { state, wake }, completions)
    }

    pub fn replace(&self, input: Option<Request>) {
        let mut state = self.state.lock().unwrap();
        if !state.closing {
            state.revision += 1;
            state.input = input;
            let _ = self.wake.try_send(());
        }
    }

    pub fn take(&self) -> Option<Completed> {
        self.state.lock().unwrap().output.take()
    }

    #[cfg(test)]
    pub fn running(&self) -> bool {
        let state = self.state.lock().unwrap();
        state.free.len() < CAPACITY || state.input.is_some()
    }

    #[cfg(test)]
    pub fn in_flight(&self) -> usize {
        CAPACITY - self.state.lock().unwrap().free.len()
    }

    pub fn close(&self) {
        let mut state = self.state.lock().unwrap();
        state.closing = true;
        state.input = None;
        state.output = None;
        let _ = self.wake.try_send(());
    }
}

impl Drop for RenderQueue {
    fn drop(&mut self) {
        self.close();
    }
}

async fn run(
    backend: Arc<Backend>,
    executor: BackgroundExecutor,
    state: Arc<Mutex<State>>,
    wake: async_channel::Sender<()>,
    requests: async_channel::Receiver<()>,
    ready: async_channel::Sender<()>,
) {
    let mut renderer = ViewportRenderer::default();
    let mut sequence = 0;
    loop {
        let work = {
            let mut state = state.lock().unwrap();
            if state.closing && state.free.len() == CAPACITY {
                state.free.clear();
                break;
            }
            if !state.free.is_empty() {
                state.input.take().map(|input| {
                    let slot = state.free.pop().unwrap();
                    (input, slot, state.revision)
                })
            } else {
                None
            }
        };
        let Some((request, mut slot, revision)) = work else {
            if requests.recv().await.is_err() {
                break;
            }
            continue;
        };
        sequence += 1;
        let pending = renderer.submit(
            &backend,
            &request.flow,
            &request.snapshot,
            request.extent,
            request.history_epoch,
            1,
            &mut slot,
        );

        let state = state.clone();
        let wake = wake.clone();
        let ready = ready.clone();
        let timer = executor.clone();
        executor
            .spawn(async move {
                let (samples, slot, image) = match pending {
                    Ok(pending) => {
                        // 一个槽等待完成或处理像素时，提交端仍可使用另一个槽。
                        #[cfg(test)]
                        if let Some(delay) = request.delay {
                            timer.timer(delay).await;
                        }
                        let samples = pending.samples;
                        let (width, height) = request.extent;
                        let (slot, pixels) = pending.finish();
                        let image = pixels.and_then(|bgra| {
                            profile::span("pack_us", || {
                                render_image(TestImage {
                                    width,
                                    height,
                                    row_stride: width as usize * 4,
                                    bgra,
                                })
                            })
                        });
                        (samples, slot, image)
                    }
                    Err(error) => (0, slot, Err(error)),
                };
                let next_sample = (request.accumulate && image.is_ok()).then(|| request.clone());
                {
                    let mut state = state.lock().unwrap();
                    state.free.push(if image.is_ok() {
                        slot
                    } else {
                        FrameSlot::default()
                    });
                    let _ = wake.try_send(());
                    if !state.closing && sequence > state.newest_output {
                        state.newest_output = sequence;
                        state.output = Some(Completed {
                            request,
                            sequence,
                            samples,
                            image,
                        });
                        let _ = ready.try_send(());
                    }
                }

                if let Some(request) = next_sample {
                    timer.timer(SAMPLE_INTERVAL).await;
                    let mut state = state.lock().unwrap();
                    if !state.closing && state.revision == revision && state.input.is_none() {
                        state.input = Some(request);
                        let _ = wake.try_send(());
                    }
                }
            })
            .detach();
    }
    // 关闭通知必须晚于最后一个 fence 和资源释放。
    drop(renderer);
    drop(backend);
    ready.close();
}
