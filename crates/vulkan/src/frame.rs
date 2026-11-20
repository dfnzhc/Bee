use crate::buffer::{Buffer, BufferState};
use crate::device::{Backend, DeviceState};
use crate::profile::{self, Timestamps};
use anyhow::{anyhow, bail, Context, Result};
use ash::vk;
use std::sync::Arc;

pub struct Frame {
    pub(crate) device: Arc<DeviceState>,
    pool: Arc<CommandPool>,
    pub(crate) command_buffer: vk::CommandBuffer,
    pub(crate) resources: Vec<Arc<dyn Send + Sync>>,
    timestamps: Option<Timestamps>,
}

struct CommandPool {
    device: Arc<DeviceState>,
    handle: vk::CommandPool,
}

impl Drop for CommandPool {
    fn drop(&mut self) {
        unsafe { self.device.device.destroy_command_pool(self.handle, None) };
    }
}

pub struct WorkerCommands {
    pool: Arc<CommandPool>,
    buffers: Vec<vk::CommandBuffer>,
    used: usize,
    secondary_buffers: Vec<vk::CommandBuffer>,
    secondary_used: usize,
}

pub struct RecordedCommands(Frame);

pub(crate) struct SecondaryCommands {
    _pool: Arc<CommandPool>,
    pub handle: vk::CommandBuffer,
}

pub struct Submission {
    frames: Vec<Frame>,
    fence: vk::Fence,
}

impl Backend {
    pub fn create_transfer_buffer(&self, size: u64) -> Result<Buffer> {
        if size == 0 || !size.is_multiple_of(4) {
            bail!("transfer buffer size must be a nonzero multiple of four");
        }
        self.create_graph_buffer(size, vk::BufferUsageFlags::TRANSFER_DST)
    }

    pub fn create_graph_buffer(&self, size: u64, usage: vk::BufferUsageFlags) -> Result<Buffer> {
        let supported = vk::BufferUsageFlags::TRANSFER_SRC
            | vk::BufferUsageFlags::TRANSFER_DST
            | vk::BufferUsageFlags::STORAGE_BUFFER;
        if size == 0 || usage.is_empty() || !(usage & !supported).is_empty() {
            bail!("invalid graph buffer size or usage");
        }
        Ok(Buffer {
            inner: Arc::new(BufferState::allocate(
                &self.device,
                size,
                usage,
                false,
                false,
            )?),
        })
    }

    pub fn begin_frame(&self) -> Result<Frame> {
        let pool = self.create_command_pool()?;
        let command_buffer = pool.allocate(vk::CommandBufferLevel::PRIMARY)?;
        Frame::begin(pool, command_buffer)
    }

    fn create_command_pool(&self) -> Result<Arc<CommandPool>> {
        let info = vk::CommandPoolCreateInfo::default()
            .queue_family_index(self.device.queue_family)
            .flags(vk::CommandPoolCreateFlags::TRANSIENT);
        let handle = unsafe { self.device.device.create_command_pool(&info, None) }
            .context("could not create Vulkan command pool")?;
        Ok(Arc::new(CommandPool {
            device: self.device.clone(),
            handle,
        }))
    }

    pub fn reuse_frame(&self, mut frame: Frame) -> Result<Frame> {
        if !Arc::ptr_eq(&self.device, &frame.device) {
            bail!("frame belongs to a different Vulkan device");
        }
        Arc::get_mut(&mut frame.pool)
            .context("command pool is still in use")?
            .reset()?;
        frame.begin_recording()?;
        if let Some(timestamps) = &mut frame.timestamps {
            timestamps.reset(frame.command_buffer);
        }
        Ok(frame)
    }

    pub fn worker_commands(&self) -> Result<WorkerCommands> {
        Ok(WorkerCommands {
            pool: self.create_command_pool()?,
            buffers: Vec::new(),
            used: 0,
            secondary_buffers: Vec::new(),
            secondary_used: 0,
        })
    }

    pub fn submit_recorded(&self, commands: Vec<RecordedCommands>) -> Result<Submission> {
        if commands
            .iter()
            .any(|commands| !Arc::ptr_eq(&self.device, &commands.0.device))
        {
            bail!("recorded commands belong to a different Vulkan device");
        }
        Submission::queue(commands.into_iter().map(|commands| commands.0).collect())
    }
}

