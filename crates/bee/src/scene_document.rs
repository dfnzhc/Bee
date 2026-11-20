use crate::project::SceneContent;
use bee_renderer::{Module, ModuleGraph, PreparedFlow, RenderFlow};
use bee_scene::{Camera, DisplayParameters, RenderParameters, Scene, SceneState};
use serde::{Deserialize, Serialize};

#[cfg(test)]
mod tests;

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub struct SceneDocument {
    pub scene: Scene,
    pub start_camera: Camera,
    pub render: RenderParameters,
    pub applied_flow: ModuleGraph,
    pub draft_flow: ModuleGraph,
}

pub struct PreparedScene {
    pub state: SceneState,
    pub flow: RenderFlow,
    pub preview: PreparedFlow,
    pub preview_reason: Option<String>,
}

impl SceneDocument {
    pub fn blank() -> Self {
        let mut graph = ModuleGraph::default();
        let raster = graph.add(Module::Rasterization);
        graph.add(Module::PathTracing);
        let tone = graph.add(Module::ToneMapping { exposure_ev: 0.0 });
        let output = graph.add(Module::ViewportOutput);
        graph.connect(raster, tone).unwrap();
        graph.connect(tone, output).unwrap();
        Self {
            scene: Scene::blank(),
            start_camera: Camera::default(),
            render: RenderParameters::default(),
            draft_flow: graph.clone(),
            applied_flow: graph,
        }
    }

    pub fn builtin() -> Self {
        Self {
            scene: Scene::builtin(),
            ..Self::blank()
        }
    }

    pub fn prepare(&self, path_unavailable: Option<String>) -> Result<PreparedScene, String> {
        self.validate_scene()?;
        let flow = RenderFlow::from_saved(self.applied_flow.clone(), self.draft_flow.clone())?;
        let (source, _) = self.applied_flow.display_source()?;
        let preview_reason = path_unavailable
            .filter(|_| self.applied_flow.module(source) == Some(Module::PathTracing));
        let preview = if preview_reason.is_some() {
            // 只替换执行时的源节点，保留文档的生效流程、草稿与曝光。
            let mut raster = self.applied_flow.clone();
            *raster.module_mut(source).unwrap() = Module::Rasterization;
            PreparedFlow::prepare(&raster, None)?
        } else {
            PreparedFlow::prepare(&self.applied_flow, None)?
        };
        let state = SceneState::new(
            self.scene.clone(),
            self.start_camera,
            self.render,
            DisplayParameters {
                exposure_ev: preview.exposure_ev,
            },
        );
        Ok(PreparedScene {
            state,
            flow,
            preview,
            preview_reason,
        })
    }

    fn validate_scene(&self) -> Result<(), String> {
        self.scene.validate()?;
        if !self.start_camera.is_valid() {
            return Err("invalid start camera".into());
        }
        if !self.render.is_valid() {
            return Err("invalid render parameters".into());
        }
        Ok(())
    }
}

impl SceneContent for SceneDocument {
    fn validate(&self) -> Result<(), String> {
        self.validate_scene()?;
        self.applied_flow
            .validate()
            .map_err(|error| format!("invalid applied flow: {error}"))?;
        self.draft_flow
            .validate_structure()
            .map_err(|error| format!("invalid flow draft: {error}"))
    }
}
