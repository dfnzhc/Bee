use crate::{Access, GraphResource, ModuleGraph, NodeId, ResourceId, Usage, WorkGraph, WorkId};
use bee_vulkan::{Frame, ToneMapper};

/// 将线性场景颜色映射到 sRGB 编码的视口颜色。
pub fn map_linear_color(color: [f32; 4], exposure_ev: f32) -> Result<[f32; 4], String> {
    if !exposure_ev.is_finite() || color.iter().any(|value| !value.is_finite()) {
        return Err("color and exposure must be finite".into());
    }
    let exposure = 2.0_f32.powf(exposure_ev.clamp(-126.0, 126.0));
    let mut result = [0.0; 4];
    for index in 0..3 {
        let linear = (color[index] * exposure).max(0.0);
        let mapped = 1.0 - 1.0 / (1.0 + linear);
        result[index] = if mapped <= 0.0031308 {
            12.92 * mapped
        } else {
            1.055 * mapped.powf(1.0 / 2.4) - 0.055
        };
    }
    result[3] = color[3].clamp(0.0, 1.0);
    Ok(result)
}

pub struct DisplayWork {
    source: NodeId,
    scene: ResourceId,
    mapped: ResourceId,
    viewport: ResourceId,
    exposure_ev: f32,
    tone_work: WorkId,
    output_work: WorkId,
}

impl DisplayWork {
    pub fn declare(
        modules: &ModuleGraph,
        graph: &mut WorkGraph,
        scene_outputs: &[(NodeId, ResourceId)],
        mapped: ResourceId,
        viewport: ResourceId,
    ) -> Result<Self, String> {
        let (source, exposure_ev) = modules.display_source()?;
        let matching: Vec<_> = scene_outputs
            .iter()
            .filter(|&&(node, _)| node == source)
            .collect();
        if matching.len() != 1 {
            return Err(
                "the connected scene module must have exactly one scene color resource".into(),
            );
        }
        let scene = matching[0].1;
        if [scene, mapped, viewport]
            .iter()
            .any(|id| id.0 >= graph.resources.len())
            || scene == mapped
            || scene == viewport
            || mapped == viewport
        {
            return Err("display work requires three distinct graph resources".into());
        }
        let tone_work = graph.add_work(
            "tone mapping",
            vec![
                Access {
                    resource: scene,
                    usage: Usage::ShaderRead,
                },
                Access {
                    resource: mapped,
                    usage: Usage::ColorAttachment,
                },
            ],
        )?;
        let output_work = graph.add_work(
            "viewport output",
            vec![
                Access {
                    resource: mapped,
                    usage: Usage::TransferRead,
                },
                Access {
                    resource: viewport,
                    usage: Usage::TransferWrite,
                },
            ],
        )?;
        Ok(Self {
            source,
            scene,
            mapped,
            viewport,
            exposure_ev,
            tone_work,
            output_work,
        })
    }

    pub fn source(&self) -> NodeId {
        self.source
    }

