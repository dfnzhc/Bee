use crate::{GraphResource, ResourceId, Usage, WorkGraph, WorkId, WorkPlan};
use bee_vulkan::profile;
use bee_vulkan::{
    Backend, Frame, RasterCommands, RasterPass, RecordedCommands, Submission, WorkerCommands,
};
use rayon::ThreadPool;
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::Mutex;

static NEXT_RESOURCES_ID: AtomicU64 = AtomicU64::new(1);

#[derive(Clone, Copy, PartialEq, Eq)]
enum Lifetime {
    Temporary,
    History,
    External,
}

struct Slot {
    lifetime: Lifetime,
    resource: Option<GraphResource>,
    usage: Option<Usage>,
}

enum InFlight {
    Serial(Submission),
    Parallel(Submission),
}

enum RecordedJob {
    Primary(RecordedCommands, Vec<profile::Event>),
    Raster(RasterCommands),
}

pub struct GraphResources {
    id: u64,
    revision: u64,
    slots: Vec<Slot>,
    pending: Option<InFlight>,
    frame: Option<Frame>,
    workers: Vec<Mutex<Option<WorkerCommands>>>,
    poisoned: bool,
}

impl Default for GraphResources {
    fn default() -> Self {
        Self {
            id: NEXT_RESOURCES_ID.fetch_add(1, Ordering::Relaxed),
            revision: 0,
            slots: Vec::new(),
            pending: None,
            frame: None,
            workers: Vec::new(),
            poisoned: false,
        }
    }
}

impl GraphResources {
    pub fn add_temporary(&mut self, resource: impl Into<GraphResource>) -> ResourceId {
        self.add(Lifetime::Temporary, resource.into(), None)
    }

    pub fn add_history(&mut self, resource: impl Into<GraphResource>) -> ResourceId {
        self.add(Lifetime::History, resource.into(), None)
    }

    /// # Safety
    /// 外部使用方必须完成访问并交出资源，usage 须匹配真实状态，资源须归图形队列族所有。
    pub unsafe fn add_external(
        &mut self,
        resource: impl Into<GraphResource>,
        usage: Option<Usage>,
    ) -> ResourceId {
        self.add(Lifetime::External, resource.into(), usage)
    }

    fn add(
        &mut self,
        lifetime: Lifetime,
        resource: GraphResource,
        usage: Option<Usage>,
    ) -> ResourceId {
        let id = ResourceId(self.slots.len());
        self.slots.push(Slot {
            lifetime,
            resource: Some(resource),
            usage,
        });
        self.revision = self.revision.wrapping_add(1);
        id
    }

    pub fn set_temporary(
        &mut self,
        id: ResourceId,
        resource: impl Into<GraphResource>,
    ) -> Result<(), String> {
        self.replace(id, resource.into(), Lifetime::Temporary)
    }

    pub fn replace_history(
        &mut self,
        id: ResourceId,
        resource: impl Into<GraphResource>,
    ) -> Result<(), String> {
        self.replace(id, resource.into(), Lifetime::History)
    }

    fn replace(
        &mut self,
        id: ResourceId,
        resource: GraphResource,
        lifetime: Lifetime,
    ) -> Result<(), String> {
        let slot = self.slots.get_mut(id.0).ok_or("unknown graph resource")?;
        if slot.lifetime != lifetime {
            return Err("resource has a different lifetime".into());
        }
        // 旧资源仍由在途帧持有；新资源从未定义内容开始。
        slot.resource = Some(resource);
        slot.usage = None;
        self.revision = self.revision.wrapping_add(1);
        Ok(())
    }

    /// # Safety
    /// 外部使用方必须完成访问并交还资源，usage 须匹配真实状态，资源须归图形队列族所有。
    pub unsafe fn acquire_external(
        &mut self,
        id: ResourceId,
        resource: impl Into<GraphResource>,
        usage: Option<Usage>,
    ) -> Result<(), String> {
        let slot = self.slots.get_mut(id.0).ok_or("unknown graph resource")?;
        if slot.lifetime != Lifetime::External || slot.resource.is_some() {
            return Err("external resource is not available for acquisition".into());
        }
        slot.resource = Some(resource.into());
        slot.usage = usage;
        self.revision = self.revision.wrapping_add(1);
        Ok(())
    }

    pub fn release_external(
        &mut self,
        id: ResourceId,
    ) -> Result<(GraphResource, Option<Usage>), String> {
        self.wait()?;
        let slot = self.slots.get_mut(id.0).ok_or("unknown graph resource")?;
        if slot.lifetime != Lifetime::External {
            return Err("resource is not external".into());
        }
        let resource = slot
            .resource
            .take()
            .ok_or("external resource was already released")?;
        self.revision = self.revision.wrapping_add(1);
        Ok((resource, slot.usage))
    }

