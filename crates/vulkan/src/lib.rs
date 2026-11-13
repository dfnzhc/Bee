//! Vulkan rendering backend.

mod capabilities;
mod device;
mod frame;

pub use device::{Backend, DeviceReport, ValidationStatus};
pub use frame::{Buffer, Frame, Submission};

pub const FOUNDATION_SHADER: &[u8] =
    include_bytes!(concat!(env!("OUT_DIR"), "/foundation.vert.spv"));
