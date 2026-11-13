//! Vulkan 渲染后端。

mod capabilities;
mod device;
mod frame;
mod sharing;
mod test_image;

pub use device::{Backend, DeviceReport, ValidationStatus};
pub use frame::{Buffer, Frame, Submission};
pub use sharing::{ExternalHandleSupport, SharingProbe};
pub use test_image::{PendingTestImage, TestImage};

pub const FOUNDATION_SHADER: &[u8] =
    include_bytes!(concat!(env!("OUT_DIR"), "/foundation.vert.spv"));
