use crate::buffer::BufferState;
use crate::device::{Backend, DeviceState};
use crate::test_image::memory_type;
use crate::{Buffer, Frame};
use anyhow::{bail, Context, Result};
use ash::vk;
use std::sync::Arc;

#[derive(Clone)]
pub struct GraphImage {
    pub(crate) inner: Arc<ImageState>,
}

pub(crate) struct ImageState {
    pub(crate) device: Arc<DeviceState>,
    pub(crate) image: vk::Image,
    memory: vk::DeviceMemory,
    pub(crate) width: u32,
    pub(crate) height: u32,
    pub(crate) format: vk::Format,
    pub(crate) usage: vk::ImageUsageFlags,
    pub(crate) aspect: vk::ImageAspectFlags,
}

impl Drop for ImageState {
    fn drop(&mut self) {
        unsafe {
            self.device.device.destroy_image(self.image, None);
            self.device.device.free_memory(self.memory, None);
        }
    }
}

pub struct ReadbackBuffer {
    inner: Arc<BufferState>,
}

impl Backend {
    /// # Safety
    /// format、usage 和 aspect 必须是 Vulkan 支持且彼此兼容的图像参数。
    pub unsafe fn create_graph_image(
        &self,
        width: u32,
        height: u32,
        format: vk::Format,
        usage: vk::ImageUsageFlags,
        aspect: vk::ImageAspectFlags,
    ) -> Result<GraphImage> {
        if width == 0
            || height == 0
            || usage.is_empty()
            || !matches!(
                aspect,
                vk::ImageAspectFlags::COLOR | vk::ImageAspectFlags::DEPTH
            )
        {
            bail!("invalid graph image dimensions, usage or aspect");
        }
        let properties = unsafe {
            self.device
                .instance
                .instance
                .get_physical_device_image_format_properties(
                    self.device.physical_device,
                    format,
                    vk::ImageType::TYPE_2D,
                    vk::ImageTiling::OPTIMAL,
                    usage,
                    vk::ImageCreateFlags::empty(),
                )
        }
        .context("graph image format or usage is not supported")?;
        if width > properties.max_extent.width || height > properties.max_extent.height {
            bail!("graph image dimensions exceed Vulkan limits");
        }
        let mut image = ImageState {
            device: self.device.clone(),
            image: vk::Image::null(),
            memory: vk::DeviceMemory::null(),
            width,
            height,
            format,
            usage,
            aspect,
        };
        let info = vk::ImageCreateInfo::default()
            .image_type(vk::ImageType::TYPE_2D)
            .format(format)
            .extent(vk::Extent3D {
                width,
                height,
                depth: 1,
            })
            .mip_levels(1)
            .array_layers(1)
            .samples(vk::SampleCountFlags::TYPE_1)
            .tiling(vk::ImageTiling::OPTIMAL)
            .usage(usage)
            .sharing_mode(vk::SharingMode::EXCLUSIVE)
            .initial_layout(vk::ImageLayout::UNDEFINED);
        image.image = unsafe { image.device.device.create_image(&info, None) }
            .context("could not create graph image")?;
        let requirements = unsafe {
            image
                .device
                .device
                .get_image_memory_requirements(image.image)
        };
        let (index, _) = memory_type(
            &image.device,
            requirements.memory_type_bits,
            vk::MemoryPropertyFlags::empty(),
            vk::MemoryPropertyFlags::DEVICE_LOCAL,
        )?;
        let allocation = vk::MemoryAllocateInfo::default()
            .allocation_size(requirements.size)
            .memory_type_index(index);
        image.memory = unsafe { image.device.device.allocate_memory(&allocation, None) }
            .context("could not allocate graph image memory")?;
        unsafe {
            image
                .device
                .device
                .bind_image_memory(image.image, image.memory, 0)
        }
        .context("could not bind graph image memory")?;
        Ok(GraphImage {
            inner: Arc::new(image),
        })
    }

    pub fn create_readback_buffer(&self, size: usize) -> Result<ReadbackBuffer> {
        if size == 0 {
            bail!("readback buffer size must be nonzero");
        }
        Ok(ReadbackBuffer {
            inner: Arc::new(BufferState::allocate(
                &self.device,
                size as u64,
                vk::BufferUsageFlags::TRANSFER_DST,
                true,
                false,
            )?),
        })
    }
}

