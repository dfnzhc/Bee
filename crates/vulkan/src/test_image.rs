use crate::device::{Backend, DeviceState};
use crate::Submission;
use anyhow::{bail, Context, Result};
use ash::vk;
use std::sync::Arc;

const FORMAT: vk::Format = vk::Format::B8G8R8A8_UNORM;

pub struct TestImage {
    pub width: u32,
    pub height: u32,
    pub row_stride: usize,
    pub bgra: Vec<u8>,
}

pub struct PendingTestImage {
    submission: Submission,
    target: Arc<TestTarget>,
    width: u32,
    height: u32,
    row_stride: usize,
}

struct TestTarget {
    device: Arc<DeviceState>,
    image: vk::Image,
    image_memory: vk::DeviceMemory,
    view: vk::ImageView,
    render_pass: vk::RenderPass,
    framebuffer: vk::Framebuffer,
    buffer: vk::Buffer,
    buffer_memory: vk::DeviceMemory,
    coherent: bool,
}

impl Drop for TestTarget {
    fn drop(&mut self) {
        unsafe {
            let device = &self.device.device;
            device.destroy_framebuffer(self.framebuffer, None);
            device.destroy_render_pass(self.render_pass, None);
            device.destroy_image_view(self.view, None);
            device.destroy_image(self.image, None);
            device.free_memory(self.image_memory, None);
            device.destroy_buffer(self.buffer, None);
            device.free_memory(self.buffer_memory, None);
        }
    }
}

pub(crate) fn memory_type(
    device: &DeviceState,
    bits: u32,
    required: vk::MemoryPropertyFlags,
    preferred: vk::MemoryPropertyFlags,
) -> Result<(u32, vk::MemoryPropertyFlags)> {
    let properties = unsafe {
        device
            .instance
            .instance
            .get_physical_device_memory_properties(device.physical_device)
    };
    (0..properties.memory_type_count)
        .filter_map(|index| {
            let flags = properties.memory_types[index as usize].property_flags;
            (bits & (1 << index) != 0
                && flags.contains(required)
                && !flags.contains(vk::MemoryPropertyFlags::PROTECTED))
            .then_some((index, flags))
        })
        .min_by_key(|(_, flags)| !flags.contains(preferred))
        .context("no compatible Vulkan memory type")
}

