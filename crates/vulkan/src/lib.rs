//! Vulkan 渲染后端。

mod capabilities;
mod device;
mod frame;
mod graph_image;
mod raster;
mod sharing;
mod test_image;
mod tonemap;

pub use device::{Backend, DeviceReport, ValidationStatus};
pub use frame::{Buffer, Frame, Submission};
pub use graph_image::{GraphImage, ReadbackBuffer};
pub use raster::Rasterizer;
pub use sharing::{ExternalHandleSupport, SharingProbe};
pub use test_image::{PendingTestImage, TestImage};
pub use tonemap::ToneMapper;

pub const FOUNDATION_SHADER: &[u8] =
    include_bytes!(concat!(env!("OUT_DIR"), "/foundation.vert.spv"));