    /// # Safety
    /// 工作图须已建立相应屏障，并保证图像用途、设备和外部使用权有效。
    pub unsafe fn record(
        &self,
        work: WorkId,
        frame: &mut Frame,
        images: &[GraphResource],
        mapper: &ToneMapper,
    ) -> Result<bool, String> {
        if images.len() <= self.scene.0.max(self.mapped.0).max(self.viewport.0) {
            return Err("display work is missing an image".into());
        }
        if work == self.tone_work {
            frame
                .tone_map(
                    mapper,
                    images[self.scene.0].image()?,
                    images[self.mapped.0].image()?,
                    self.exposure_ev,
                )
                .map_err(|error| error.to_string())?;
        } else if work == self.output_work {
            frame
                .copy_graph_image(
                    images[self.mapped.0].image()?,
                    images[self.viewport.0].image()?,
                )
                .map_err(|error| error.to_string())?;
        } else {
            return Ok(false);
        }
        Ok(true)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{GraphResources, Module};
    use ash::vk;
    use bee_vulkan::{Backend, ValidationStatus};

    fn modules() -> (ModuleGraph, NodeId, NodeId, NodeId) {
        let mut graph = ModuleGraph::default();
        let raster = graph.add(Module::Rasterization);
        let path = graph.add(Module::PathTracing);
        let tone = graph.add(Module::ToneMapping { exposure_ev: 1.0 });
        let output = graph.add(Module::ViewportOutput);
        graph.connect(path, tone).unwrap();
        graph.connect(tone, output).unwrap();
        (graph, raster, path, tone)
    }

    #[test]
    fn maps_linear_color_and_follows_the_selected_source() {
        let black = map_linear_color([-1.0, 0.0, 0.0, 1.0], 0.0).unwrap();
        assert_eq!(black, [0.0, 0.0, 0.0, 1.0]);
        let value = map_linear_color([1.0, 1.0, 1.0, 0.5], 0.0).unwrap();
        assert!((value[0] - 0.735_357).abs() < 0.000_01);
        assert_eq!(value[3], 0.5);
        assert!(map_linear_color([1.0; 4], 1.0).unwrap()[0] > value[0]);
        assert!(map_linear_color([1.0; 4], f32::NAN).is_err());

        let (mut modules, raster, path, tone) = modules();
        let mut graph = WorkGraph::default();
        let raster_color = graph.add_resource(None);
        let path_color = graph.add_resource(None);
        let mapped = graph.add_resource(None);
        let viewport = graph.add_resource(None);
        let sources = [(raster, raster_color), (path, path_color)];
        assert_eq!(
            DisplayWork::declare(&modules, &mut graph, &sources, mapped, viewport)
                .unwrap()
                .source(),
            path
        );
        modules.connect(raster, tone).unwrap();
        let mut graph = WorkGraph::default();
        for _ in 0..4 {
            graph.add_resource(None);
        }
        assert_eq!(
            DisplayWork::declare(&modules, &mut graph, &sources, mapped, viewport)
                .unwrap()
                .source(),
            raster
        );
        assert!(DisplayWork::declare(&modules, &mut graph, &[], mapped, viewport).is_err());
    }

    #[test]
    #[ignore = "requires a Vulkan graphics device and validation layer"]
    fn displays_tone_mapped_linear_scene_color() {
        if std::env::var_os("BEE_DISPLAY_GPU_CHILD").is_none() {
            let output = std::process::Command::new(std::env::current_exe().unwrap())
                .args([
                    "--exact",
                    "display::tests::displays_tone_mapped_linear_scene_color",
                    "--ignored",
                    "--nocapture",
                ])
                .env("BEE_DISPLAY_GPU_CHILD", "1")
                .env("VK_LAYER_VALIDATE_SYNC", "1")
                .output()
                .unwrap();
            let stderr = String::from_utf8_lossy(&output.stderr);
            assert!(
                output.status.success(),
                "{}\n{stderr}",
                String::from_utf8_lossy(&output.stdout)
            );
            assert!(
                !stderr
                    .lines()
                    .any(|line| line.starts_with("Vulkan validation ") && line.contains("ERROR")),
                "{stderr}"
            );
            return;
        }
        let backend = Backend::new(true).unwrap();
        assert_eq!(backend.report.validation, ValidationStatus::Enabled);
        let mapper = backend.create_tone_mapper().unwrap();
        let image = |format, usage| unsafe {
            backend
                .create_graph_image(2, 2, format, usage, vk::ImageAspectFlags::COLOR)
                .unwrap()
        };
        let mut resources = GraphResources::default();
        let scene = resources.add_temporary(image(
            vk::Format::R32G32B32A32_SFLOAT,
            vk::ImageUsageFlags::TRANSFER_DST | vk::ImageUsageFlags::SAMPLED,
        ));
        let mapped = resources.add_temporary(image(
            vk::Format::R8G8B8A8_UNORM,
            vk::ImageUsageFlags::COLOR_ATTACHMENT | vk::ImageUsageFlags::TRANSFER_SRC,
        ));
        let viewport = unsafe {
            resources.add_external(
                image(
                    vk::Format::R8G8B8A8_UNORM,
                    vk::ImageUsageFlags::TRANSFER_DST | vk::ImageUsageFlags::TRANSFER_SRC,
                ),
                None,
            )
        };
        let (modules, _, path, _) = modules();
        let mut graph = resources.graph().unwrap();
        let display =
            DisplayWork::declare(&modules, &mut graph, &[(path, scene)], mapped, viewport).unwrap();
        let read = graph
            .add_work(
                "verify viewport",
                vec![Access {
                    resource: viewport,
                    usage: Usage::TransferRead,
                }],
            )
            .unwrap();
        let producer = graph
            .add_work(
                "produce linear scene",
                vec![Access {
                    resource: scene,
                    usage: Usage::TransferWrite,
                }],
            )
            .unwrap();
        let readback = backend.create_readback_buffer(16).unwrap();
        let linear = [0.0, 0.5, 2.0, 1.0];
        unsafe {
            resources
                .submit(&backend, &graph, |work, frame, images| {
                    if work == producer {
                        frame.record_raw(|device, command| {
                            let range = vk::ImageSubresourceRange::default()
                                .aspect_mask(vk::ImageAspectFlags::COLOR)
                                .level_count(1)
                                .layer_count(1);
                            device.cmd_clear_color_image(
                                command,
                                images[scene.index()].image().unwrap().raw(),
                                vk::ImageLayout::TRANSFER_DST_OPTIMAL,
                                &vk::ClearColorValue { float32: linear },
                                &[range],
                            );
                        });
                    } else if work == read {
                        frame
                            .copy_graph_image_to_readback(
                                images[viewport.index()].image()?,
                                &readback,
                            )
                            .map_err(|error| error.to_string())?;
                    } else {
                        assert!(display.record(work, frame, images, &mapper)?);
                    }
                    Ok(())
                })
                .unwrap();
        }
        let (_, state) = resources.release_external(viewport).unwrap();
        assert_eq!(state, Some(Usage::TransferRead));
        let expected = map_linear_color(linear, 1.0)
            .unwrap()
            .map(|value| (value * 255.0).round() as u8);
        for pixel in unsafe { readback.read().unwrap() }.as_chunks::<4>().0 {
            for (&actual, expected) in pixel.iter().zip(expected) {
                assert!(
                    actual.abs_diff(expected) <= 2,
                    "pixel {pixel:?}, expected {expected}"
                );
            }
        }
    }
}