impl TestTarget {
    fn new(backend: &Backend, width: u32, height: u32, size: u64) -> Result<Self> {
        // 空句柄使 Drop 能在创建中途出错时清理已创建的资源。
        let mut target = Self {
            device: backend.device.clone(),
            image: vk::Image::null(),
            image_memory: vk::DeviceMemory::null(),
            view: vk::ImageView::null(),
            render_pass: vk::RenderPass::null(),
            framebuffer: vk::Framebuffer::null(),
            buffer: vk::Buffer::null(),
            buffer_memory: vk::DeviceMemory::null(),
            coherent: false,
        };
        let device = &target.device.device;

        // 渲染到设备图像，再复制到主机可见的缓冲区。
        let image_info = vk::ImageCreateInfo::default()
            .image_type(vk::ImageType::TYPE_2D)
            .format(FORMAT)
            .extent(vk::Extent3D {
                width,
                height,
                depth: 1,
            })
            .mip_levels(1)
            .array_layers(1)
            .samples(vk::SampleCountFlags::TYPE_1)
            .tiling(vk::ImageTiling::OPTIMAL)
            .usage(vk::ImageUsageFlags::COLOR_ATTACHMENT | vk::ImageUsageFlags::TRANSFER_SRC)
            .sharing_mode(vk::SharingMode::EXCLUSIVE)
            .initial_layout(vk::ImageLayout::UNDEFINED);
        target.image = unsafe { device.create_image(&image_info, None) }
            .context("could not create test image")?;
        let requirements = unsafe { device.get_image_memory_requirements(target.image) };
        let (index, _) = memory_type(
            &target.device,
            requirements.memory_type_bits,
            vk::MemoryPropertyFlags::empty(),
            vk::MemoryPropertyFlags::DEVICE_LOCAL,
        )?;
        let allocation = vk::MemoryAllocateInfo::default()
            .allocation_size(requirements.size)
            .memory_type_index(index);
        target.image_memory = unsafe { device.allocate_memory(&allocation, None) }
            .context("could not allocate test image memory")?;
        unsafe { device.bind_image_memory(target.image, target.image_memory, 0) }
            .context("could not bind test image memory")?;

        // 创建帧缓冲所需的图像视图和渲染通道。
        let range = vk::ImageSubresourceRange::default()
            .aspect_mask(vk::ImageAspectFlags::COLOR)
            .level_count(1)
            .layer_count(1);
        let view_info = vk::ImageViewCreateInfo::default()
            .image(target.image)
            .view_type(vk::ImageViewType::TYPE_2D)
            .format(FORMAT)
            .subresource_range(range);
        target.view = unsafe { device.create_image_view(&view_info, None) }
            .context("could not create test image view")?;
        let attachments = [vk::AttachmentDescription::default()
            .format(FORMAT)
            .samples(vk::SampleCountFlags::TYPE_1)
            .load_op(vk::AttachmentLoadOp::CLEAR)
            .store_op(vk::AttachmentStoreOp::STORE)
            .initial_layout(vk::ImageLayout::UNDEFINED)
            .final_layout(vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL)];
        let color_attachments = [vk::AttachmentReference::default()
            .attachment(0)
            .layout(vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL)];
        let subpasses = [vk::SubpassDescription::default()
            .pipeline_bind_point(vk::PipelineBindPoint::GRAPHICS)
            .color_attachments(&color_attachments)];
        let render_pass_info = vk::RenderPassCreateInfo::default()
            .attachments(&attachments)
            .subpasses(&subpasses);
        target.render_pass = unsafe { device.create_render_pass(&render_pass_info, None) }
            .context("could not create test render pass")?;
        let views = [target.view];
        let framebuffer_info = vk::FramebufferCreateInfo::default()
            .render_pass(target.render_pass)
            .attachments(&views)
            .width(width)
            .height(height)
            .layers(1);
        target.framebuffer = unsafe { device.create_framebuffer(&framebuffer_info, None) }
            .context("could not create test framebuffer")?;

        // 优先使用一致性内存；读取前需使非一致性内存的缓存失效。
        let buffer_info = vk::BufferCreateInfo::default()
            .size(size)
            .usage(vk::BufferUsageFlags::TRANSFER_DST)
            .sharing_mode(vk::SharingMode::EXCLUSIVE);
        target.buffer = unsafe { device.create_buffer(&buffer_info, None) }
            .context("could not create image readback buffer")?;
        let requirements = unsafe { device.get_buffer_memory_requirements(target.buffer) };
        let (index, flags) = memory_type(
            &target.device,
            requirements.memory_type_bits,
            vk::MemoryPropertyFlags::HOST_VISIBLE,
            vk::MemoryPropertyFlags::HOST_COHERENT,
        )?;
        target.coherent = flags.contains(vk::MemoryPropertyFlags::HOST_COHERENT);
        let allocation = vk::MemoryAllocateInfo::default()
            .allocation_size(requirements.size)
            .memory_type_index(index);
        target.buffer_memory = unsafe { device.allocate_memory(&allocation, None) }
            .context("could not allocate image readback memory")?;
        unsafe { device.bind_buffer_memory(target.buffer, target.buffer_memory, 0) }
            .context("could not bind image readback memory")?;
        Ok(target)
    }
}

