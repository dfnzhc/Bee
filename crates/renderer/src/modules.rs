#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum PortType {
    LinearSceneColor,
    DisplayColor,
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum Module {
    Rasterization,
    PathTracing,
    ToneMapping { exposure_ev: f32 },
    ViewportOutput,
}

impl Module {
    pub fn input_type(self) -> Option<PortType> {
        match self {
            Self::ToneMapping { .. } => Some(PortType::LinearSceneColor),
            Self::ViewportOutput => Some(PortType::DisplayColor),
            _ => None,
        }
    }

    pub fn output_type(self) -> Option<PortType> {
        match self {
            Self::Rasterization | Self::PathTracing => Some(PortType::LinearSceneColor),
            Self::ToneMapping { .. } => Some(PortType::DisplayColor),
            Self::ViewportOutput => None,
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct NodeId(usize);

impl NodeId {
    pub fn index(self) -> usize {
        self.0
    }
}

#[derive(Clone, Debug, Default, PartialEq)]
pub struct ModuleGraph {
    nodes: Vec<Option<Module>>,
    connections: Vec<(NodeId, NodeId)>,
}

impl ModuleGraph {
    pub fn add(&mut self, module: Module) -> NodeId {
        let id = NodeId(self.nodes.len());
        self.nodes.push(Some(module));
        id
    }

    pub fn remove(&mut self, id: NodeId) -> Option<Module> {
        let removed = self.nodes.get_mut(id.0)?.take();
        self.connections
            .retain(|&(from, to)| from != id && to != id);
        removed
    }

    pub fn module(&self, id: NodeId) -> Option<Module> {
        self.nodes.get(id.0).copied().flatten()
    }

    pub fn module_mut(&mut self, id: NodeId) -> Option<&mut Module> {
        self.nodes.get_mut(id.0)?.as_mut()
    }

    pub fn nodes(&self) -> impl Iterator<Item = (NodeId, Module)> + '_ {
        self.nodes
            .iter()
            .enumerate()
            .filter_map(|(index, module)| module.map(|module| (NodeId(index), module)))
    }

    pub fn connections(&self) -> &[(NodeId, NodeId)] {
        &self.connections
    }

    pub fn connect(&mut self, from: NodeId, to: NodeId) -> Result<(), String> {
        if self.module(from).is_none() || self.module(to).is_none() {
            return Err("connection refers to a missing module".into());
        }
        // 当前模块均只有一个输入，新连接替换该输入的旧连接。
        self.disconnect(to);
        self.connections.push((from, to));
        Ok(())
    }

    pub fn disconnect(&mut self, to: NodeId) {
        self.connections.retain(|&(_, target)| target != to);
    }

    pub fn validate(&self) -> Result<(), String> {
        self.execution_order().map(|_| ())
    }

    pub fn execution_order(&self) -> Result<Vec<NodeId>, String> {
        // 先检查环，使循环连接不会被后续的端口类型错误掩盖。
        let mut incoming = vec![0; self.nodes.len()];
        for &(_, to) in &self.connections {
            incoming[to.0] += 1;
        }
        let mut order: Vec<NodeId> = self
            .nodes
            .iter()
            .enumerate()
            .filter_map(|(index, node)| {
                (node.is_some() && incoming[index] == 0).then_some(NodeId(index))
            })
            .collect();
        let mut next = 0;
        while next < order.len() {
            let node = order[next];
            for &(_, to) in self.connections.iter().filter(|&&(from, _)| from == node) {
                incoming[to.0] -= 1;
                if incoming[to.0] == 0 {
                    order.push(to);
                }
            }
            next += 1;
        }
        if order.len() != self.nodes.iter().flatten().count() {
            return Err("module graph contains a dependency cycle".into());
        }

        let output_count = self
            .nodes
            .iter()
            .filter(|node| matches!(node, Some(Module::ViewportOutput)))
            .count();
        if output_count != 1 {
            return Err("the graph requires exactly one viewport output".into());
        }

        for &(from, to) in &self.connections {
            let source = self.module(from).ok_or("connection source is missing")?;
            let target = self.module(to).ok_or("connection target is missing")?;
            match (source.output_type(), target.input_type()) {
                (Some(output), Some(input)) if output == input => {}
                _ => return Err(format!("incompatible connection from {from:?} to {to:?}")),
            }
        }
        for (index, module) in self.nodes.iter().enumerate() {
            if matches!(module, Some(Module::ToneMapping { exposure_ev }) if !exposure_ev.is_finite())
            {
                return Err(format!("module {:?} has invalid exposure", NodeId(index)));
            }
            if module.is_some_and(|module| module.input_type().is_some())
                && !self.connections.iter().any(|&(_, to)| to == NodeId(index))
            {
                return Err(format!(
                    "module {:?} is missing its required input",
                    NodeId(index)
                ));
            }
        }
        Ok(order)
    }

    pub fn display_source(&self) -> Result<(NodeId, f32), String> {
        let ([source, _, _], exposure) = self.display_chain()?;
        Ok((source, exposure))
    }

    pub(crate) fn display_chain(&self) -> Result<([NodeId; 3], f32), String> {
        self.validate()?;
        let output = NodeId(
            self.nodes
                .iter()
                .position(|node| matches!(node, Some(Module::ViewportOutput)))
                .unwrap(),
        );
        let tone = self
            .connections
            .iter()
            .find(|&&(_, to)| to == output)
            .unwrap()
            .0;
        let Module::ToneMapping { exposure_ev } = self.module(tone).unwrap() else {
            return Err("viewport output must receive tone-mapped color".into());
        };
        let source = self
            .connections
            .iter()
            .find(|&&(_, to)| to == tone)
            .unwrap()
            .0;
        Ok(([source, tone, output], exposure_ev))
    }
}

#[derive(Default)]
pub struct RenderFlow {
    draft: ModuleGraph,
    active: Option<ModuleGraph>,
    error: Option<String>,
}

impl RenderFlow {
    pub fn draft(&self) -> &ModuleGraph {
        &self.draft
    }

    pub fn edit(&mut self) -> &mut ModuleGraph {
        self.error = None;
        &mut self.draft
    }

    pub fn active(&self) -> Option<&ModuleGraph> {
        self.active.as_ref()
    }

    pub fn is_applied(&self) -> bool {
        self.active.as_ref() == Some(&self.draft)
    }

    pub fn error(&self) -> Option<&str> {
        self.error.as_deref()
    }

    pub fn apply(&mut self) -> Result<(), String> {
        if let Err(error) = self.draft.validate() {
            self.error = Some(error.clone());
            return Err(error);
        }
        // 验证成功后一次替换，失败时保留上一份完整流程。
        self.active = Some(self.draft.clone());
        self.error = None;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn validates_typed_connections_and_required_inputs() {
        let mut graph = ModuleGraph::default();
        let raster = graph.add(Module::Rasterization);
        let tone = graph.add(Module::ToneMapping { exposure_ev: 0.0 });
        let output = graph.add(Module::ViewportOutput);
        assert!(graph.validate().unwrap_err().contains("required input"));

        graph.connect(raster, tone).unwrap();
        graph.connect(tone, output).unwrap();
        graph.validate().unwrap();

        graph.connect(raster, output).unwrap();
        assert!(graph
            .validate()
            .unwrap_err()
            .contains("incompatible connection"));
        graph.remove(output);
        assert!(graph.validate().unwrap_err().contains("viewport output"));
        assert!(graph.connect(raster, output).is_err());
    }

    #[test]
    fn rejects_cycles_without_replacing_the_active_flow() {
        let mut flow = RenderFlow::default();
        let raster = flow.edit().add(Module::Rasterization);
        let tone = flow.edit().add(Module::ToneMapping { exposure_ev: 0.0 });
        let output = flow.edit().add(Module::ViewportOutput);
        flow.edit().connect(raster, tone).unwrap();
        flow.edit().connect(tone, output).unwrap();
        flow.apply().unwrap();
        let previous = flow.active().unwrap().clone();
        assert!(flow.is_applied());

        flow.edit().connect(tone, tone).unwrap();
        assert!(!flow.is_applied());
        assert!(flow.apply().unwrap_err().contains("cycle"));
        assert!(flow.error().unwrap().contains("cycle"));
        assert_eq!(flow.active(), Some(&previous));

        flow.edit().connect(raster, tone).unwrap();
        flow.edit().disconnect(output);
        assert!(flow.apply().unwrap_err().contains("required input"));
        assert_eq!(flow.active(), Some(&previous));
        flow.edit().connect(tone, output).unwrap();
        flow.apply().unwrap();
        assert!(flow.is_applied());
        assert!(flow.error().is_none());
    }
}