impl GraphImage {
    pub fn raw(&self) -> vk::Image {
        self.inner.image
    }
    pub fn usage(&self) -> vk::ImageUsageFlags {
        self.inner.usage
    }
    pub fn aspect(&self) -> vk::ImageAspectFlags {
        self.inner.aspect
    }
}

impl ReadbackBuffer {
    pub fn graph_buffer(&self) -> Buffer {
        Buffer {
            inner: self.inner.clone(),
        }
    }

    /// # Safety
    /// 调用方必须先等待所有写入此缓冲区的提交完成，并保证没有并发访问。
    pub unsafe fn read(&self) -> Result<Vec<u8>> {
        let device = &self.inner.device.device;
        let ptr = device
            .map_memory(
                self.inner.memory,
                0,
                vk::WHOLE_SIZE,
                vk::MemoryMapFlags::empty(),
            )
            .context("could not map readback memory")?;
        if !self.inner.coherent {
            let range = vk::MappedMemoryRange::default()
                .memory(self.inner.memory)
                .offset(0)
                .size(vk::WHOLE_SIZE);
            if let Err(error) = device.invalidate_mapped_memory_ranges(&[range]) {
                device.unmap_memory(self.inner.memory);
                return Err(error).context("could not invalidate readback memory");
            }
        }
        let bytes = std::slice::from_raw_parts(ptr.cast::<u8>(), self.inner.size as usize).to_vec();
        device.unmap_memory(self.inner.memory);
        Ok(bytes)
    }
}

impl Frame {
    /// # Safety
    /// 缓冲区的前序写入必须已对传输读取可见。
    pub unsafe fn copy_graph_buffer_to_readback(
        &mut self,
        source: &Buffer,
        target: &ReadbackBuffer,
    ) -> Result<()> {
        if !Arc::ptr_eq(&self.device, &source.inner.device)
            || !Arc::ptr_eq(&self.device, &target.inner.device)
            || !source.usage().contains(vk::BufferUsageFlags::TRANSFER_SRC)
            || source.inner.size > target.inner.size
        {
            bail!("incompatible buffer or readback buffer");
        }
        let copy = vk::BufferCopy::default().size(source.inner.size);
        self.device.device.cmd_copy_buffer(
            self.command_buffer,
            source.raw(),
            target.inner.buffer,
            &[copy],
        );
        let barrier = vk::MemoryBarrier::default()
            .src_access_mask(vk::AccessFlags::TRANSFER_WRITE)
            .dst_access_mask(vk::AccessFlags::HOST_READ);
        self.device.device.cmd_pipeline_barrier(
            self.command_buffer,
            vk::PipelineStageFlags::TRANSFER,
            vk::PipelineStageFlags::HOST,
            vk::DependencyFlags::empty(),
            &[barrier],
            &[],
            &[],
        );
        self.resources.push(source.inner.clone());
        self.resources.push(target.inner.clone());
        Ok(())
    }

    /// # Safety
    /// 源图像和目标图像必须分别处于 TRANSFER_SRC_OPTIMAL 和 TRANSFER_DST_OPTIMAL 布局。
    pub unsafe fn copy_graph_image(
        &mut self,
        source: &GraphImage,
        target: &GraphImage,
    ) -> Result<()> {
        if !Arc::ptr_eq(&self.device, &source.inner.device)
            || !Arc::ptr_eq(&self.device, &target.inner.device)
            || source.raw() == target.raw()
            || source.inner.width != target.inner.width
            || source.inner.height != target.inner.height
            || source.inner.format != target.inner.format
            || source.inner.aspect != vk::ImageAspectFlags::COLOR
            || target.inner.aspect != vk::ImageAspectFlags::COLOR
            || !source
                .inner
                .usage
                .contains(vk::ImageUsageFlags::TRANSFER_SRC)
            || !target
                .inner
                .usage
                .contains(vk::ImageUsageFlags::TRANSFER_DST)
        {
            bail!("incompatible graph image copy");
        }
        let subresource = vk::ImageSubresourceLayers::default()
            .aspect_mask(vk::ImageAspectFlags::COLOR)
            .layer_count(1);
        let copy = vk::ImageCopy::default()
            .src_subresource(subresource)
            .dst_subresource(subresource)
            .extent(vk::Extent3D {
                width: source.inner.width,
                height: source.inner.height,
                depth: 1,
            });
        crate::profile::record("image_copy", 1.0);
        self.device.device.cmd_copy_image(
            self.command_buffer,
            source.raw(),
            vk::ImageLayout::TRANSFER_SRC_OPTIMAL,
            target.raw(),
            vk::ImageLayout::TRANSFER_DST_OPTIMAL,
            &[copy],
        );
        self.resources.push(source.inner.clone());
        self.resources.push(target.inner.clone());
        Ok(())
    }

