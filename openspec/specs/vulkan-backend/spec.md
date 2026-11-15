# vulkan-backend Specification

## Purpose

为 Windows 上的三维渲染提供可验证的 Vulkan 后端，明确设备能力、资源使用和故障报告，让光栅化与路径追踪可以共享可靠的底层基础。

## Requirements

### Requirement: 使用自研 Vulkan 后端渲染三维场景
系统 SHALL 由项目自身封装并调用 Vulkan 图形接口来生成三维场景画面。

#### Scenario: 场景渲染启动
- **WHEN** 用户启动支持 Vulkan 的应用并进入三维视口
- **THEN** 系统使用自研 Vulkan 渲染后端生成场景画面
- **AND** 界面自身的绘制不要求使用该 Vulkan 后端

### Requirement: 报告设备及渲染能力
系统 SHALL 检测并报告所选设备的基础图形能力和路径追踪所需的光追能力。

#### Scenario: 设备支持光追
- **WHEN** 所选设备满足路径追踪所需能力
- **THEN** 系统将光栅化和路径追踪都标记为可用

#### Scenario: 设备不支持光追
- **WHEN** 所选设备满足基础图形要求但缺少路径追踪所需能力
- **THEN** 系统说明缺失的能力并禁用路径追踪
- **AND** 光栅化和编辑器仍可使用

### Requirement: 报告 Vulkan 初始化失败
系统 SHALL 在 Vulkan 初始化失败时提供可辨识的错误信息，不得将失败伪装为成功渲染。

#### Scenario: 没有可用 Vulkan 设备
- **WHEN** 系统无法找到满足基础图形要求的 Vulkan 设备
- **THEN** 应用向用户报告初始化失败及可获得的原因

### Requirement: 保护在途图形资源的生命周期
系统 SHALL 保证渲染使用中的资源在相关 GPU 工作完成之前不会被释放或重用。

#### Scenario: 资源在视口调整大小后替换
- **WHEN** 视口尺寸变化导致渲染资源被替换，而旧资源仍被已提交工作引用
- **THEN** 系统在旧资源不再被使用后才释放或重用它们

### Requirement: 提供开发期验证信息
系统 SHALL 支持在可用时启用 Vulkan 验证层，并暴露初始化和渲染错误以便诊断。

#### Scenario: 验证层可用
- **WHEN** 开发者以验证模式运行且验证层可用
- **THEN** 系统启用验证层并输出其报告

#### Scenario: 验证层不可用
- **WHEN** 请求验证模式但验证层不可用
- **THEN** 系统报告该情况，而不是声称验证层已经启用
