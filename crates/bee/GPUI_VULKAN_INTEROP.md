# GPUI 与 Vulkan 画面接入验证

验证环境：Windows、GPUI 0.2.2、NVIDIA GeForce RTX 4060 Laptop GPU，
NVIDIA 驱动 581.29。Vulkan 设备的 LUID 为 `ed1e010000000000`。

- GPUI 的 Windows 后端使用 Direct3D 11。其公开的 `RenderImage` / `paint_image`
  接口接收 CPU 内存中的 BGRA 图像；`surface` 的外部图像入口仅用于 macOS。
  Windows 后端没有公开导入外部纹理、访问 D3D 设备或交接同步对象的接口。
- 无窗口测试复现了 GPUI 源码中的 DXGI 适配器选择条件，选中的设备名称和
  LUID 与 Vulkan 一致。该测试并未访问实际 GPUI 窗口的 D3D 设备；GPUI 公共的
  `gpu_specs()` 只暴露名称，不能据此确认两者使用的是同一个设备。
- Vulkan 查询使用 `B8G8R8A8_UNORM`、最优平铺，以及颜色附件、采样和传输源
  用途。本机 `D3D11_TEXTURE` 外部内存可导入但不可导出；`OPAQUE_WIN32`
  外部内存可导入和导出。这些是能力查询，不是跨 API 导入测试。
- 本机 `D3D12_FENCE` 信号量可导入但不可导出，`OPAQUE_WIN32` 信号量可导入
  和导出，也报告支持 `VK_KHR_win32_keyed_mutex`。GPUI 没有公开对应的 D3D
  访问或同步交接入口，因此无法验证这些机制与 GPUI 的互操作。

结论：当前 GPUI 版本不能建立已验证的 GPU 共享纹理路径，使用 Vulkan 异步读回
和 GPUI 图像上传。硬件扩展存在或设备名称相同都不足以将共享路径标记为可用。

无窗口设备与句柄探测：

```text
cargo test -p bee probe_gpui_and_vulkan_adapters_without_a_window -- --ignored --nocapture
```

无窗口画面与资源验证：

```text
cargo test --workspace
cargo test -p bee gpui_displays_readback_and_recovers_from_resize_and_failure -- --ignored --nocapture
cargo test -p bee-vulkan --lib test_image -- --ignored --nocapture --test-threads=1
```

这些测试使用真实 Vulkan 设备和验证层，以及 GPUI 内存测试平台。测试覆盖 BGRA
颜色、带填充行跨度、画布绘制调用、连续尺寸变化、不可绘制尺寸、资源释放、
读回失败和模拟的上传失败。GPUI 的测试图集不执行实际 D3D 上传；真实 Windows
窗口的最小化和恢复也未在本次无窗口测试中操作。GPUI 0.2.2 的 Windows 实现会在
最小化时暂停帧回调，并在恢复时恢复该回调。