    pub fn wait(&mut self) -> Result<(), String> {
        if self.poisoned {
            return Err("previous graph submission did not complete safely".into());
        }
        if let Some(submission) = self.pending.take() {
            let result = match submission {
                InFlight::Serial(submission) => {
                    submission.reclaim().map(|frame| self.frame = Some(frame))
                }
                InFlight::Parallel(submission) => submission.wait(),
            };
            if let Err(error) = result {
                self.poisoned = true;
                return Err(error.to_string());
            }
        }
        Ok(())
    }

    pub fn graph(&mut self) -> Result<WorkGraph, String> {
        // 当前使用单队列并等待上一提交，避免旧帧与新的历史资源竞争。
        self.wait()?;
        if self.slots.iter().any(|slot| slot.resource.is_none()) {
            return Err("a graph resource has not been supplied or acquired".into());
        }
        let mut graph = WorkGraph::default();
        for slot in &self.slots {
            graph.add_resource(slot.usage);
        }
        graph.binding = Some((self.id, self.revision));
        Ok(graph)
    }

    /// # Safety
    /// 回调只能访问声明的资源和用途，并须满足其 Vulkan 命令及其他外部对象的生命周期要求。
    pub unsafe fn submit(
        &mut self,
        backend: &Backend,
        graph: &WorkGraph,
        record: impl FnMut(WorkId, &mut Frame, &[GraphResource]) -> Result<(), String>,
    ) -> Result<(), String> {
        if self.poisoned
            || self.pending.is_some()
            || graph.binding != Some((self.id, self.revision))
            || graph.resources.len() != self.slots.len()
        {
            return Err("graph resources changed or a submission is still pending".into());
        }
        let plan = graph.compile()?;
        self.frame = None;
        self.submit_plan(backend, &plan, record)
    }

    /// # Safety
    /// 资源的初始用途和队列所有权须真实，回调须遵守计划声明及对象生命周期。
    pub unsafe fn submit_plan(
        &mut self,
        backend: &Backend,
        plan: &WorkPlan,
        mut record: impl FnMut(WorkId, &mut Frame, &[GraphResource]) -> Result<(), String>,
    ) -> Result<(), String> {
        let (resources, mut usages) = self.bindings()?;
        let mut frame = profile::span("begin_frame_us", || match self.frame.take() {
            Some(frame) => backend.reuse_frame(frame),
            None => backend.begin_frame(),
        })
        .map_err(|error| error.to_string())?;
        profile::span("record_us", || {
            plan.record_bound(&mut frame, &resources, &mut usages, |work, frame| {
                record(work, frame, &resources)
            })
        })?;
        let submission =
            profile::span("submit_us", || frame.submit()).map_err(|error| error.to_string())?;
        self.submitted(InFlight::Serial(submission), usages);
        Ok(())
    }

    /// # Safety
    /// 回调须遵守计划声明及对象生命周期，可在不同工作线程上同时调用。
    pub(crate) unsafe fn submit_parallel(
        &mut self,
        backend: &Backend,
        plan: &WorkPlan,
        pool: &ThreadPool,
        raster_pass: impl FnOnce(&[GraphResource]) -> Result<Option<(WorkId, RasterPass)>, String>,
        record: impl Fn(WorkId, &mut Frame, &[GraphResource]) -> Result<(), String> + Sync,
    ) -> Result<(), String> {
        let (resources, mut usages) = self.bindings()?;
        profile::span("begin_frame_us", || {
            self.workers
                .resize_with(pool.current_num_threads(), || Mutex::new(None));
            for worker in &mut self.workers {
                if let Some(commands) = worker.get_mut().unwrap() {
                    commands.reset().map_err(|error| error.to_string())?;
                }
            }
            Ok::<_, String>(())
        })?;
        let workers = &self.workers;
        let capture = profile::capture_start();
        let commands = profile::span("record_us", || {
            let barriers = plan.bind(&resources, &mut usages)?;
            let mut schedule = None;
            let raster = raster_pass(&resources)?.map(|(work, pass)| {
                let jobs = schedule.insert(plan.cpu.clone()).add_predecessors(work, 2);
                (work, pass, jobs)
            });
            schedule
                .as_ref()
                .unwrap_or(&plan.cpu)
                .run(pool, plan.order(), |work, finished| {
                    let mut worker = workers[pool.current_thread_index().unwrap()]
                        .lock()
                        .unwrap();
                    if worker.is_none() {
                        *worker = Some(
                            backend
                                .worker_commands()
                                .map_err(|error| error.to_string())?,
                        );
                    }
                    let worker = worker.as_mut().unwrap();
                    if let Some((_, pass, jobs)) = &raster {
                        if jobs.contains(&work.0) {
                            let middle = pass.draw_count().div_ceil(2);
                            let range = if work.0 == jobs.start {
                                0..middle
                            } else {
                                middle..pass.draw_count()
                            };
                            return pass
                                .record_batch(worker, range)
                                .map(RecordedJob::Raster)
                                .map_err(|error| error.to_string());
                        }
                    }
                    let mut encode = || {
                        worker.record(|frame| {
                            plan.record_one(work, frame, &resources, &barriers, |work, frame| {
                                if let Some((id, pass, jobs)) = &raster {
                                    if work == *id {
                                        let ranges = [jobs.start, jobs.start + 1].map(|id| {
                                            match finished[id].get().unwrap() {
                                                RecordedJob::Raster(commands) => commands,
                                                _ => unreachable!(),
                                            }
                                        });
                                        return frame
                                            .record_raster(pass, &ranges)
                                            .map_err(|error| error.to_string());
                                    }
                                }
                                record(work, frame, &resources)
                            })
                        })
                    };
                    match capture {
                        Some(start) => {
                            let (commands, events) = profile::capture_from(start, encode);
                            Ok(RecordedJob::Primary(commands?, events))
                        }
                        None => Ok(RecordedJob::Primary(encode()?, Vec::new())),
                    }
                })
        })?;
        let commands = commands
            .into_iter()
            .map(|job| {
                let RecordedJob::Primary(commands, events) = job else {
                    unreachable!()
                };
                profile::append(events);
                commands
            })
            .collect();
        let submission = profile::span("submit_us", || backend.submit_recorded(commands))
            .map_err(|error| error.to_string())?;
        self.submitted(InFlight::Parallel(submission), usages);
        Ok(())
    }

