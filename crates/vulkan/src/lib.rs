//! Vulkan 渲染后端。

mod capabilities;
mod device;
mod diagnostics;
mod frame;
mod graph_image;
pub mod profile;
mod raster;
mod ray;
mod sharing;
mod test_image;
mod tonemap;

pub use device::{Backend, DeviceReport, ValidationStatus};
pub use diagnostics::{
    diagnostic_channel, DiagnosticLevel, DiagnosticMessage, DiagnosticReceiver, DiagnosticSender,
    DIAGNOSTIC_QUEUE_CAPACITY,
};
pub use frame::{Buffer, Frame, Submission};
pub use graph_image::{GraphImage, ReadbackBuffer};
pub use raster::{RasterGeometry, Rasterizer};
pub use ray::{AccelerationScene, PathTracer};
pub use sharing::{ExternalHandleSupport, SharingProbe};
pub use test_image::{PendingTestImage, TestImage};
pub use tonemap::ToneMapper;

pub const FOUNDATION_SHADER: &[u8] =
    include_bytes!(concat!(env!("OUT_DIR"), "/foundation.vert.spv"));
