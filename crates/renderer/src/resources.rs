use crate::{GraphResource, ResourceId, Usage, WorkGraph, WorkId};
use bee_vulkan::{Backend, Frame, Submission};
use std::sync::atomic::{AtomicU64, Ordering};

static NEXT_RESOURCES_ID: AtomicU64 = AtomicU64::new(1);

#[derive(Clone, Copy, PartialEq, Eq)]
enum Lifetime {
    Temporary,
    History,
    External,
}

struct Slot {
    lifetime: Lifetime,
    resource: Option<GraphResource>,
    usage: Option<Usage>,
}

pub struct GraphResources {
    id: u64,
    revision: u64,
    slots: Vec<Slot>,
    pending: Option<Submission>,
    poisoned: bool,
}

impl Default for GraphResources {
    fn default() -> Self {
        Self {
            id: NEXT_RESOURCES_ID.fetch_add(1, Ordering::Relaxed),
            revision: 0,
            slots: Vec::new(),
            pending: None,
            poisoned: false,
        }
    }
}

impl GraphResources {
    pub fn add_temporary(&mut self, resource: impl Into<GraphResource>) -> ResourceId {
        self.add(Lifetime::Temporary, resource.into(), None)
    }

    pub fn add_history(&mut self, resource: impl Into<GraphResource>) -> ResourceId {
        self.add(Lifetime::History, resource.into(), None)
    }

    /// # Safety
    /// 外部使用方必须完成访问并交出资源，usage 须匹配真实状态，资源须归图形队列族所有。
    pub unsafe fn add_external(
        &mut self,
        resource: impl Into<GraphResource>,
        usage: Option<Usage>,
    ) -> ResourceId {
        self.add(Lifetime::External, resource.into(), usage)
    }

    fn add(
        &mut self,
        lifetime: Lifetime,
        resource: GraphResource,
        usage: Option<Usage>,
    ) -> ResourceId {
        let id = ResourceId(self.slots.len());
        self.slots.push(Slot {
            lifetime,
            resource: Some(resource),
            usage,
        });
        self.revision = self.revision.wrapping_add(1);
        id
    }

    pub fn set_temporary(
        &mut self,
        id: ResourceId,
        resource: impl Into<GraphResource>,
    ) -> Result<(), String> {
        self.replace(id, resource.into(), Lifetime::Temporary)
    }

    pub fn replace_history(
        &mut self,
        id: ResourceId,
        resource: impl Into<GraphResource>,
    ) -> Result<(), String> {
        self.replace(id, resource.into(), Lifetime::History)
    }

    fn replace(
        &mut self,
        id: ResourceId,
        resource: GraphResource,
        lifetime: Lifetime,
    ) -> Result<(), String> {
        let slot = self.slots.get_mut(id.0).ok_or("unknown graph resource")?;
        if slot.lifetime != lifetime {
            return Err("resource has a different lifetime".into());
        }
        // 旧资源仍由在途帧持有；新资源从未定义内容开始。
        slot.resource = Some(resource);
        slot.usage = None;
        self.revision = self.revision.wrapping_add(1);
        Ok(())
    }

    /// # Safety
    /// 外部使用方必须完成访问并交还资源，usage 须匹配真实状态，资源须归图形队列族所有。
    pub unsafe fn acquire_external(
        &mut self,
        id: ResourceId,
        resource: impl Into<GraphResource>,
        usage: Option<Usage>,
    ) -> Result<(), String> {
        let slot = self.slots.get_mut(id.0).ok_or("unknown graph resource")?;
        if slot.lifetime != Lifetime::External || slot.resource.is_some() {
            return Err("external resource is not available for acquisition".into());
        }
        slot.resource = Some(resource.into());
        slot.usage = usage;
        self.revision = self.revision.wrapping_add(1);
        Ok(())
    }

