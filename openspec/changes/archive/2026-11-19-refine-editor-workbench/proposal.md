# Proposal

## Why

Bee 已有实时视口和可停靠面板。当前顶栏只有 `Panels`，默认展开的底部区域和常驻诊断又挤占画面，需要按已确认的样稿重新组织工作台入口和空间。

## What Changes

- 将顶栏改为 `Bee`、`File`、`View`。本轮 `File` 仅提供 `Exit`，`View` 提供六个面板入口和 `Reset Layout`。
- 采用左侧 `Scene`、中央 `Viewport`、右侧 `Properties` 的紧凑默认布局。`Files`、`Console`、`Render Flow` 通过底部入口展开，启动和重置布局时抽屉收起。
- `Files` 内提供 `Browse Folder…`，沿用当前的只读目录浏览。
- 视口用单行工具栏显示渲染模式和光追样本数，画面填满剩余区域。常规诊断在 `Console` 查看，异常在视口提示。
- 将场景新建、打开、打开最近场景和保存，项目资源目录、资源导入与管理，以及场景编辑、视口拾取和 Gizmo 留给后续变更。

## Capabilities

### New Capabilities

### Modified Capabilities

- `editor-workbench` 定义顶栏菜单、默认布局、底部抽屉和渲染状态的位置。

## Impact

- 涉及 `crates/bee/src/workbench.rs`、`crates/bee/src/workbench/files.rs`、`crates/bee/src/viewport.rs`、相关 GPUI 回归测试与 `README.md`。
- `editor-workbench` 的基线来自已完成的 `editor-ui-workbench`。归并本增量前，先将该基线同步到主规格。
