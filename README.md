# Bee

Bee 是一个 Windows 三维渲染原型，使用 GPUI 显示界面，由项目内的 Vulkan 后端渲染内置场景。

## 运行环境

- 64 位 Windows，以及支持 Vulkan 图形队列的显卡和驱动。
- Rust 的 `x86_64-pc-windows-msvc` 工具链及 MSVC C++ 构建工具；本次验证使用 Rust 1.100.0。
- Vulkan SDK **1.4.321.1**。构建脚本通过 `VULKAN_SDK` 查找该版本的 `Bin/glslc.exe`，并检查编译器版本。
- 路径追踪要求 Vulkan 1.1 或更新版本、图形队列的计算能力、加速结构、光追流水线和缓冲区设备地址等能力。应用会检查扩展、特性及属性；缺失时仍可使用光栅化，并在设备信息中给出原因。
- 开发期验证需要 Vulkan SDK 中的 `VK_LAYER_KHRONOS_validation`。Debug 构建会尝试启用它；界面和终端会报告实际状态。

## 构建和启动

在仓库根目录的 PowerShell 中，将 SDK 路径替换为本机安装路径：

```powershell
$env:VULKAN_SDK = 'C:\VulkanSDK\1.4.321.1'
cargo build -p bee
.\target\debug\bee.exe
```

首次构建需要获取 Cargo 依赖；依赖已缓存时可使用 `cargo build -p bee --offline`。如果要启用验证层的同步检查，在同一 PowerShell 会话中启动前设置：

```powershell
$env:VK_LAYER_VALIDATE_SYNC = '1'
.\target\debug\bee.exe
```

## 操作

- 在场景画面上按住鼠标左键拖动以旋转相机，滚动鼠标滚轮以缩放。
- 使用视口上方的 `Raster` 和 `Path tracing` 快捷按钮切换路径；路径追踪可用时，静止画面的样本数会持续增加。切换时会修改当前流程草稿的渲染源并尝试应用整份草稿，按钮的 `active` 标记表示实际生效的路径。
- 在底部选择 `Render Flow` 标签，先选择节点的 `Output`，再选择目标节点的 `Input`，最后点击 `Apply flow`。节点可添加、移除、断开，色调映射节点可调整曝光；无效草稿或不可用的路径会说明原因并保留上一份有效流程。面板关闭后，可从顶部 `Panels` 菜单重新打开。
- 调整窗口或面板大小后，视口会按新尺寸重新渲染。视口状态显示渲染路径、样本数、画面接入方式和错误，悬停可查看完整错误；底部 `Console` 标签显示设备能力及视口错误。关闭的面板也可从 `Panels` 菜单恢复。
- 在底部 `Files` 标签点击 `Choose folder` 选择本地文件夹；当前路径显示在工具栏下方，取消选择会保留原位置。点击 `[Folder]` 进入子目录，使用 `Up` 返回上级、`Refresh` 重新读取；`[File]` 只显示名称，浏览不会写入、导入或打开文件。空目录和读取失败会显示状态，目录被删除或无法访问时仍可返回或重新选择。

## 当前限制

两条路径使用相同的内置几何体、漫反射材质、点光源和相机。路径追踪是渐进采样，运动时允许噪点，暂不提供降噪；节点图暂不保存到磁盘。当前 GPUI 版本没有可用的 Windows 外部纹理和同步交接入口，画面通过 Vulkan 异步读回再上传到 GPUI，可能带来额外延迟。没有预设帧率验收门槛。

待排查：工作台改动后，用户反馈渲染体感明显变慢。原因尚未确认，后续可在相同视口物理尺寸和构建配置下比较渲染、读回、显示及面板刷新耗时。

## 测试

在仓库根目录执行常规测试。设备测试默认跳过，需要支持光追的 Vulkan 设备、Direct3D 11 和可用的验证层：

```powershell
cargo test --workspace
$env:VK_LAYER_VALIDATE_SYNC = '1'
cargo test --workspace -- --ignored --test-threads=1
```

运行以下示例可以检查 GPUI Kit 停靠布局、文件夹选择和现有视口：

```powershell
cargo run -p bee --example workbench_probe
```

该示例用于验证接入，尚不是正式工作台。