    fn bindings(&self) -> Result<(Vec<GraphResource>, Vec<Option<Usage>>), String> {
        if self.poisoned || self.pending.is_some() {
            return Err("graph resources are not ready for submission".into());
        }
        let resources = self
            .slots
            .iter()
            .map(|slot| slot.resource.clone().ok_or("graph resource is unavailable"))
            .collect::<Result<_, _>>()?;
        let usages = self.slots.iter().map(|slot| slot.usage).collect();
        Ok((resources, usages))
    }

    fn submitted(&mut self, submission: InFlight, usages: Vec<Option<Usage>>) {
        for (slot, usage) in self.slots.iter_mut().zip(usages) {
            slot.usage = usage;
            if slot.lifetime == Lifetime::Temporary {
                slot.resource = None;
                slot.usage = None;
            }
        }
        self.pending = Some(submission);
        self.revision = self.revision.wrapping_add(1);
    }

    /// # Safety
    /// 所有使用方须在同一队列按提交顺序交接，usage 须反映最后一次已提交的用途。
    pub(crate) unsafe fn bind_ordered(
        &mut self,
        id: ResourceId,
        resource: impl Into<GraphResource>,
        usage: Option<Usage>,
    ) {
        assert!(self.pending.is_none());
        let slot = &mut self.slots[id.0];
        assert!(slot.lifetime == Lifetime::History);
        slot.resource = Some(resource.into());
        slot.usage = usage;
        self.revision = self.revision.wrapping_add(1);
    }

    pub(crate) fn usage(&self, id: ResourceId) -> Option<Usage> {
        self.slots[id.0].usage
    }

    /// # Safety
    /// 新绑定须已由相同队列或宿主生产，producer 须与实际写入匹配。
    pub(crate) unsafe fn bind_input(
        &mut self,
        id: ResourceId,
        resource: impl Into<GraphResource>,
        producer: Usage,
    ) {
        let resource = resource.into();
        if self.slots[id.0]
            .resource
            .as_ref()
            .is_none_or(|old| !old.same_handle(&resource))
        {
            self.bind_ordered(id, resource, Some(producer));
        }
    }

