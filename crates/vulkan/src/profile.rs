use crate::device::DeviceState;
use anyhow::Result;
use ash::vk;
use std::cell::RefCell;
use std::sync::Arc;
use std::time::Instant;

#[derive(Debug)]
pub struct Event {
    pub name: &'static str,
    pub value: f64,
    pub elapsed_us: u64,
}

thread_local! {
    static CAPTURE: RefCell<Option<(Instant, Vec<Event>)>> = const { RefCell::new(None) };
}

pub fn active() -> bool {
    CAPTURE.with(|capture| capture.borrow().is_some())
}

pub fn capture<T>(run: impl FnOnce() -> T) -> (T, Vec<Event>) {
    capture_from(Instant::now(), run)
}

pub fn capture_start() -> Option<Instant> {
    CAPTURE.with(|capture| capture.borrow().as_ref().map(|(start, _)| *start))
}

pub fn capture_from<T>(start: Instant, run: impl FnOnce() -> T) -> (T, Vec<Event>) {
    CAPTURE.with(|capture| {
        assert!(capture.borrow().is_none(), "nested frame profile");
        *capture.borrow_mut() = Some((start, Vec::new()));
    });
    struct Reset;
    impl Drop for Reset {
        fn drop(&mut self) {
            CAPTURE.with(|capture| capture.borrow_mut().take());
        }
    }
    let _reset = Reset;
    let result = run();
    let (_, events) = CAPTURE.with(|capture| capture.borrow_mut().take().unwrap());
    (result, events)
}

pub fn append(events: Vec<Event>) {
    CAPTURE.with(|capture| {
        if let Some((_, existing)) = capture.borrow_mut().as_mut() {
            existing.extend(events);
        }
    });
}

pub fn record(name: &'static str, value: f64) {
    CAPTURE.with(|capture| {
        if let Some((start, events)) = capture.borrow_mut().as_mut() {
            events.push(Event {
                name,
                value,
                elapsed_us: start.elapsed().as_micros() as u64,
            });
        }
    });
}

pub fn span<T>(name: &'static str, run: impl FnOnce() -> T) -> T {
    if !active() {
        return run();
    }
    let start = Instant::now();
    let result = run();
    record(name, start.elapsed().as_secs_f64() * 1e6);
    result
}

pub(crate) struct Timestamps {
    device: Arc<DeviceState>,
    pool: vk::QueryPool,
    passes: Vec<&'static str>,
}

impl Timestamps {
    const CAPACITY: u32 = 32;

    pub(crate) fn new(device: &Arc<DeviceState>) -> Result<Option<Self>> {
        if device.timestamp_bits == 0 {
            return Ok(None);
        }
        let info = vk::QueryPoolCreateInfo::default()
            .query_type(vk::QueryType::TIMESTAMP)
            .query_count(Self::CAPACITY);
        Ok(Some(Self {
            pool: unsafe { device.device.create_query_pool(&info, None)? },
            device: device.clone(),
            passes: Vec::new(),
        }))
    }

    pub(crate) fn reset(&mut self, command: vk::CommandBuffer) {
        self.passes.clear();
        unsafe {
            self.device
                .device
                .cmd_reset_query_pool(command, self.pool, 0, Self::CAPACITY);
        }
    }

    pub(crate) fn begin(&mut self, command: vk::CommandBuffer, name: &'static str) -> Option<u32> {
        let index = self.passes.len() as u32 * 2;
        if index == Self::CAPACITY {
            record("timestamp_overflow", 1.0);
            return None;
        }
        self.passes.push(name);
        unsafe {
            self.device.device.cmd_write_timestamp(
                command,
                vk::PipelineStageFlags::TOP_OF_PIPE,
                self.pool,
                index,
            );
        }
        Some(index + 1)
    }

    pub(crate) fn end(&self, command: vk::CommandBuffer, end: u32) {
        unsafe {
            self.device.device.cmd_write_timestamp(
                command,
                vk::PipelineStageFlags::BOTTOM_OF_PIPE,
                self.pool,
                end,
            );
        }
    }

    pub(crate) fn collect(&self) -> Result<()> {
        if self.passes.is_empty() {
            return Ok(());
        }
        let mut ticks = [0u64; Self::CAPACITY as usize];
        unsafe {
            self.device.device.get_query_pool_results(
                self.pool,
                0,
                &mut ticks[..self.passes.len() * 2],
                vk::QueryResultFlags::TYPE_64,
            )?;
        }
        let mask = u64::MAX >> (64 - self.device.timestamp_bits);
        for (&name, [start, end]) in self.passes.iter().zip(ticks.as_chunks::<2>().0) {
            let elapsed = end.wrapping_sub(*start) & mask;
            record(
                name,
                elapsed as f64 * self.device.timestamp_period as f64 / 1000.0,
            );
        }
        Ok(())
    }
}

impl Drop for Timestamps {
    fn drop(&mut self) {
        unsafe { self.device.device.destroy_query_pool(self.pool, None) };
    }
}