impl CommandPool {
    fn allocate(&self, level: vk::CommandBufferLevel) -> Result<vk::CommandBuffer> {
        let allocate = vk::CommandBufferAllocateInfo::default()
            .command_pool(self.handle)
            .level(level)
            .command_buffer_count(1);
        let buffers = unsafe { self.device.device.allocate_command_buffers(&allocate) }
            .context("could not allocate Vulkan command buffer")?;
        Ok(buffers[0])
    }

    fn reset(&mut self) -> Result<()> {
        unsafe {
            self.device
                .device
                .reset_command_pool(self.handle, vk::CommandPoolResetFlags::empty())
        }
        .context("could not reset Vulkan command pool")
    }
}

impl WorkerCommands {
    pub fn reset(&mut self) -> Result<()> {
        if self.used != 0 || self.secondary_used != 0 {
            // 录制结果和在途提交都持有引用，最后一个引用回收后才可重置。
            Arc::get_mut(&mut self.pool)
                .context("worker command pool is still in use")?
                .reset()?;
            self.used = 0;
            self.secondary_used = 0;
        }
        Ok(())
    }

    pub fn record(
        &mut self,
        run: impl FnOnce(&mut Frame) -> Result<(), String>,
    ) -> Result<RecordedCommands, String> {
        if self.used == self.buffers.len() {
            self.buffers.push(
                self.pool
                    .allocate(vk::CommandBufferLevel::PRIMARY)
                    .map_err(|error| error.to_string())?,
            );
        }
        let command_buffer = self.buffers[self.used];
        self.used += 1;
        let mut frame =
            Frame::begin(self.pool.clone(), command_buffer).map_err(|error| error.to_string())?;
        run(&mut frame)?;
        frame
            .finish_recording()
            .map_err(|error| error.to_string())?;
        Ok(RecordedCommands(frame))
    }

    pub(crate) fn record_secondary(
        &mut self,
        device: &Arc<DeviceState>,
        inheritance: &vk::CommandBufferInheritanceInfo<'_>,
        run: impl FnOnce(vk::CommandBuffer),
    ) -> Result<Arc<SecondaryCommands>> {
        if !Arc::ptr_eq(device, &self.pool.device) {
            bail!("secondary commands belong to a different Vulkan device");
        }
        if self.secondary_used == self.secondary_buffers.len() {
            self.secondary_buffers
                .push(self.pool.allocate(vk::CommandBufferLevel::SECONDARY)?);
        }
        let handle = self.secondary_buffers[self.secondary_used];
        self.secondary_used += 1;
        let begin = vk::CommandBufferBeginInfo::default()
            .flags(
                vk::CommandBufferUsageFlags::ONE_TIME_SUBMIT
                    | vk::CommandBufferUsageFlags::RENDER_PASS_CONTINUE,
            )
            .inheritance_info(inheritance);
        unsafe { device.device.begin_command_buffer(handle, &begin)? };
        run(handle);
        unsafe { device.device.end_command_buffer(handle)? };
        Ok(Arc::new(SecondaryCommands {
            _pool: self.pool.clone(),
            handle,
        }))
    }
}

impl Frame {
    fn begin(pool: Arc<CommandPool>, command_buffer: vk::CommandBuffer) -> Result<Self> {
        let frame = Self {
            device: pool.device.clone(),
            pool,
            command_buffer,
            resources: Vec::new(),
            timestamps: None,
        };
        frame.begin_recording()?;
        Ok(frame)
    }

    fn begin_recording(&self) -> Result<()> {
        let info = vk::CommandBufferBeginInfo::default()
            .flags(vk::CommandBufferUsageFlags::ONE_TIME_SUBMIT);
        unsafe {
            self.device
                .device
                .begin_command_buffer(self.command_buffer, &info)
        }
        .context("could not begin Vulkan command buffer")
    }

    fn finish_recording(&self) -> Result<()> {
        unsafe { self.device.device.end_command_buffer(self.command_buffer) }
            .context("could not finish Vulkan command buffer")
    }

