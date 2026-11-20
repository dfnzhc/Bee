# Bee

Bee 是一个 Windows 三维渲染原型，使用 GPUI 显示界面，由项目内的 Vulkan 后端渲染 Bee 项目中的场景。

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

## 项目与场景

1. 启动后在 `Bee Projects` 选择 `New Project`，选定位置，输入项目名称并选择 `Built-in Example` 或 `Blank`。默认在所选位置创建同名子目录。直接使用已选的空目录时，选择 `Use the selected empty folder`。点击 `Create` 后进入首个场景。
2. `File` > `Close Project` 返回项目选择页。用 `Recent Projects` 重新打开，或用 `Open Project` 选择包含 `project.beeproj` 的目录。工作台里的 `File` 菜单也能新建或打开项目。项目移动后可从新目录打开，或用最近列表的 `Locate` 更新位置。`Remove` 仅移除本机最近记录。
3. 顶栏显示项目和当前场景名称。打开场景选择器可切换场景，或使用 `New Scene`、`Copy Saved Scene`、`Rename Scene`、`Set as Startup Scene`。复制读取来源场景已保存的版本。列表中的 `Current` 和 `Startup` 分别标出当前场景和下次打开项目时使用的场景。
4. 旋转或缩放只改变本次浏览视角。需要下次从该视角开始时，在场景选择器选择 `Set Current View as Start View`，再用 `File` > `Save Scene` 或 `Ctrl+S` 保存。保存包含起始相机、Render Flow 草稿和最后一次应用成功的流程。场景名旁的 `*` 表示有未保存的改动。离开该场景时可选择 `Save`、`Discard` 或 `Cancel`。

随项目一起移动或备份整个目录。`project.beeproj` 登记项目名称、场景及启动场景。`Scenes/<场景 ID>.beescene` 保存独立的场景文档。`Assets/` 存放项目共享资源。

每次替换已保存的场景都会保留 `Scenes/<场景 ID>.beescene.bak`。若正式文件丢失或损坏，重新打开项目或切换到该场景时，可在错误对话框选择其他场景，或选择 `Restore Backup` 并确认恢复。恢复使用上一份有效的已保存版本。

通过 `Delete Current Scene` 删除时，先选择下一个要打开的场景。删除的是启动场景时，该选项也成为新的启动场景。项目至少保留一个场景。删除后的文件通常位于 `Trash/<场景 ID>.beescene`。工作台会报告实际保留路径，包括移动失败时的原位置。

## 操作

- `File` > `Exit` 和窗口关闭按钮都先处理未保存改动，再等待在途渲染工作完成并释放资源。`View` 提供六个面板入口和 `Reset Layout`，可切换或恢复面板，以及重置布局。用 Tab 聚焦菜单或场景选择器，Enter 或空格打开，方向键选择条目，Enter 确认，Escape 关闭并返回对应按钮。
- 默认布局从左至右是 `Scene`、`Viewport`、`Properties`，底部抽屉默认收起。点击底部 `Files`、`Console` 或 `Render Flow` 展开或切换，也可用 Tab 聚焦入口后按 Enter 或空格。再次选择抽屉中的当前入口，或使用底部右侧的收起按钮，可把空间还给视口。面板拖到其他停靠位置后，入口会聚焦新位置。`View` 也可显示或恢复面板。`Reset Layout` 恢复默认分区并收起抽屉，保留浏览位置、控制台历史与筛选，以及流程草稿和生效流程。
- 在场景画面上按住鼠标左键拖动以旋转相机，滚动鼠标滚轮以缩放。
- 使用视口单行工具栏中的 `Raster` 和 `Path tracing` 快捷按钮切换路径。切换时会修改当前流程草稿的渲染源并尝试应用整份草稿，按钮的 `active` 标记表示实际生效的路径。光追生效时，同一行显示当前样本数。
- 在底部选择 `Render Flow` 标签，先选择节点的 `Output`，再选择目标节点的 `Input`，最后点击 `Apply flow`。节点可添加、移除、断开，色调映射节点可调整曝光；无效草稿或不可用的路径会说明原因并保留上一份有效流程。
- 渲染画面铺满工具栏下方的区域，开合抽屉或调整窗口、面板大小后会按新的可绘制尺寸重新渲染。光追不可用时仍可使用光栅化，悬停在 `Path tracing unavailable` 按钮上可查看原因。视口内显示渲染或流程错误，悬停可查看完整提示。
- 在底部 `Files` 标签点击 `Browse Folder…` 选择本地文件夹；当前路径显示在工具栏下方，取消选择会保留原位置。点击 `[Folder]` 进入子目录，使用 `Up` 返回上级、`Refresh` 重新读取；`[File]` 只显示名称，浏览不会写入、导入或打开文件。空目录和读取失败会显示状态，目录被删除或无法访问时仍可返回或重新选择。
- 底部 `Console` 标签持续收取本次运行的设备报告、初始化失败、视口和 Render Flow 错误，以及可用的 Vulkan 验证警告与错误；每条消息标明 `Info`、`Warning` 或 `Error` 和来源。三个级别按钮分别开关筛选，隐藏消息仍保留在历史中；按钮可通过 Tab 聚焦、Enter 或空格操作。视口及流程面板原有的错误提示和终端输出继续保留。
- 控制台最多保留 1000 条历史，后台队列最多容纳 256 条消息；超出容量时显示累计丢弃数量，提示不随级别筛选隐藏。关闭控制台面板后仍保留筛选和有界历史。

## 设备与验证诊断

打开项目后在 `Console` 查看实际设备名、Vulkan 版本、图形队列、图像尺寸限制、光追能力及 `Validation` 状态。`Viewport` 来源的 `Image presentation` 信息记录当前画面接入方式。`enabled` 表示验证层和消息回调可用；`requested but unavailable` 表示本机缺少所请求的验证层；`enabled; debug callback unavailable` 表示验证层已启用但缺少调试回调扩展；`disabled` 表示本次未请求验证。缺少验证层或回调时会额外记录 `Warning`，不会显示为完整启用。

设备报告使用 `Info`，初始化、视口及流程失败使用 `Error`；Vulkan 回调保留验证层给出的警告或错误级别，同时继续写入标准错误。要检查流程错误，可断开色调映射节点的输入并点击 `Apply flow`：Render Flow 会保留无效草稿和原错误提示，Console 记录对应的 `Error`，上一份有效流程继续生效。只有已启用的验证层实际产生消息时，控制台才会出现 `Vulkan validation` 条目。

## 当前限制

两条路径使用当前场景的几何体、漫反射材质、点光源和相机。路径追踪是渐进采样，运动时允许噪点，暂不提供降噪。当前 GPUI 版本没有可用的 Windows 外部纹理和同步交接入口，画面通过 Vulkan 异步读回再上传到 GPUI，可能带来额外延迟。没有预设帧率验收门槛。

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
