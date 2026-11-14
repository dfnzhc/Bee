//! 算法模块图和渲染工作。

mod display;
mod modules;
mod path;
mod raster;
mod resources;
mod work;

pub use display::{map_linear_color, DisplayWork};
pub use modules::{Module, ModuleGraph, NodeId, PortType, RenderFlow};
pub use path::PathRenderer;
pub use raster::{render_raster_snapshot, RasterWork};
pub use resources::GraphResources;
pub use work::{Access, Barrier, GraphResource, ResourceId, Usage, WorkGraph, WorkId, WorkPlan};
