# Spec Delta

## MODIFIED Requirements

### Requirement: 显示统一的默认工作台
系统 SHALL 在项目和场景成功打开后，于桌面窗口中提供统一的深色工作台，默认将 `Scene`、`Viewport` 和 `Properties` 从左至右排列，并以主视口为主要工作区域。`Files`、`Console` 和 `Render Flow` 默认收起。

#### Scenario: 打开工作台
- **WHEN** 用户成功创建或打开项目并加载其中一个场景
- **THEN** 左侧显示当前场景的场景树，中央显示主视口，右侧显示属性，底部仅显示抽屉入口
- **AND** `Files`、`Console`、`Render Flow` 均可从底部入口和 `View` 菜单访问

#### Scenario: 默认窗口的画面占比
- **WHEN** Bee 在 `1200 × 800` 逻辑像素客户端区域中使用默认布局，且渲染器已输出稳定画面
- **THEN** 画面按可绘制区域尺寸渲染并铺满该区，保持正确比例
- **AND** 可见渲染画面至少占客户端区域的 `50%`，画面占比仅计实际显示的渲染图像

#### Scenario: 使用较小窗口或系统缩放
- **WHEN** 用户在 `960 × 640` 逻辑像素窗口或较高系统缩放下使用工作台
- **THEN** 菜单、场景选择器、渲染模式控件和抽屉入口完整可见，支持鼠标及键盘操作
- **AND** 面板内容仍可通过滚动、调整或切换访问

### Requirement: 提供工作台应用菜单
系统 SHALL 在 `Bee` 顶栏提供独立的 `File` 和 `View` 菜单。`File` 提供打开项目、关闭项目、保存当前场景和退出，`View` 提供面板与布局操作，外部目录选择入口 `Browse Folder…` 位于 `Files` 面板内。

#### Scenario: 查看文件菜单并退出
- **WHEN** 用户打开 `File` 菜单
- **THEN** 菜单提供打开项目、关闭项目、保存当前场景和 `Exit`
- **AND** 用户选择 `Exit` 后走包含未保存确认和渲染资源清理的窗口关闭流程

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

## ADDED Requirements

### Requirement: 从工作台顶栏查看和选择场景
系统 SHALL 在工作台顶栏显示当前项目与场景名称、未保存状态及项目内的场景选择器。场景选择器 SHALL 提供切换、新建、复制、重命名、删除和设为启动场景的入口。

#### Scenario: 查看场景选择器
- **WHEN** 用户打开场景选择器
- **THEN** 列出当前项目的场景，并以可辨识的标记区分当前场景和启动场景

#### Scenario: 切换到另一个场景
- **WHEN** 用户通过场景选择器成功切换场景
- **THEN** 顶栏名称、场景树、属性、视口与 Render Flow 显示目标场景的状态
- **AND** 旧场景的对象选择被清除
