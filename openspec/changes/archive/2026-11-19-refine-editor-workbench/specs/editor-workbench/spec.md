# Spec Delta

## ADDED Requirements

### Requirement: 提供工作台应用菜单
系统 SHALL 在 `Bee` 顶栏提供独立的 `File` 和 `View` 菜单。`File` 提供应用退出，`View` 提供面板与布局操作，目录选择入口 `Browse Folder…` 位于 `Files` 面板内。

#### Scenario: 查看文件菜单并退出
- **WHEN** 用户打开 `File` 菜单
- **THEN** 菜单的唯一条目为 `Exit`，选择后走正常的窗口关闭流程

#### Scenario: 从 View 访问面板
- **WHEN** 用户打开 `View` 菜单
- **THEN** 菜单提供 `Scene`、`Viewport`、`Properties`、`Files`、`Console`、`Render Flow` 和 `Reset Layout`
- **AND** 选择已关闭或被其他标签遮住的面板会显示并聚焦该面板

#### Scenario: 在文件面板选择目录
- **WHEN** 用户在 `Files` 面板选择 `Browse Folder…`
- **THEN** 系统打开文件夹选择器，选中目录后在该面板列出真实目录内容

#### Scenario: 用键盘操作菜单
- **WHEN** 用户用键盘聚焦并打开 `File` 或 `View`
- **THEN** 用户可通过键盘选择菜单项，或按 `Escape` 关闭菜单并将焦点返回触发按钮

### Requirement: 按需展开工作台底部面板
系统 SHALL 在工作台底部保留 `Files`、`Console` 和 `Render Flow` 入口，并可按需展开或收起底部停靠区域。

#### Scenario: 从底部入口展开与切换
- **WHEN** 用户在默认停靠布局下操作任一底部入口
- **THEN** 对应面板展开或切换为当前面板，且同一时间只展示抽屉内的一个面板
- **AND** 再次选择当前入口或使用收起控件会折叠抽屉，释放空间给视口

#### Scenario: 重新打开已收起的抽屉
- **WHEN** 用户收起并重新打开任一底部面板
- **THEN** `Files` 保留浏览位置，`Console` 保留日志与筛选，`Render Flow` 保留草稿及生效流程

#### Scenario: 访问已移动的面板
- **WHEN** 用户通过底部入口访问已拖到其他位置的面板
- **THEN** 系统聚焦该面板所在位置，且面板只出现一次

## MODIFIED Requirements

### Requirement: 显示统一的默认工作台
系统 SHALL 在桌面窗口中提供统一的深色工作台，默认将 `Scene`、`Viewport` 和 `Properties` 从左至右排列，并以主视口为主要工作区域。`Files`、`Console` 和 `Render Flow` 默认收起。

#### Scenario: 打开工作台
- **WHEN** 用户启动 Bee
- **THEN** 左侧显示场景树，中央显示主视口，右侧显示属性，底部仅显示抽屉入口
- **AND** `Files`、`Console`、`Render Flow` 均可从底部入口和 `View` 菜单访问

#### Scenario: 默认窗口的画面占比
- **WHEN** Bee 在 `1200 × 800` 逻辑像素客户端区域中使用默认布局，且渲染器已输出稳定画面
- **THEN** 画面按可绘制区域尺寸渲染并铺满该区，保持正确比例
- **AND** 可见渲染画面至少占客户端区域的 `50%`，画面占比仅计实际显示的渲染图像

#### Scenario: 使用较小窗口或系统缩放
- **WHEN** 用户在 `960 × 640` 逻辑像素窗口或较高系统缩放下使用工作台
- **THEN** 菜单、渲染模式控件和抽屉入口完整可见，支持鼠标及键盘操作
- **AND** 面板内容仍可通过滚动、调整或切换访问

### Requirement: 调整和恢复工作台面板
系统 SHALL 允许用户调整面板分区尺寸、拖动并停靠面板、关闭和重新打开面板，以及恢复默认布局。

#### Scenario: 调整面板分区
- **WHEN** 用户拖动分区边界
- **THEN** 相邻面板尺寸相应变化，且面板仍可操作

#### Scenario: 拖动和重新停靠面板
- **WHEN** 用户将一个面板拖到其他受支持的停靠位置
- **THEN** 面板在目标位置显示，并保留原有内容及状态

#### Scenario: 关闭和恢复面板
- **WHEN** 用户关闭面板后通过 `View` 重新打开它
- **THEN** 复用同一面板及其内容，视口以当前场景、相机及生效流程恢复显示

#### Scenario: 恢复默认布局
- **WHEN** 用户选择 `View > Reset Layout`
- **THEN** 系统恢复默认面板位置、尺寸和可见性，收起底部抽屉
- **AND** 保留当前场景、相机、文件浏览位置、控制台历史与筛选、Render Flow 草稿和生效流程

### Requirement: 显示可理解的渲染状态
系统 SHALL 在视口的单行工具栏显示 `Raster`、`Path tracing` 及其生效状态，并在光追模式显示当前样本数。`Console` 提供当前画面接入方式和设备诊断，视口就地显示渲染异常和不可用状态。

#### Scenario: 路径追踪不可用
- **WHEN** 设备不支持路径追踪
- **THEN** 视口标识该路径不可用及其原因，并保留可用的光栅化视口

#### Scenario: 画面接入失败
- **WHEN** 视口无法显示有效渲染结果
- **THEN** 视口就地显示可辨识的失败原因和异常状态，`Console` 记录相应错误

#### Scenario: 查看常规渲染诊断
- **WHEN** 用户打开 `Console` 查看渲染诊断
- **THEN** 可查到实际画面接入方式及当前设备能力
- **AND** 视口正常显示时，其工具栏保持单行并显示渲染模式及适用的光追样本数