    pub(crate) fn clear_bindings(&mut self) {
        assert!(self.pending.is_none());
        self.slots.clear();
        self.revision = self.revision.wrapping_add(1);
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{Access, Usage};
    use ash::vk;
    use bee_vulkan::{GraphImage, ValidationStatus};

    #[test]
    #[ignore = "requires a Vulkan graphics device and validation layer"]
    fn retains_replaces_and_hands_off_images() {
        if std::env::var_os("BEE_RESOURCES_GPU_CHILD").is_none() {
            let output = std::process::Command::new(std::env::current_exe().unwrap())
                .args([
                    "--exact",
                    "resources::tests::retains_replaces_and_hands_off_images",
                    "--ignored",
                    "--nocapture",
                ])
                .env("BEE_RESOURCES_GPU_CHILD", "1")
                .env("VK_LAYER_VALIDATE_SYNC", "1")
                .output()
                .unwrap();
            let stderr = String::from_utf8_lossy(&output.stderr);
            assert!(
                output.status.success(),
                "{}\n{stderr}",
                String::from_utf8_lossy(&output.stdout)
            );
            assert!(
                !stderr
                    .lines()
                    .any(|line| line.starts_with("Vulkan validation ") && line.contains("ERROR")),
                "{stderr}"
            );
            return;
        }

        let backend = Backend::new(true).unwrap();
        assert_eq!(backend.report.validation, ValidationStatus::Enabled);
        let new_image = || unsafe {
            backend
                .create_graph_image(
                    1,
                    1,
                    vk::Format::R8G8B8A8_UNORM,
                    vk::ImageUsageFlags::TRANSFER_SRC | vk::ImageUsageFlags::TRANSFER_DST,
                    vk::ImageAspectFlags::COLOR,
                )
                .unwrap()
        };
        let readback = backend.create_readback_buffer(4).unwrap();
        let mut resources = GraphResources::default();
        let history = resources.add_history(new_image());
        let temporary = resources.add_temporary(new_image());
        let external = unsafe { resources.add_external(new_image(), None) };
        let mut graph = resources.graph().unwrap();
        let initialize = graph
            .add_work(
                "initialize history",
                vec![Access {
                    resource: history,
                    usage: Usage::TransferWrite,
                }],
            )
            .unwrap();
        unsafe {
            resources
                .submit(&backend, &graph, |work, frame, images| {
                    assert_eq!(work, initialize);
                    clear(
                        frame,
                        images[history.index()].image()?,
                        [1.0, 0.0, 0.0, 1.0],
                    );
                    Ok(())
                })
                .unwrap();
        }
        // 临时图像每帧重新提供，历史图像的内容则保留。
        assert!(resources.graph().unwrap_err().contains("not been supplied"));
        resources.set_temporary(temporary, new_image()).unwrap();
        let mut graph = resources.graph().unwrap();
        let read = graph
            .add_work(
                "read history",
                vec![Access {
                    resource: history,
                    usage: Usage::TransferRead,
                }],
            )
            .unwrap();
        unsafe {
            resources
                .submit(&backend, &graph, |work, frame, images| {
                    assert_eq!(work, read);
                    frame
                        .copy_graph_image_to_readback(images[history.index()].image()?, &readback)
                        .map_err(|error| error.to_string())
                })
                .unwrap();
        }
        // 提交尚未回收时替换历史，旧图像仍须支撑此前的读回。
        resources.replace_history(history, new_image()).unwrap();
        resources.wait().unwrap();
        assert_eq!(unsafe { readback.read().unwrap() }, [255, 0, 0, 255]);

        resources.set_temporary(temporary, new_image()).unwrap();
        let mut graph = resources.graph().unwrap();
        graph
            .add_work(
                "read replaced history",
                vec![Access {
                    resource: history,
                    usage: Usage::TransferRead,
                }],
            )
            .unwrap();
        assert!(graph.compile().unwrap_err().contains("undefined"));

        let mut graph = resources.graph().unwrap();
        let write = graph
            .add_work(
                "write external",
                vec![Access {
                    resource: external,
                    usage: Usage::TransferWrite,
                }],
            )
            .unwrap();
        unsafe {
            resources
                .submit(&backend, &graph, |work, frame, images| {
                    assert_eq!(work, write);
                    clear(
                        frame,
                        images[external.index()].image()?,
                        [0.0, 1.0, 0.0, 1.0],
                    );
                    Ok(())
                })
                .unwrap();
        }
        let (external_image, state) = resources.release_external(external).unwrap();
        assert_eq!(state, Some(Usage::TransferWrite));
        assert!(resources.graph().is_err());
        unsafe {
            resources
                .acquire_external(external, external_image, state)
                .unwrap()
        };
        resources.set_temporary(temporary, new_image()).unwrap();
        let mut graph = resources.graph().unwrap();
        let read = graph
            .add_work(
                "read reacquired external",
                vec![Access {
                    resource: external,
                    usage: Usage::TransferRead,
                }],
            )
            .unwrap();
        unsafe {
            resources
                .submit(&backend, &graph, |work, frame, images| {
                    assert_eq!(work, read);
                    frame
                        .copy_graph_image_to_readback(images[external.index()].image()?, &readback)
                        .map_err(|error| error.to_string())
                })
                .unwrap();
        }
        let _ = resources.release_external(external).unwrap();
        assert_eq!(unsafe { readback.read().unwrap() }, [0, 255, 0, 255]);
    }

    unsafe fn clear(frame: &mut Frame, image: &GraphImage, color: [f32; 4]) {
        frame.record_raw(|device, command| {
            let range = vk::ImageSubresourceRange::default()
                .aspect_mask(vk::ImageAspectFlags::COLOR)
                .level_count(1)
                .layer_count(1);
            device.cmd_clear_color_image(
                command,
                image.raw(),
                vk::ImageLayout::TRANSFER_DST_OPTIMAL,
                &vk::ClearColorValue { float32: color },
                &[range],
            );
        });
    }
}