    pub fn profile_begin(&mut self, name: &'static str) -> Option<u32> {
        if !profile::active() || self.device.timestamp_bits == 0 {
            return None;
        }
        if self.timestamps.is_none() {
            match Timestamps::new(&self.device) {
                Ok(Some(mut timestamps)) => {
                    timestamps.reset(self.command_buffer);
                    self.timestamps = Some(timestamps);
                }
                Ok(None) => return None,
                Err(error) => {
                    eprintln!("Vulkan timestamps unavailable: {error}");
                    return None;
                }
            }
        }
        self.timestamps.as_mut()?.begin(self.command_buffer, name)
    }

    pub fn profile_end(&self, end: Option<u32>) {
        if let (Some(timestamps), Some(end)) = (&self.timestamps, end) {
            timestamps.end(self.command_buffer, end);
        }
    }

    pub fn fill_buffer(&mut self, buffer: &Buffer, value: u32) -> Result<()> {
        if !Arc::ptr_eq(&self.device, &buffer.inner.device) {
            bail!("buffer belongs to a different Vulkan device");
        }
        if !buffer.usage().contains(vk::BufferUsageFlags::TRANSFER_DST)
            || !buffer.inner.size.is_multiple_of(4)
        {
            bail!("buffer does not support transfer filling");
        }
        unsafe {
            self.device.device.cmd_fill_buffer(
                self.command_buffer,
                buffer.inner.buffer,
                0,
                buffer.inner.size,
                value,
            );

            // 使填充结果对本帧后续的传输命令可见。
            let barrier = vk::MemoryBarrier::default()
                .src_access_mask(vk::AccessFlags::TRANSFER_WRITE)
                .dst_access_mask(vk::AccessFlags::TRANSFER_READ | vk::AccessFlags::TRANSFER_WRITE);
            self.device.device.cmd_pipeline_barrier(
                self.command_buffer,
                vk::PipelineStageFlags::TRANSFER,
                vk::PipelineStageFlags::TRANSFER,
                vk::DependencyFlags::empty(),
                &[barrier],
                &[],
                &[],
            );
        }

        // 即使调用方释放原始句柄，也要保留缓冲区。
        self.resources.push(buffer.inner.clone());
        Ok(())
    }

    /// # Safety
    /// 调用方必须保证屏障描述真实的缓冲区访问，并对跨队列使用进行同步。
    pub unsafe fn graph_buffer_barrier(
        &mut self,
        buffer: &Buffer,
        before: (vk::PipelineStageFlags, vk::AccessFlags),
        after: (vk::PipelineStageFlags, vk::AccessFlags),
    ) -> Result<()> {
        self.graph_buffer_barriers(std::slice::from_ref(buffer), before, after)
    }

    /// # Safety
    /// 调用方须保证屏障阶段和访问类型匹配这些缓冲区的真实用途。
    pub unsafe fn graph_buffer_barriers(
        &mut self,
        buffers: &[Buffer],
        before: (vk::PipelineStageFlags, vk::AccessFlags),
        after: (vk::PipelineStageFlags, vk::AccessFlags),
    ) -> Result<()> {
        if buffers.is_empty() {
            return Ok(());
        }
        if buffers
            .iter()
            .any(|buffer| !Arc::ptr_eq(&self.device, &buffer.inner.device))
        {
            bail!("graph buffer belongs to a different Vulkan device");
        }
        let barriers: Vec<_> = buffers
            .iter()
            .map(|buffer| {
                vk::BufferMemoryBarrier::default()
                    .src_access_mask(before.1)
                    .dst_access_mask(after.1)
                    .src_queue_family_index(vk::QUEUE_FAMILY_IGNORED)
                    .dst_queue_family_index(vk::QUEUE_FAMILY_IGNORED)
                    .buffer(buffer.raw())
                    .offset(0)
                    .size(vk::WHOLE_SIZE)
            })
            .collect();
        self.device.device.cmd_pipeline_barrier(
            self.command_buffer,
            before.0,
            after.0,
            vk::DependencyFlags::empty(),
            &[],
            &barriers,
            &[],
        );
        self.resources.extend(
            buffers
                .iter()
                .map(|buffer| buffer.inner.clone() as Arc<dyn Send + Sync>),
        );
        Ok(())
    }