impl Backend {
    pub fn submit_test_image(&self, width: u32, height: u32) -> Result<PendingTestImage> {
        if width == 0 || height == 0 {
            bail!("test image dimensions must be nonzero");
        }
        let usage = vk::ImageUsageFlags::COLOR_ATTACHMENT | vk::ImageUsageFlags::TRANSFER_SRC;
        let properties = unsafe {
            self.device
                .instance
                .instance
                .get_physical_device_image_format_properties(
                    self.device.physical_device,
                    FORMAT,
                    vk::ImageType::TYPE_2D,
                    vk::ImageTiling::OPTIMAL,
                    usage,
                    vk::ImageCreateFlags::empty(),
                )
        }
        .context("test image format is not supported")?;
        if width > properties.max_extent.width || height > properties.max_extent.height {
            bail!("test image dimensions exceed Vulkan limits");
        }

        // Vulkan 将图像复制到缓冲区时，保持每行按 16 个像素对齐。
        let row_pixels = width
            .checked_add(15)
            .map(|value| value & !15)
            .context("test image row size overflow")?;
        let row_stride = usize::try_from(row_pixels)?
            .checked_mul(4)
            .context("test image row size overflow")?;
        let buffer_size = row_stride
            .checked_mul(usize::try_from(height)?)
            .context("test image buffer size overflow")?;
        let target = Arc::new(TestTarget::new(self, width, height, buffer_size as u64)?);
        let mut frame = self.begin_frame()?;
        let command_buffer = frame.command_buffer;
        let device = &self.device.device;
        let extent = vk::Extent2D { width, height };
        let area = vk::Rect2D {
            offset: vk::Offset2D { x: 0, y: 0 },
            extent,
        };
        let clears = [vk::ClearValue {
            color: vk::ClearColorValue {
                float32: [0.0, 0.0, 0.0, 1.0],
            },
        }];
        let begin = vk::RenderPassBeginInfo::default()
            .render_pass(target.render_pass)
            .framebuffer(target.framebuffer)
            .render_area(area)
            .clear_values(&clears);

        // 使用四个不同颜色的象限，便于发现方向和颜色顺序错误。
        unsafe {
            device.cmd_begin_render_pass(command_buffer, &begin, vk::SubpassContents::INLINE);
            let middle_x = width / 2;
            let middle_y = height / 2;
            for (x, y, w, h, color) in [
                (0, 0, middle_x, middle_y, [1.0, 0.0, 0.0, 1.0]),
                (
                    middle_x,
                    0,
                    width - middle_x,
                    middle_y,
                    [0.0, 1.0, 0.0, 1.0],
                ),
                (
                    0,
                    middle_y,
                    middle_x,
                    height - middle_y,
                    [0.0, 0.0, 1.0, 1.0],
                ),
                (
                    middle_x,
                    middle_y,
                    width - middle_x,
                    height - middle_y,
                    [1.0, 1.0, 1.0, 1.0],
                ),
            ] {
                if w == 0 || h == 0 {
                    continue;
                }
                let attachment = vk::ClearAttachment::default()
                    .aspect_mask(vk::ImageAspectFlags::COLOR)
                    .color_attachment(0)
                    .clear_value(vk::ClearValue {
                        color: vk::ClearColorValue { float32: color },
                    });
                let rect = vk::ClearRect::default()
                    .rect(vk::Rect2D {
                        offset: vk::Offset2D {
                            x: x as i32,
                            y: y as i32,
                        },
                        extent: vk::Extent2D {
                            width: w,
                            height: h,
                        },
                    })
                    .layer_count(1);
                device.cmd_clear_attachments(command_buffer, &[attachment], &[rect]);
            }
            device.cmd_end_render_pass(command_buffer);

            // 将渲染后的附件转换为可供传输读取的布局。
            let image_barrier = vk::ImageMemoryBarrier::default()
                .src_access_mask(vk::AccessFlags::COLOR_ATTACHMENT_WRITE)
                .dst_access_mask(vk::AccessFlags::TRANSFER_READ)
                .old_layout(vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL)
                .new_layout(vk::ImageLayout::TRANSFER_SRC_OPTIMAL)
                .src_queue_family_index(vk::QUEUE_FAMILY_IGNORED)
                .dst_queue_family_index(vk::QUEUE_FAMILY_IGNORED)
                .image(target.image)
                .subresource_range(
                    vk::ImageSubresourceRange::default()
                        .aspect_mask(vk::ImageAspectFlags::COLOR)
                        .level_count(1)
                        .layer_count(1),
                );
            device.cmd_pipeline_barrier(
                command_buffer,
                vk::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT,
                vk::PipelineStageFlags::TRANSFER,
                vk::DependencyFlags::empty(),
                &[],
                &[],
                &[image_barrier],
            );
            let copy = vk::BufferImageCopy::default()
                .buffer_row_length(row_pixels)
                .image_subresource(
                    vk::ImageSubresourceLayers::default()
                        .aspect_mask(vk::ImageAspectFlags::COLOR)
                        .layer_count(1),
                )
                .image_extent(vk::Extent3D {
                    width,
                    height,
                    depth: 1,
                });
            device.cmd_copy_image_to_buffer(
                command_buffer,
                target.image,
                vk::ImageLayout::TRANSFER_SRC_OPTIMAL,
                target.buffer,
                &[copy],
            );

            // 确保复制的像素在栅栏发出信号后对主机可见。
            let host_barrier = vk::MemoryBarrier::default()
                .src_access_mask(vk::AccessFlags::TRANSFER_WRITE)
                .dst_access_mask(vk::AccessFlags::HOST_READ);
            device.cmd_pipeline_barrier(
                command_buffer,
                vk::PipelineStageFlags::TRANSFER,
                vk::PipelineStageFlags::HOST,
                vk::DependencyFlags::empty(),
                &[host_barrier],
                &[],
                &[],
            );
        }

        // 保留全部图像资源，直到提交的帧执行完成。
        frame.resources.push(target.clone());
        let submission = frame.submit()?;
        Ok(PendingTestImage {
            submission,
            target,
            width,
            height,
            row_stride,
        })
    }
}

