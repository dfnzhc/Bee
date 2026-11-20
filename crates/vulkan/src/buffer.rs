use crate::device::DeviceState;
use crate::test_image::memory_type;
use anyhow::{bail, Context, Result};
use ash::vk;
use std::sync::Arc;

#[derive(Clone)]
pub struct Buffer {
    pub(crate) inner: Arc<BufferState>,
}

impl Buffer {
    pub fn raw(&self) -> vk::Buffer {
        self.inner.buffer
    }

    pub fn usage(&self) -> vk::BufferUsageFlags {
        self.inner.usage
    }

    pub fn host_visible(&self) -> bool {
        self.inner.host_visible
    }
}

pub(crate) struct BufferState {
    pub device: Arc<DeviceState>,
    pub buffer: vk::Buffer,
    pub memory: vk::DeviceMemory,
    pub size: u64,
    pub address: vk::DeviceAddress,
    pub coherent: bool,
    host_visible: bool,
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

impl BufferState {
    pub fn allocate(
        device: &Arc<DeviceState>,
        size: u64,
        mut usage: vk::BufferUsageFlags,
        host_visible: bool,
        needs_address: bool,
    ) -> Result<Self> {
        if size == 0 || (needs_address && !device.ray_tracing_enabled) {
            bail!("invalid Vulkan buffer size or unsupported device address");
        }
        if needs_address {
            usage |= vk::BufferUsageFlags::SHADER_DEVICE_ADDRESS;
        }
        let mut result = Self {
            device: device.clone(),
            buffer: vk::Buffer::null(),
            memory: vk::DeviceMemory::null(),
            size,
            address: 0,
            coherent: false,
            host_visible: false,
            usage,
        };
        let vk_device = &device.device;
        unsafe {
            result.buffer = vk_device
                .create_buffer(
                    &vk::BufferCreateInfo::default()
                        .size(size)
                        .usage(usage)
                        .sharing_mode(vk::SharingMode::EXCLUSIVE),
                    None,
                )
                .context("could not create Vulkan buffer")?;
            let requirements = vk_device.get_buffer_memory_requirements(result.buffer);
            let (required, preferred) = if host_visible {
                (
                    vk::MemoryPropertyFlags::HOST_VISIBLE,
                    vk::MemoryPropertyFlags::HOST_COHERENT,
                )
            } else {
                (
                    vk::MemoryPropertyFlags::empty(),
                    vk::MemoryPropertyFlags::DEVICE_LOCAL,
                )
            };
            let (index, flags) =
                memory_type(device, requirements.memory_type_bits, required, preferred)?;
            result.coherent = flags.contains(vk::MemoryPropertyFlags::HOST_COHERENT);
            result.host_visible = flags.contains(vk::MemoryPropertyFlags::HOST_VISIBLE);
            let mut address_flags = vk::MemoryAllocateFlagsInfo::default()
                .flags(vk::MemoryAllocateFlags::DEVICE_ADDRESS);
            let mut info = vk::MemoryAllocateInfo::default()
                .allocation_size(requirements.size)
                .memory_type_index(index);
            if needs_address {
                info = info.push_next(&mut address_flags);
            }
            result.memory = vk_device
                .allocate_memory(&info, None)
                .context("could not allocate Vulkan buffer memory")?;
            vk_device
                .bind_buffer_memory(result.buffer, result.memory, 0)
                .context("could not bind Vulkan buffer memory")?;
            if needs_address {
                let info = vk::BufferDeviceAddressInfo::default().buffer(result.buffer);
                result.address = if device.api_version >= vk::API_VERSION_1_2 {
                    vk_device.get_buffer_device_address(&info)
                } else {
                    ash::khr::buffer_device_address::Device::new(
                        &device.instance.instance,
                        vk_device,
                    )
                    .get_buffer_device_address(&info)
                };
                if result.address == 0 {
                    bail!("Vulkan buffer has no device address");
                }
            }
        }
        Ok(result)
    }

    pub fn write(&self, bytes: &[u8]) -> Result<()> {
        if bytes.len() as u64 > self.size {
            bail!("Vulkan buffer upload exceeds its size");
        }
        let device = &self.device.device;
        unsafe {
            let mapped = device
                .map_memory(self.memory, 0, vk::WHOLE_SIZE, vk::MemoryMapFlags::empty())
                .context("could not map Vulkan buffer")?;
            std::ptr::copy_nonoverlapping(bytes.as_ptr(), mapped.cast::<u8>(), bytes.len());
            let flush = if self.coherent {
                Ok(())
            } else {
                device.flush_mapped_memory_ranges(&[vk::MappedMemoryRange::default()
                    .memory(self.memory)
                    .offset(0)
                    .size(vk::WHOLE_SIZE)])
            };
            device.unmap_memory(self.memory);
            flush.context("could not flush Vulkan buffer")?;
        }
        Ok(())
    }

    pub fn descriptor(&self) -> vk::DescriptorBufferInfo {
        vk::DescriptorBufferInfo::default()
            .buffer(self.buffer)
            .range(self.size)
    }
}