    pub fn submit(self) -> Result<Submission> {
        self.finish_recording()?;
        Submission::queue(vec![self])
    }
}

impl Submission {
    fn queue(frames: Vec<Frame>) -> Result<Self> {
        let state = &frames
            .first()
            .context("no Vulkan commands to submit")?
            .device;
        let device = &state.device;
        let fence = unsafe { device.create_fence(&vk::FenceCreateInfo::default(), None) }
            .context("could not create Vulkan submission fence")?;
        let command_buffers: Vec<_> = frames.iter().map(|frame| frame.command_buffer).collect();
        let submissions = [vk::SubmitInfo::default().command_buffers(&command_buffers)];

        // Vulkan 要求在外部同步对队列的提交。
        let result = match state.queue_lock.lock() {
            Ok(_lock) => unsafe { device.queue_submit(state.queue, &submissions, fence) }
                .context("could not submit Vulkan commands"),
            Err(_) => Err(anyhow!("Vulkan queue lock was poisoned")),
        };
        if let Err(error) = result {
            unsafe { device.destroy_fence(fence, None) };
            return Err(error);
        }
        Ok(Submission { frames, fence })
    }

    pub fn is_complete(&self) -> Result<bool> {
        let frame = self.frames.first().expect("submission is already finished");
        unsafe { frame.device.device.get_fence_status(self.fence) }
            .context("could not query Vulkan submission fence")
    }

    pub fn wait(mut self) -> Result<()> {
        self.finish()
    }

    pub fn reclaim(mut self) -> Result<Frame> {
        self.finish()?;
        if self.frames.len() != 1 {
            bail!("cannot reclaim a multi-buffer submission as one frame");
        }
        Ok(self
            .frames
            .pop()
            .expect("completed submission retains its frame"))
    }

    fn finish(&mut self) -> Result<()> {
        let Some(frame) = self.frames.first() else {
            return Ok(());
        };
        let result = profile::span("fence_wait_us", || unsafe {
            frame
                .device
                .device
                .wait_for_fences(&[self.fence], true, u64::MAX)
        });
        if result.is_ok() {
            for frame in &self.frames {
                if let Some(Err(error)) = frame.timestamps.as_ref().map(Timestamps::collect) {
                    eprintln!("Could not collect Vulkan timestamps: {error}");
                }
            }
        }
        if let Err(error) = result {
            // 释放资源前，尝试通过等待设备空闲确认执行已完成。
            let idle = frame
                .device
                .queue_lock
                .lock()
                .map_err(|_| anyhow!("Vulkan queue lock was poisoned"))
                .and_then(|_lock| {
                    unsafe { frame.device.device.device_wait_idle() }.map_err(Into::into)
                });
            if idle.is_err() && error != vk::Result::ERROR_DEVICE_LOST {
                // 无法确认执行完成时，继续保留设备和资源。
                std::mem::forget(std::mem::take(&mut self.frames));
                return Err(anyhow!(
                    "Vulkan submission completion could not be established: {error:?}"
                ));
            }
        }
        unsafe { frame.device.device.destroy_fence(self.fence, None) };
        for frame in &mut self.frames {
            frame.resources.clear();
        }
        self.fence = vk::Fence::null();
        result.context("Vulkan submission wait failed")
    }
}