impl PendingTestImage {
    pub fn is_complete(&self) -> Result<bool> {
        self.submission.is_complete()
    }

    pub fn read(self) -> Result<TestImage> {
        self.submission.wait()?;
        let device = &self.target.device.device;
        let memory = self.target.buffer_memory;
        let ptr =
            unsafe { device.map_memory(memory, 0, vk::WHOLE_SIZE, vk::MemoryMapFlags::empty()) }
                .context("could not map image readback memory")?;

        // 栅栏完成并不会自动使非一致性内存的主机缓存失效。
        if !self.target.coherent {
            let range = vk::MappedMemoryRange::default()
                .memory(memory)
                .offset(0)
                .size(vk::WHOLE_SIZE);
            if let Err(error) = unsafe { device.invalidate_mapped_memory_ranges(&[range]) } {
                unsafe { device.unmap_memory(memory) };
                return Err(error).context("could not invalidate image readback memory");
            }
        }

        let mut bgra = vec![0; self.row_stride * self.height as usize];
        // 逐行复制可见像素，并将填充部分保留为零。
        for row in 0..self.height as usize {
            unsafe {
                std::ptr::copy_nonoverlapping(
                    (ptr as *const u8).add(row * self.row_stride),
                    bgra.as_mut_ptr().add(row * self.row_stride),
                    self.width as usize * 4,
                );
            }
        }
        unsafe { device.unmap_memory(memory) };
        Ok(TestImage {
            width: self.width,
            height: self.height,
            row_stride: self.row_stride,
            bgra,
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::atomic::Ordering;

    #[test]
    #[ignore = "requires a Vulkan graphics device"]
    fn offscreen_test_image_has_expected_colors_and_stride() -> Result<()> {
        crate::device::VALIDATION_ERRORS.store(0, Ordering::Relaxed);
        let backend = Backend::new(true)?;
        assert!(backend.submit_test_image(0, 1).is_err());
        let pending = backend.submit_test_image(17, 9)?;
        drop(backend);
        let image = pending.read()?;
        assert_eq!(image.row_stride, 128);
        let pixel = |x: usize, y: usize| {
            let offset = y * image.row_stride + x * 4;
            &image.bgra[offset..offset + 4]
        };
        assert_eq!(pixel(0, 0), &[0, 0, 255, 255]);
        assert_eq!(pixel(16, 0), &[0, 255, 0, 255]);
        assert_eq!(pixel(0, 8), &[255, 0, 0, 255]);
        assert_eq!(pixel(16, 8), &[255, 255, 255, 255]);
        assert!(image.bgra[17 * 4..image.row_stride]
            .iter()
            .all(|value| *value == 0));
        assert_eq!(crate::device::VALIDATION_ERRORS.load(Ordering::Relaxed), 0);
        Ok(())
    }

    #[test]
    #[ignore = "requires a Vulkan graphics device and validation layer"]
    fn replaced_images_remain_alive_until_submissions_finish() -> Result<()> {
        use crate::ValidationStatus;

        crate::device::VALIDATION_ERRORS.store(0, Ordering::Relaxed);
        let backend = Backend::new(true)?;
        assert_eq!(backend.report.validation, ValidationStatus::Enabled);
        let first = backend.submit_test_image(17, 9)?;
        let first_target = Arc::downgrade(&first.target);
        let second = backend.submit_test_image(7, 13)?;
        let second_target = Arc::downgrade(&second.target);
        assert!(first_target.upgrade().is_some());
        assert!(second_target.upgrade().is_some());
        drop(first);
        assert!(first_target.upgrade().is_none());
        let image = second.read()?;
        assert_eq!((image.width, image.height), (7, 13));
        assert!(second_target.upgrade().is_none());
        drop(backend);
        assert_eq!(crate::device::VALIDATION_ERRORS.load(Ordering::Relaxed), 0);
        Ok(())
    }
}
