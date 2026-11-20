use ash::vk;
use bee_vulkan::profile;
use bee_vulkan::{Buffer, Frame, GraphImage};

#[derive(Clone)]
pub enum GraphResource {
    Image(GraphImage),
    Buffer(Buffer),
}

impl From<GraphImage> for GraphResource {
    fn from(image: GraphImage) -> Self {
        Self::Image(image)
    }
}

impl From<Buffer> for GraphResource {
    fn from(buffer: Buffer) -> Self {
        Self::Buffer(buffer)
    }
}

impl GraphResource {
    pub fn image(&self) -> Result<&GraphImage, String> {
        match self {
            Self::Image(image) => Ok(image),
            _ => Err("graph resource is not an image".into()),
        }
    }

    pub fn buffer(&self) -> Result<&Buffer, String> {
        match self {
            Self::Buffer(buffer) => Ok(buffer),
            _ => Err("graph resource is not a buffer".into()),
        }
    }

    fn same_handle(&self, other: &Self) -> bool {
        match (self, other) {
            (Self::Image(a), Self::Image(b)) => a.raw() == b.raw(),
            (Self::Buffer(a), Self::Buffer(b)) => a.raw() == b.raw(),
            _ => false,
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct ResourceId(pub(crate) usize);

impl ResourceId {
    pub fn index(self) -> usize {
        self.0
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct WorkId(usize);

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Usage {
    TransferRead,
    TransferWrite,
    ColorAttachment,
    DepthAttachment,
    ShaderRead,
    ShaderWrite,
    ShaderReadWrite,
}

impl Usage {
    fn reads(self) -> bool {
        matches!(
            self,
            Self::TransferRead | Self::ShaderRead | Self::ShaderReadWrite
        )
    }

    fn writes(self) -> bool {
        !matches!(self, Self::TransferRead | Self::ShaderRead)
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Access {
    pub resource: ResourceId,
    pub usage: Usage,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Barrier {
    pub resource: ResourceId,
    pub before: Option<Usage>,
    pub after: Usage,
}

#[derive(Clone, Debug)]
struct Work {
    name: String,
    accesses: Vec<Access>,
}

#[derive(Clone, Debug, Default)]
pub struct WorkGraph {
    pub(crate) resources: Vec<Option<Usage>>,
    pub(crate) binding: Option<(u64, u64)>,
    work: Vec<Work>,
    dependencies: Vec<(WorkId, WorkId)>,
}

#[derive(Clone, Debug)]
pub struct WorkPlan {
    order: Vec<WorkId>,
    work: Vec<Work>,
    barriers: Vec<Vec<Barrier>>,
    resource_count: usize,
    initial_usages: Vec<Option<Usage>>,
}

impl WorkPlan {
    pub fn order(&self) -> &[WorkId] {
        &self.order
    }

    pub fn name(&self, work: WorkId) -> &str {
        &self.work[work.0].name
    }

    pub fn accesses(&self, work: WorkId) -> &[Access] {
        &self.work[work.0].accesses
    }

    pub fn barriers(&self, work: WorkId) -> &[Barrier] {
        &self.barriers[work.0]
    }

    /// # Safety
    /// 调用方须保证初始状态真实、跨帧及外部访问已经同步，且回调只按声明录制工作。
    /// 如果回调失败，必须丢弃此帧，不得提交部分录制的命令。
    pub unsafe fn record(
        &self,
        frame: &mut Frame,
        resources: &[GraphResource],
        record_work: impl FnMut(WorkId, &mut Frame) -> Result<(), String>,
    ) -> Result<(), String> {
        self.record_bound(
            frame,
            resources,
            &mut self.initial_usages.clone(),
            record_work,
        )
    }

    pub(crate) unsafe fn record_bound(
        &self,
        frame: &mut Frame,
        resources: &[GraphResource],
        usages: &mut [Option<Usage>],
        mut record_work: impl FnMut(WorkId, &mut Frame) -> Result<(), String>,
    ) -> Result<(), String> {
        if resources.len() != self.resource_count || usages.len() != self.resource_count {
            return Err("incorrect number of graph resources".into());
        }
        for (index, resource) in resources.iter().enumerate() {
            if resources[..index]
                .iter()
                .any(|other| other.same_handle(resource))
            {
                return Err("one Vulkan object cannot represent multiple graph resources".into());
            }
        }
        for work in &self.work {
            for access in &work.accesses {
                match &resources[access.resource.0] {
                    GraphResource::Image(image) => validate_image(image, access.usage)?,
                    GraphResource::Buffer(buffer) => validate_buffer(buffer, access.usage)?,
                }
            }
        }
        for &work in &self.order {
            for &Access { resource, usage } in &self.work[work.0].accesses {
                let state = &mut usages[resource.0];
                if state.is_none() && usage.reads() {
                    return Err(format!(
                        "work {:?} reads undefined resource {resource:?}",
                        self.name(work)
                    ));
                }
                let before = state.map(vulkan_usage).unwrap_or((
                    vk::PipelineStageFlags::TOP_OF_PIPE,
                    vk::AccessFlags::empty(),
                    vk::ImageLayout::UNDEFINED,
                ));
                let after = vulkan_usage(usage);
                match &resources[resource.0] {
                    GraphResource::Image(image) => frame.graph_image_barrier(image, before, after),
                    GraphResource::Buffer(buffer) => {
                        frame.graph_buffer_barrier(buffer, (before.0, before.1), (after.0, after.1))
                    }
                }
                .map_err(|error| error.to_string())?;
                *state = Some(usage);
            }
            let timestamp = match self.name(work) {
                "rasterization" => frame.profile_begin("gpu_raster_us"),
                "path tracing" => frame.profile_begin("gpu_trace_us"),
                "tone mapping" => frame.profile_begin("gpu_tone_us"),
                "viewport output" => frame.profile_begin("gpu_copy_us"),
                "read viewport" => frame.profile_begin("gpu_read_us"),
                _ => None,
            };
            record_work(work, frame)?;
            frame.profile_end(timestamp);
        }
        Ok(())
    }
}

impl WorkGraph {
    pub fn add_resource(&mut self, initial_usage: Option<Usage>) -> ResourceId {
        let id = ResourceId(self.resources.len());
        self.resources.push(initial_usage);
        id
    }

    pub fn add_work(
        &mut self,
        name: impl Into<String>,
        accesses: Vec<Access>,
    ) -> Result<WorkId, String> {
        for (index, access) in accesses.iter().enumerate() {
            if access.resource.0 >= self.resources.len() {
                return Err("work refers to an unknown resource".into());
            }
            if accesses[..index]
                .iter()
                .any(|other| other.resource == access.resource)
            {
                return Err("work declares the same resource more than once".into());
            }
        }
        let id = WorkId(self.work.len());
        self.work.push(Work {
            name: name.into(),
            accesses,
        });
        Ok(id)
    }

    pub fn depends_on(&mut self, work: WorkId, dependency: WorkId) -> Result<(), String> {
        if work.0 >= self.work.len() || dependency.0 >= self.work.len() {
            return Err("dependency refers to unknown work".into());
        }
        self.dependencies.push((dependency, work));
        Ok(())
    }

    pub fn compile(&self) -> Result<WorkPlan, String> {
        profile::record("graph_compile", 1.0);
        profile::span("compile_us", || self.compile_plan())
    }

    fn compile_plan(&self) -> Result<WorkPlan, String> {
        let count = self.work.len();
        let mut edges = vec![vec![false; count]; count];
        for &(before, after) in &self.dependencies {
            edges[before.0][after.0] = true;
        }
        let mut reachable = closure(&edges)?;

        // 先建立无初始内容资源的依赖，再处理可能读取上一帧的资源。
        for initially_defined in [false, true] {
            for resource in 0..self.resources.len() {
                if self.resources[resource].is_some() != initially_defined {
                    continue;
                }
                let writers = self.users(ResourceId(resource), Usage::writes);
                if writers.len() == 1 {
                    let writer = writers[0];
                    for reader in self.users(ResourceId(resource), Usage::reads) {
                        if writer != reader
                            && !reachable[reader][writer]
                            && !reachable[writer][reader]
                        {
                            edges[writer][reader] = true;
                            reachable = closure(&edges)?;
                        }
                    }
                }
            }
        }

        // 多个写入者必须已有可传递的明确顺序。
        for resource in 0..self.resources.len() {
            let id = ResourceId(resource);
            let writers = self.users(id, Usage::writes);
            for (index, &a) in writers.iter().enumerate() {
                for &b in &writers[index + 1..] {
                    if !reachable[a][b] && !reachable[b][a] {
                        return Err(format!("resource {id:?} has conflicting unordered writes"));
                    }
                }
            }
            if writers.len() > 1 {
                for reader in self.users(id, Usage::reads) {
                    if writers.iter().any(|&writer| {
                        writer != reader && !reachable[writer][reader] && !reachable[reader][writer]
                    }) {
                        return Err(format!(
                            "resource {id:?} has an ambiguous read between writes"
                        ));
                    }
                }
            }
        }

        let mut order = Vec::with_capacity(count);
        let mut selected = vec![false; count];
        while order.len() < count {
            let next = (0..count)
                .find(|&candidate| {
                    !selected[candidate]
                        && (0..count).all(|before| !edges[before][candidate] || selected[before])
                })
                .ok_or("work graph contains a dependency cycle")?;
            selected[next] = true;
            order.push(WorkId(next));
        }

        // 按实际执行顺序检查每次读取是否已有内容。
        let mut defined: Vec<bool> = self.resources.iter().map(Option::is_some).collect();
        let mut previous = self.resources.clone();
        let mut barriers = vec![Vec::new(); count];
        for &work in &order {
            for access in &self.work[work.0].accesses {
                if access.usage.reads() && !defined[access.resource.0] {
                    return Err(format!(
                        "work {:?} reads undefined resource {:?}",
                        self.work[work.0].name, access.resource
                    ));
                }
                if access.usage.writes() {
                    defined[access.resource.0] = true;
                }
                barriers[work.0].push(Barrier {
                    resource: access.resource,
                    before: previous[access.resource.0],
                    after: access.usage,
                });
                previous[access.resource.0] = Some(access.usage);
            }
        }
        Ok(WorkPlan {
            order,
            work: self.work.clone(),
            barriers,
            resource_count: self.resources.len(),
            initial_usages: self.resources.clone(),
        })
    }

    fn users(&self, resource: ResourceId, predicate: fn(Usage) -> bool) -> Vec<usize> {
        self.work
            .iter()
            .enumerate()
            .filter_map(|(index, work)| {
                work.accesses
                    .iter()
                    .any(|access| access.resource == resource && predicate(access.usage))
                    .then_some(index)
            })
            .collect()
    }
}

fn vulkan_usage(usage: Usage) -> (vk::PipelineStageFlags, vk::AccessFlags, vk::ImageLayout) {
    match usage {
        Usage::TransferRead => (
            vk::PipelineStageFlags::TRANSFER,
            vk::AccessFlags::TRANSFER_READ,
            vk::ImageLayout::TRANSFER_SRC_OPTIMAL,
        ),
        Usage::TransferWrite => (
            vk::PipelineStageFlags::TRANSFER,
            vk::AccessFlags::TRANSFER_WRITE,
            vk::ImageLayout::TRANSFER_DST_OPTIMAL,
        ),
        Usage::ColorAttachment => (
            vk::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT,
            vk::AccessFlags::COLOR_ATTACHMENT_WRITE,
            vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
        ),
        Usage::DepthAttachment => (
            vk::PipelineStageFlags::EARLY_FRAGMENT_TESTS
                | vk::PipelineStageFlags::LATE_FRAGMENT_TESTS,
            vk::AccessFlags::DEPTH_STENCIL_ATTACHMENT_READ
                | vk::AccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE,
            vk::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        ),
        Usage::ShaderRead => (
            vk::PipelineStageFlags::ALL_COMMANDS,
            vk::AccessFlags::SHADER_READ,
            vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
        ),
        Usage::ShaderWrite => (
            vk::PipelineStageFlags::ALL_COMMANDS,
            vk::AccessFlags::SHADER_WRITE,
            vk::ImageLayout::GENERAL,
        ),
        Usage::ShaderReadWrite => (
            vk::PipelineStageFlags::ALL_COMMANDS,
            vk::AccessFlags::SHADER_READ | vk::AccessFlags::SHADER_WRITE,
            vk::ImageLayout::GENERAL,
        ),
    }
}

fn validate_image(image: &GraphImage, usage: Usage) -> Result<(), String> {
    let (required, aspect) = match usage {
        Usage::TransferRead => (vk::ImageUsageFlags::TRANSFER_SRC, image.aspect()),
        Usage::TransferWrite => (vk::ImageUsageFlags::TRANSFER_DST, image.aspect()),
        Usage::ColorAttachment => (
            vk::ImageUsageFlags::COLOR_ATTACHMENT,
            vk::ImageAspectFlags::COLOR,
        ),
        Usage::DepthAttachment => (
            vk::ImageUsageFlags::DEPTH_STENCIL_ATTACHMENT,
            vk::ImageAspectFlags::DEPTH,
        ),
        Usage::ShaderRead => (vk::ImageUsageFlags::SAMPLED, image.aspect()),
        Usage::ShaderWrite | Usage::ShaderReadWrite => {
            (vk::ImageUsageFlags::STORAGE, image.aspect())
        }
    };
    if !image.usage().contains(required) || image.aspect() != aspect {
        return Err("image does not support its declared graph usage".into());
    }
    Ok(())
}

fn validate_buffer(buffer: &Buffer, usage: Usage) -> Result<(), String> {
    let required = match usage {
        Usage::TransferRead => vk::BufferUsageFlags::TRANSFER_SRC,
        Usage::TransferWrite => vk::BufferUsageFlags::TRANSFER_DST,
        Usage::ShaderRead | Usage::ShaderWrite | Usage::ShaderReadWrite => {
            vk::BufferUsageFlags::STORAGE_BUFFER
        }
        Usage::ColorAttachment | Usage::DepthAttachment => {
            return Err("a buffer cannot be an image attachment".into())
        }
    };
    if !buffer.usage().contains(required) {
        return Err("buffer does not support its declared graph usage".into());
    }
    Ok(())
}

fn closure(edges: &[Vec<bool>]) -> Result<Vec<Vec<bool>>, String> {
    let mut reachable = edges.to_vec();
    for via in 0..edges.len() {
        for from in 0..edges.len() {
            for to in 0..edges.len() {
                reachable[from][to] |= reachable[from][via] && reachable[via][to];
            }
        }
    }
    if (0..edges.len()).any(|index| reachable[index][index]) {
        return Err("work graph contains a dependency cycle".into());
    }
    Ok(reachable)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn access(resource: ResourceId, usage: Usage) -> Vec<Access> {
        vec![Access { resource, usage }]
    }

    #[test]
    fn orders_producers_and_rejects_undefined_reads() {
        let mut graph = WorkGraph::default();
        let image = graph.add_resource(None);
        let read = graph
            .add_work("read", access(image, Usage::ShaderRead))
            .unwrap();
        assert!(graph.compile().unwrap_err().contains("undefined"));
        let write = graph
            .add_work("write", access(image, Usage::ColorAttachment))
            .unwrap();
        let plan = graph.compile().unwrap();
        assert_eq!(plan.order(), &[write, read]);
        assert_eq!(plan.name(plan.order()[0]), "write");
        assert_eq!(plan.accesses(read), &access(image, Usage::ShaderRead));
        assert_eq!(
            plan.barriers(write),
            &[Barrier {
                resource: image,
                before: None,
                after: Usage::ColorAttachment
            }]
        );
        assert_eq!(
            plan.barriers(read),
            &[Barrier {
                resource: image,
                before: Some(Usage::ColorAttachment),
                after: Usage::ShaderRead
            }]
        );
    }

    #[test]
    fn rejects_unordered_writes_and_ambiguous_reads() {
        let mut graph = WorkGraph::default();
        let image = graph.add_resource(None);
        let first = graph
            .add_work("first", access(image, Usage::TransferWrite))
            .unwrap();
        let second = graph
            .add_work("second", access(image, Usage::ShaderWrite))
            .unwrap();
        assert!(graph.compile().unwrap_err().contains("unordered writes"));
        graph.depends_on(second, first).unwrap();
        let read = graph
            .add_work("read", access(image, Usage::ShaderRead))
            .unwrap();
        assert!(graph.compile().unwrap_err().contains("ambiguous read"));
        graph.depends_on(read, second).unwrap();
        assert_eq!(graph.compile().unwrap().order(), &[first, second, read]);
        graph.depends_on(first, read).unwrap();
        assert!(graph.compile().unwrap_err().contains("cycle"));
    }

    #[test]
    fn explicit_read_before_write_requires_initial_content() {
        let mut graph = WorkGraph::default();
        let image = graph.add_resource(None);
        let read = graph
            .add_work("read old", access(image, Usage::ShaderRead))
            .unwrap();
        let write = graph
            .add_work("write new", access(image, Usage::ShaderWrite))
            .unwrap();
        graph.depends_on(write, read).unwrap();
        assert!(graph.compile().unwrap_err().contains("undefined"));
        graph.resources[image.0] = Some(Usage::ShaderRead);
        assert_eq!(graph.compile().unwrap().order(), &[read, write]);
    }

    #[test]
    fn inferred_dependency_preserves_previous_history() {
        let mut graph = WorkGraph::default();
        let history = graph.add_resource(Some(Usage::ShaderRead));
        let temporary = graph.add_resource(None);
        let update = graph
            .add_work(
                "update history",
                vec![
                    Access {
                        resource: temporary,
                        usage: Usage::ShaderRead,
                    },
                    Access {
                        resource: history,
                        usage: Usage::ShaderWrite,
                    },
                ],
            )
            .unwrap();
        let prepare = graph
            .add_work(
                "read previous history",
                vec![
                    Access {
                        resource: history,
                        usage: Usage::ShaderRead,
                    },
                    Access {
                        resource: temporary,
                        usage: Usage::ShaderWrite,
                    },
                ],
            )
            .unwrap();
        assert_eq!(graph.compile().unwrap().order(), &[prepare, update]);
    }

    #[test]
    #[ignore = "requires a Vulkan graphics device and validation layer"]
    fn graph_barriers_synchronize_real_gpu_work() {
        if std::env::var_os("BEE_GRAPH_GPU_CHILD").is_none() {
            let output = std::process::Command::new(std::env::current_exe().unwrap())
                .args([
                    "--exact",
                    "work::tests::graph_barriers_synchronize_real_gpu_work",
                    "--ignored",
                    "--nocapture",
                ])
                .env("BEE_GRAPH_GPU_CHILD", "1")
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

        use bee_vulkan::{Backend, ValidationStatus};
        let backend = Backend::new(true).unwrap();
        assert_eq!(backend.report.validation, ValidationStatus::Enabled);
        let image = unsafe {
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
        let mut graph = WorkGraph::default();
        let resource = graph.add_resource(None);
        let read = graph
            .add_work("read", access(resource, Usage::TransferRead))
            .unwrap();
        let write = graph
            .add_work("write", access(resource, Usage::TransferWrite))
            .unwrap();
        let plan = graph.compile().unwrap();
        assert_eq!(plan.order(), &[write, read]);
        let mut frame = backend.begin_frame().unwrap();
        unsafe {
            plan.record(&mut frame, &[image.clone().into()], |work, frame| {
                if work == write {
                    frame.record_raw(|device, command| {
                        let color = vk::ClearColorValue {
                            float32: [0.25, 0.5, 0.75, 1.0],
                        };
                        let range = vk::ImageSubresourceRange::default()
                            .aspect_mask(vk::ImageAspectFlags::COLOR)
                            .level_count(1)
                            .layer_count(1);
                        device.cmd_clear_color_image(
                            command,
                            image.raw(),
                            vk::ImageLayout::TRANSFER_DST_OPTIMAL,
                            &color,
                            &[range],
                        );
                    });
                } else if work == read {
                    frame
                        .copy_graph_image_to_readback(&image, &readback)
                        .map_err(|error| error.to_string())?;
                }
                Ok(())
            })
            .unwrap();
        }
        frame.submit().unwrap().wait().unwrap();
        let bytes = unsafe { readback.read().unwrap() };
        for (actual, expected) in bytes.into_iter().zip([64_u8, 128, 191, 255]) {
            assert!(actual.abs_diff(expected) <= 1);
        }

        let buffer = backend
            .create_graph_buffer(
                4,
                vk::BufferUsageFlags::TRANSFER_SRC | vk::BufferUsageFlags::TRANSFER_DST,
            )
            .unwrap();
        let readback = backend.create_readback_buffer(4).unwrap();
        let mut graph = WorkGraph::default();
        let resource = graph.add_resource(None);
        let read = graph
            .add_work("read buffer", access(resource, Usage::TransferRead))
            .unwrap();
        let write = graph
            .add_work("write buffer", access(resource, Usage::TransferWrite))
            .unwrap();
        let plan = graph.compile().unwrap();
        assert_eq!(plan.order(), &[write, read]);
        let mut frame = backend.begin_frame().unwrap();
        unsafe {
            plan.record(&mut frame, &[buffer.clone().into()], |work, frame| {
                if work == write {
                    frame.record_raw(|device, command| {
                        device.cmd_fill_buffer(command, buffer.raw(), 0, 4, 0x1234_5678)
                    });
                } else if work == read {
                    frame
                        .copy_graph_buffer_to_readback(&buffer, &readback)
                        .map_err(|error| error.to_string())?;
                }
                Ok(())
            })
            .unwrap();
        }
        frame.submit().unwrap().wait().unwrap();
        assert_eq!(
            unsafe { readback.read().unwrap() },
            0x1234_5678_u32.to_ne_bytes()
        );
    }
}