    pub fn release_external(
        &mut self,
        id: ResourceId,
    ) -> Result<(GraphResource, Option<Usage>), String> {
        self.wait()?;
        let slot = self.slots.get_mut(id.0).ok_or("unknown graph resource")?;
        if slot.lifetime != Lifetime::External {
            return Err("resource is not external".into());
        }
        let resource = slot
            .resource
            .take()
            .ok_or("external resource was already released")?;
        self.revision = self.revision.wrapping_add(1);
        Ok((resource, slot.usage))
    }

    pub fn wait(&mut self) -> Result<(), String> {
        if self.poisoned {
            return Err("previous graph submission did not complete safely".into());
        }
        if let Some(submission) = self.pending.take() {
            if let Err(error) = submission.wait() {
                self.poisoned = true;
                return Err(error.to_string());
            }
        }
        Ok(())
    }

    pub fn graph(&mut self) -> Result<WorkGraph, String> {
        // 当前使用单队列并等待上一提交，避免旧帧与新的历史资源竞争。
        self.wait()?;
        if self.slots.iter().any(|slot| slot.resource.is_none()) {
            return Err("a graph resource has not been supplied or acquired".into());
        }
        let mut graph = WorkGraph::default();
        for slot in &self.slots {
            graph.add_resource(slot.usage);
        }
        graph.binding = Some((self.id, self.revision));
        Ok(graph)
    }

