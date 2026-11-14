use crate::device::{Backend, DeviceState};
use anyhow::{anyhow, bail, Context, Result};
use ash::vk;
use std::sync::Arc;

#[derive(Clone)]
pub struct Buffer {
    pub(crate) inner: Arc<BufferState>,
}

pub(crate) struct BufferState {
    pub(crate) device: Arc<DeviceState>,
    pub(crate) buffer: vk::Buffer,
    memory: vk::DeviceMemory,
    pub(crate) size: u64,
    usage: vk::BufferUsageFlags,
}

impl Drop for BufferState {
    fn drop(&mut self) {
        unsafe {
            self.device.device.destroy_buffer(self.buffer, None);
            self.device.device.free_memory(self.memory, None);
        }
    }
}

pub struct Frame {
    pub(crate) device: Arc<DeviceState>,
    pool: vk::CommandPool,
    pub(crate) command_buffer: vk::CommandBuffer,
    pub(crate) resources: Vec<Arc<dyn Send + Sync>>,
}

impl Drop for Frame {
    fn drop(&mut self) {
        unsafe { self.device.device.destroy_command_pool(self.pool, None) };
    }
}

pub struct Submission {
    frame: Option<Frame>,
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
        let device = &self.device.device;
        let info = vk::BufferCreateInfo::default()
            .size(size)
            .usage(usage)
            .sharing_mode(vk::SharingMode::EXCLUSIVE);
        let buffer = unsafe { device.create_buffer(&info, None) }
            .context("could not create Vulkan buffer")?;
        let requirements = unsafe { device.get_buffer_memory_requirements(buffer) };
        let properties = unsafe {
            self.device
                .instance
                .instance
                .get_physical_device_memory_properties(self.device.physical_device)
        };

        // 优先使用设备本地内存，同时排除受保护的内存。
        let memory_type = (0..properties.memory_type_count)
            .filter(|index| {
                requirements.memory_type_bits & (1 << index) != 0
                    && !properties.memory_types[*index as usize]
                        .property_flags
                        .contains(vk::MemoryPropertyFlags::PROTECTED)
            })
            .min_by_key(|index| {
                !properties.memory_types[*index as usize]
                    .property_flags
                    .contains(vk::MemoryPropertyFlags::DEVICE_LOCAL)
            });
        let Some(memory_type) = memory_type else {
            unsafe { device.destroy_buffer(buffer, None) };
            bail!("no compatible Vulkan memory type for transfer buffer");
        };
        let allocation = vk::MemoryAllocateInfo::default()
            .allocation_size(requirements.size)
            .memory_type_index(memory_type);
        let memory = match unsafe { device.allocate_memory(&allocation, None) } {
            Ok(memory) => memory,
            Err(error) => {
                unsafe { device.destroy_buffer(buffer, None) };
                return Err(error).context("could not allocate Vulkan buffer memory");
            }
        };
        if let Err(error) = unsafe { device.bind_buffer_memory(buffer, memory, 0) } {
            unsafe {
                device.destroy_buffer(buffer, None);
                device.free_memory(memory, None);
            }
            return Err(error).context("could not bind Vulkan buffer memory");
        }
        Ok(Buffer {
            inner: Arc::new(BufferState {
                device: self.device.clone(),
                buffer,
                memory,
                size,
                usage,
            }),
        })
    }

    pub fn begin_frame(&self) -> Result<Frame> {
        // 每帧持有自己的命令池，确保录制的命令在提交期间保持有效。
        let device = &self.device.device;
        let info = vk::CommandPoolCreateInfo::default()
            .queue_family_index(self.device.queue_family)
            .flags(vk::CommandPoolCreateFlags::TRANSIENT);
        let pool = unsafe { device.create_command_pool(&info, None) }
            .context("could not create Vulkan command pool")?;
        let allocate = vk::CommandBufferAllocateInfo::default()
            .command_pool(pool)
            .level(vk::CommandBufferLevel::PRIMARY)
            .command_buffer_count(1);
        let command_buffer = match unsafe { device.allocate_command_buffers(&allocate) } {
            Ok(buffers) => buffers[0],
            Err(error) => {
                unsafe { device.destroy_command_pool(pool, None) };
                return Err(error).context("could not allocate Vulkan command buffer");
            }
        };
        let begin = vk::CommandBufferBeginInfo::default()
            .flags(vk::CommandBufferUsageFlags::ONE_TIME_SUBMIT);
        if let Err(error) = unsafe { device.begin_command_buffer(command_buffer, &begin) } {
            unsafe { device.destroy_command_pool(pool, None) };
            return Err(error).context("could not begin Vulkan command buffer");
        }
        Ok(Frame {
            device: self.device.clone(),
            pool,
            command_buffer,
            resources: Vec::new(),
        })
    }
}