impl Drop for Submission {
    fn drop(&mut self) {
        if self.fence != vk::Fence::null() {
            if let Err(error) = self.finish() {
                eprintln!("{error:#}");
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    #[ignore = "requires a Vulkan graphics device"]
    fn resources_survive_until_the_gpu_finishes() -> Result<()> {
        use std::sync::atomic::Ordering;
        use std::sync::Mutex;

        crate::device::VALIDATION_ERRORS.store(0, Ordering::Relaxed);
        let initial = Backend::new(true)?;
        assert!(initial.create_transfer_buffer(0).is_err());
        assert!(initial.create_transfer_buffer(15).is_err());
        let other_device_buffer = initial.create_transfer_buffer(16)?;
        if initial.report.api_version < vk::API_VERSION_1_2 {
            bail!("this GPU test requires Vulkan 1.2 timeline semaphores");
        }
        let mut timeline_features = vk::PhysicalDeviceTimelineSemaphoreFeatures::default();
        let mut features = vk::PhysicalDeviceFeatures2::default().push_next(&mut timeline_features);
        unsafe {
            initial
                .device
                .instance
                .instance
                .get_physical_device_features2(initial.device.physical_device, &mut features);
        }
        if timeline_features.timeline_semaphore != vk::TRUE {
            bail!("this GPU test requires timeline semaphores");
        }
        let priorities = [1.0];
        let queues = [vk::DeviceQueueCreateInfo::default()
            .queue_family_index(initial.device.queue_family)
            .queue_priorities(&priorities)];
        let mut timeline_features =
            vk::PhysicalDeviceTimelineSemaphoreFeatures::default().timeline_semaphore(true);
        let info = vk::DeviceCreateInfo::default()
            .queue_create_infos(&queues)
            .push_next(&mut timeline_features);
        let raw_device = unsafe {
            initial.device.instance.instance.create_device(
                initial.device.physical_device,
                &info,
                None,
            )?
        };
        let queue = unsafe { raw_device.get_device_queue(initial.device.queue_family, 0) };
        let backend = Backend {
            report: initial.report,
            device: Arc::new(DeviceState {
                instance: initial.device.instance.clone(),
                physical_device: initial.device.physical_device,
                api_version: initial.device.api_version,
                ray_tracing_enabled: false,
                device: raw_device,
                queue,
                queue_family: initial.device.queue_family,
                timestamp_bits: initial.device.timestamp_bits,
                timestamp_period: initial.device.timestamp_period,
                queue_lock: Mutex::new(()),
            }),
        };
        drop(initial.device);
        let device = &backend.device.device;
        let buffer = backend.create_transfer_buffer(16)?;
        let weak = Arc::downgrade(&buffer.inner);
        let mut frame = backend.begin_frame()?;
        assert!(frame.fill_buffer(&other_device_buffer, 0).is_err());
        drop(other_device_buffer);
        frame.fill_buffer(&buffer, 0x12345678)?;

        // 使用时间线信号量阻塞队列，以检查执行中的资源是否存活。
        let mut semaphore_type = vk::SemaphoreTypeCreateInfo::default()
            .semaphore_type(vk::SemaphoreType::TIMELINE)
            .initial_value(0);
        let semaphore_info = vk::SemaphoreCreateInfo::default().push_next(&mut semaphore_type);
        let semaphore = unsafe { device.create_semaphore(&semaphore_info, None)? };
        let semaphores = [semaphore];
        let values = [1];
        let stages = [vk::PipelineStageFlags::TOP_OF_PIPE];
        let mut timeline =
            vk::TimelineSemaphoreSubmitInfo::default().wait_semaphore_values(&values);
        let wait = vk::SubmitInfo::default()
            .wait_semaphores(&semaphores)
            .wait_dst_stage_mask(&stages)
            .push_next(&mut timeline);
        unsafe { device.queue_submit(backend.device.queue, &[wait], vk::Fence::null())? };
        let submission = match frame.submit() {
            Ok(submission) => submission,
            Err(error) => {
                let signal = vk::SemaphoreSignalInfo::default()
                    .semaphore(semaphore)
                    .value(1);
                if unsafe { device.signal_semaphore(&signal) }.is_err() {
                    std::mem::forget(backend);
                }
                return Err(error);
            }
        };
        drop(buffer);
        let is_complete = submission.is_complete();
        let was_alive = weak.upgrade().is_some();
        let signal = vk::SemaphoreSignalInfo::default()
            .semaphore(semaphore)
            .value(1);
        if let Err(error) = unsafe { device.signal_semaphore(&signal) } {
            std::mem::forget(submission);
            return Err(error.into());
        }
        submission.wait()?;
        unsafe { device.destroy_semaphore(semaphore, None) };
        assert!(!is_complete?);
        assert!(was_alive);
        assert!(weak.upgrade().is_none());
        drop(backend);
        assert_eq!(crate::device::VALIDATION_ERRORS.load(Ordering::Relaxed), 0);
        Ok(())
    }
}