    /// # Safety
    /// 回调只能访问声明的资源和用途，并须满足其 Vulkan 命令及其他外部对象的生命周期要求。
    pub unsafe fn submit(
        &mut self,
        backend: &Backend,
        graph: &WorkGraph,
        mut record: impl FnMut(WorkId, &mut Frame, &[GraphResource]) -> Result<(), String>,
    ) -> Result<(), String> {
        if self.poisoned
            || self.pending.is_some()
            || graph.binding != Some((self.id, self.revision))
            || graph.resources.len() != self.slots.len()
        {
            return Err("graph resources changed or a submission is still pending".into());
        }
        let plan = graph.compile()?;
        let resources: Vec<GraphResource> = self
            .slots
            .iter()
            .map(|slot| slot.resource.clone().ok_or("graph resource is unavailable"))
            .collect::<Result<_, _>>()?;
        let mut frame = backend.begin_frame().map_err(|error| error.to_string())?;
        plan.record(&mut frame, &resources, |work, frame| {
            record(work, frame, &resources)
        })?;
        let submission = frame.submit().map_err(|error| error.to_string())?;
        for (slot, &usage) in self.slots.iter_mut().zip(plan.final_usages()) {
            slot.usage = usage;
            if slot.lifetime == Lifetime::Temporary {
                slot.resource = None;
                slot.usage = None;
            }
        }
        self.pending = Some(submission);
        self.revision = self.revision.wrapping_add(1);
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{Access, Usage};
    use ash::vk;
    use bee_vulkan::{GraphImage, ValidationStatus};

    #[test]
    #[ignore = "requires a Vulkan graphics device and validation layer"]
    fn retains_replaces_and_hands_off_images() {
        if std::env::var_os("BEE_RESOURCES_GPU_CHILD").is_none() {
            let output = std::process::Command::new(std::env::current_exe().unwrap())
                .args([
                    "--exact",
                    "resources::tests::retains_replaces_and_hands_off_images",
                    "--ignored",
                    "--nocapture",
                ])
                .env("BEE_RESOURCES_GPU_CHILD", "1")
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
        let new_image = || unsafe {
            backend
                .create_graph_image(
                    1,
                    1,
                    vk::Format::R8G8B8A8_UNORM,
                    vk::ImageUsageFlags::TRANSFER_SRC | vk::ImageUsageFlags::TRANSFER_DST,
                    vk::ImageAspectFlags::COLOR,
                )
                .unwrap()
        };
        let readback = backend.create_readback_buffer(4).unwrap();
        let mut resources = GraphResources::default();
        let history = resources.add_history(new_image());
        let temporary = resources.add_temporary(new_image());
        let external = unsafe { resources.add_external(new_image(), None) };
        let mut graph = resources.graph().unwrap();
        let initialize = graph
            .add_work(
                "initialize history",
                vec![Access {
                    resource: history,
                    usage: Usage::TransferWrite,
                }],
            )
            .unwrap();
        unsafe {
            resources
                .submit(&backend, &graph, |work, frame, images| {
                    assert_eq!(work, initialize);
                    clear(
                        frame,
                        images[history.index()].image()?,
                        [1.0, 0.0, 0.0, 1.0],
                    );
                    Ok(())
                })
                .unwrap();
        }
        // 临时图像每帧重新提供，历史图像的内容则保留。
        assert!(resources.graph().unwrap_err().contains("not been supplied"));
        resources.set_temporary(temporary, new_image()).unwrap();
        let mut graph = resources.graph().unwrap();
        let read = graph
            .add_work(
                "read history",
                vec![Access {
                    resource: history,
                    usage: Usage::TransferRead,
                }],
            )
            .unwrap();
        unsafe {
            resources
                .submit(&backend, &graph, |work, frame, images| {
                    assert_eq!(work, read);
                    frame
                        .copy_graph_image_to_readback(images[history.index()].image()?, &readback)
                        .map_err(|error| error.to_string())
                })
                .unwrap();
        }
        // 提交尚未回收时替换历史，旧图像仍须支撑此前的读回。
        resources.replace_history(history, new_image()).unwrap();
        resources.wait().unwrap();
        assert_eq!(unsafe { readback.read().unwrap() }, [255, 0, 0, 255]);

        resources.set_temporary(temporary, new_image()).unwrap();
        let mut graph = resources.graph().unwrap();
        graph
            .add_work(
                "read replaced history",
                vec![Access {
                    resource: history,
                    usage: Usage::TransferRead,
                }],
            )
            .unwrap();
        assert!(graph.compile().unwrap_err().contains("undefined"));

        let mut graph = resources.graph().unwrap();
        let write = graph
            .add_work(
                "write external",
                vec![Access {
                    resource: external,
                    usage: Usage::TransferWrite,
                }],
            )
            .unwrap();
        unsafe {
            resources
                .submit(&backend, &graph, |work, frame, images| {
                    assert_eq!(work, write);
                    clear(
                        frame,
                        images[external.index()].image()?,
                        [0.0, 1.0, 0.0, 1.0],
                    );
                    Ok(())
                })
                .unwrap();
        }
        let (external_image, state) = resources.release_external(external).unwrap();
        assert_eq!(state, Some(Usage::TransferWrite));
        assert!(resources.graph().is_err());
        unsafe {
            resources
                .acquire_external(external, external_image, state)
                .unwrap()
        };
        resources.set_temporary(temporary, new_image()).unwrap();
        let mut graph = resources.graph().unwrap();
        let read = graph
            .add_work(
                "read reacquired external",
                vec![Access {
                    resource: external,
                    usage: Usage::TransferRead,
                }],
            )
            .unwrap();
        unsafe {
            resources
                .submit(&backend, &graph, |work, frame, images| {
                    assert_eq!(work, read);
                    frame
                        .copy_graph_image_to_readback(images[external.index()].image()?, &readback)
                        .map_err(|error| error.to_string())
                })
                .unwrap();
        }
        let _ = resources.release_external(external).unwrap();
        assert_eq!(unsafe { readback.read().unwrap() }, [0, 255, 0, 255]);
    }

    unsafe fn clear(frame: &mut Frame, image: &GraphImage, color: [f32; 4]) {
        frame.record_raw(|device, command| {
            let range = vk::ImageSubresourceRange::default()
                .aspect_mask(vk::ImageAspectFlags::COLOR)
                .level_count(1)
                .layer_count(1);
            device.cmd_clear_color_image(
                command,
                image.raw(),
                vk::ImageLayout::TRANSFER_DST_OPTIMAL,
                &vk::ClearColorValue { float32: color },
                &[range],
            );
        });
    }
}
