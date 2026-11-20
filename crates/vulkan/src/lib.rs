//! Vulkan 渲染后端。

mod acceleration;
mod buffer;
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

pub use acceleration::{AccelerationMesh, AccelerationScene, RayInstance};
pub use buffer::Buffer;
pub use device::{Backend, DeviceReport, ValidationStatus};
pub use diagnostics::{
    diagnostic_channel, DiagnosticLevel, DiagnosticMessage, DiagnosticReceiver, DiagnosticSender,
    DIAGNOSTIC_QUEUE_CAPACITY,
};
pub use frame::{Frame, RecordedCommands, Submission, WorkerCommands};
pub use graph_image::{GraphImage, ReadbackBuffer};
pub use raster::{
    RasterCommands, RasterGeometry, RasterInstance, RasterInstances, RasterPass, Rasterizer,
};
pub use ray::{PathTracer, SceneBuffer};
pub use sharing::{ExternalHandleSupport, SharingProbe};
pub use test_image::{PendingTestImage, TestImage};
pub use tonemap::ToneMapper;

pub const FOUNDATION_SHADER: &[u8] =
    include_bytes!(concat!(env!("OUT_DIR"), "/foundation.vert.spv"));
