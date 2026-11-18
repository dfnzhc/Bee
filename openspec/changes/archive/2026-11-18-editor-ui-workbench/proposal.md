# Proposal

## Why

Bee 已具备真实的 Vulkan 视口、光栅化与路径追踪以及 Render Flow，但当前窗口将视口、流程编辑和设备信息简单并排，难以承载后续渲染调试工作。需要建立风格统一、可调整的编辑器界面，使场景、文件和诊断信息围绕主视口组织。

## What Changes

- 建立参考 GPUI Kit 演示的深色工作台，提供场景树、主视口、属性、文件、控制台和 Render Flow 面板；支持面板停靠、调整尺寸、关闭后重新打开及恢复默认布局。
- 场景树和属性面板读取当前内置场景及相机，允许选择并查看真实数据；本次不提供场景编辑或虚构的场景层级。
- 文件面板支持选择文件夹并只读浏览真实目录；控制台展示真实的启动、渲染和 Vulkan 验证消息，并提供基本级别筛选。
- 将现有视口、相机操作、渲染模式切换、流程编辑、设备能力及错误提示接入新布局，保留现有渲染行为和失败诊断。
- 优先评估使用 GPUI Kit 的布局与基础组件，先验证其与当前 GPUI 版本及 Windows 视口接入的兼容性，再确定具体依赖组合。
- 本次不实现对象增删、场景层级编辑、变换或材质编辑、Gizmo、资源导入、场景保存、项目管理或节点画布重做。

## Capabilities

### New Capabilities

- `editor-workbench`: 工作台的视觉布局、面板管理、只读场景检查及现有视口和流程入口。
- `workspace-files`: 在用户选择的目录下只读浏览文件和文件夹。
- `editor-console`: 在工作台中查看、筛选真实的应用与 Vulkan 诊断消息。

### Modified Capabilities

无。现有 `interactive-viewport`、`render-graph`、`path-tracing`、`raster-rendering` 和 `vulkan-backend` 的要求继续适用。

## Impact

- 主要影响 `crates/bee` 的窗口布局、视口和流程面板、场景选择、文件浏览与诊断展示，以及对应的 GPUI 测试和使用文档。
- Vulkan 验证消息当前直接写入标准错误输出；为在控制台展示，可能需要调整 `crates/vulkan` 的诊断接入，同时保留现有终端输出与线程安全。
- 可能调整 `crates/bee/Cargo.toml`、`Cargo.lock` 和 GPUI 相关集成；不得假设 GPUI Kit 与固定的 GPUI 0.2.2 可以直接混用。
- Windows 仍是首个支持和验证的平台；渲染器与 GPUI 的边界、两条渲染路径及现有规格不因界面重排而改变。