    /// # Safety
    /// 调用方必须保证屏障描述真实的图像状态及访问，并对跨队列使用进行同步。
    pub unsafe fn graph_image_barrier(
        &mut self,
        image: &GraphImage,
        before: (vk::PipelineStageFlags, vk::AccessFlags, vk::ImageLayout),
        after: (vk::PipelineStageFlags, vk::AccessFlags, vk::ImageLayout),
    ) -> Result<()> {
        if !Arc::ptr_eq(&self.device, &image.inner.device) {
            bail!("graph image belongs to a different Vulkan device");
        }
        let range = vk::ImageSubresourceRange::default()
            .aspect_mask(image.inner.aspect)
            .level_count(1)
            .layer_count(1);
        let barrier = vk::ImageMemoryBarrier::default()
            .src_access_mask(before.1)
            .dst_access_mask(after.1)
            .old_layout(before.2)
            .new_layout(after.2)
            .src_queue_family_index(vk::QUEUE_FAMILY_IGNORED)
            .dst_queue_family_index(vk::QUEUE_FAMILY_IGNORED)
            .image(image.inner.image)
            .subresource_range(range);
        self.device.device.cmd_pipeline_barrier(
            self.command_buffer,
            before.0,
            after.0,
            vk::DependencyFlags::empty(),
            &[],
            &[],
            &[barrier],
        );
        self.resources.push(image.inner.clone());
        Ok(())
    }

    /// # Safety
    /// 调用方必须遵守 Vulkan 命令录制要求，并保留所有命令引用的资源。
    pub unsafe fn record_raw(&mut self, record: impl FnOnce(&ash::Device, vk::CommandBuffer)) {
        record(&self.device.device, self.command_buffer);
    }

    /// # Safety
    /// 图像必须处于 TRANSFER_SRC_OPTIMAL 布局，且前序写入已对传输读取可见。
    pub unsafe fn copy_graph_image_to_readback(
        &mut self,
        image: &GraphImage,
        buffer: &ReadbackBuffer,
    ) -> Result<()> {
        self.copy_graph_image_to_buffer(image, buffer)?;
        self.graph_buffer_barrier(
            &buffer.graph_buffer(),
            (
                vk::PipelineStageFlags::TRANSFER,
                vk::AccessFlags::TRANSFER_WRITE,
            ),
            (vk::PipelineStageFlags::HOST, vk::AccessFlags::HOST_READ),
        )
    }

    /// # Safety
    /// 调用方须同步图像传输读取及缓冲区传输写入，读回前还须同步宿主读取。
    pub unsafe fn copy_graph_image_to_buffer(
        &mut self,
        image: &GraphImage,
        buffer: &ReadbackBuffer,
    ) -> Result<()> {
        if !Arc::ptr_eq(&self.device, &image.inner.device)
            || !Arc::ptr_eq(&self.device, &buffer.inner.device)
        {
            bail!("readback resources belong to a different Vulkan device");
        }
        if image.inner.aspect != vk::ImageAspectFlags::COLOR
            || !matches!(
                image.inner.format,
                vk::Format::R8G8B8A8_UNORM | vk::Format::B8G8R8A8_UNORM
            )
            || !image
                .inner
                .usage
                .contains(vk::ImageUsageFlags::TRANSFER_SRC)
            || (image.inner.width as usize)
                .checked_mul(image.inner.height as usize)
                .and_then(|size| size.checked_mul(4))
                .is_none_or(|size| size as u64 > buffer.inner.size)
        {
            bail!("incompatible image or readback buffer");
        }
        let copy = vk::BufferImageCopy::default()
            .image_subresource(
                vk::ImageSubresourceLayers::default()
                    .aspect_mask(vk::ImageAspectFlags::COLOR)
                    .layer_count(1),
            )
            .image_extent(vk::Extent3D {
                width: image.inner.width,
                height: image.inner.height,
                depth: 1,
            });
        self.device.device.cmd_copy_image_to_buffer(
            self.command_buffer,
            image.inner.image,
            vk::ImageLayout::TRANSFER_SRC_OPTIMAL,
            buffer.inner.buffer,
            &[copy],
        );
        self.resources.push(image.inner.clone());
        self.resources.push(buffer.inner.clone());
        Ok(())
    }
}