impl Buffer {
    pub fn raw(&self) -> vk::Buffer {
        self.inner.buffer
    }
    pub fn usage(&self) -> vk::BufferUsageFlags {
        self.inner.usage
    }
}

impl Frame {
    pub fn fill_buffer(&mut self, buffer: &Buffer, value: u32) -> Result<()> {
        if !Arc::ptr_eq(&self.device, &buffer.inner.device) {
            bail!("buffer belongs to a different Vulkan device");
        }
        if !buffer
            .inner
            .usage
            .contains(vk::BufferUsageFlags::TRANSFER_DST)
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
        if !Arc::ptr_eq(&self.device, &buffer.inner.device) {
            bail!("graph buffer belongs to a different Vulkan device");
        }
        let barrier = vk::BufferMemoryBarrier::default()
            .src_access_mask(before.1)
            .dst_access_mask(after.1)
            .src_queue_family_index(vk::QUEUE_FAMILY_IGNORED)
            .dst_queue_family_index(vk::QUEUE_FAMILY_IGNORED)
            .buffer(buffer.raw())
            .offset(0)
            .size(vk::WHOLE_SIZE);
        self.device.device.cmd_pipeline_barrier(
            self.command_buffer,
            before.0,
            after.0,
            vk::DependencyFlags::empty(),
            &[],
            &[barrier],
            &[],
        );
        self.resources.push(buffer.inner.clone());
        Ok(())
    }

    pub fn submit(self) -> Result<Submission> {
        let device = &self.device.device;
        unsafe { device.end_command_buffer(self.command_buffer) }
            .context("could not finish Vulkan command buffer")?;
        let fence = unsafe { device.create_fence(&vk::FenceCreateInfo::default(), None) }
            .context("could not create Vulkan submission fence")?;
        let command_buffers = [self.command_buffer];
        let submissions = [vk::SubmitInfo::default().command_buffers(&command_buffers)];

        // Vulkan 要求在外部同步对队列的提交。
        let result = match self.device.queue_lock.lock() {
            Ok(_lock) => unsafe { device.queue_submit(self.device.queue, &submissions, fence) }
                .context("could not submit Vulkan commands"),
            Err(_) => Err(anyhow!("Vulkan queue lock was poisoned")),
        };
        if let Err(error) = result {
            unsafe { device.destroy_fence(fence, None) };
            return Err(error);
        }
        Ok(Submission {
            frame: Some(self),
            fence,
        })
    }
}

impl Submission {
    pub fn is_complete(&self) -> Result<bool> {
        let frame = self.frame.as_ref().expect("submission is already finished");
        unsafe { frame.device.device.get_fence_status(self.fence) }
            .context("could not query Vulkan submission fence")
    }

    pub fn wait(mut self) -> Result<()> {
        self.finish()
    }

    fn finish(&mut self) -> Result<()> {
        let Some(frame) = self.frame.as_ref() else {
            return Ok(());
        };
        let result = unsafe {
            frame
                .device
                .device
                .wait_for_fences(&[self.fence], true, u64::MAX)
        };
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
                std::mem::forget(self.frame.take());
                return Err(anyhow!(
                    "Vulkan submission completion could not be established: {error:?}"
                ));
            }
        }
        unsafe { frame.device.device.destroy_fence(self.fence, None) };
        self.frame.take();
        result.context("Vulkan submission wait failed")
    }
}

impl Drop for Submission {
    fn drop(&mut self) {
        if let Err(error) = self.finish() {
            eprintln!("{error:#}");
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
                device: raw_device,
                queue,
                queue_family: initial.device.queue_family,
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
