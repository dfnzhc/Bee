mod viewport;

use gpui::{
    div, prelude::*, px, relative, rgb, size, App, Application, Bounds, Context, Window,
    WindowBounds, WindowOptions,
};
use std::sync::Arc;
use viewport::Viewport;

struct Bee {
    viewport: Viewport,
    lines: Vec<String>,
    failed: bool,
}

fn sharing_unavailable_reason() -> &'static str {
    "GPUI 0.2.2 does not expose Windows shared textures or synchronization"
}

impl Bee {
    fn new() -> Self {
        match bee_vulkan::Backend::new(cfg!(debug_assertions)) {
            Ok(backend) => {
                let report = &backend.report;
                let version = report.api_version;
                let mut lines = vec![
                    format!("Device: {}", report.name),
                    format!(
                        "Vulkan {}.{}.{} | Rasterization: available | Graphics queue: {}",
                        version >> 22,
                        (version >> 12) & 0x3ff,
                        version & 0xfff,
                        report.graphics_queue
                    ),
                    format!(
                        "Maximum 2D image: {} px | Color attachments: {}",
                        report.max_image_dimension_2d, report.max_color_attachments
                    ),
                    format!("Validation: {}", report.validation),
                    format!(
                        "GPU image sharing unavailable: {}",
                        sharing_unavailable_reason()
                    ),
                ];
                if report.ray_tracing_missing.is_empty() {
                    lines.push("Ray tracing: available".into());
                } else {
                    lines.push("Ray tracing: unavailable".into());
                    lines.extend(report.ray_tracing_missing.iter().cloned());
                }
                for line in &lines {
                    eprintln!("{line}");
                }
                Self {
                    viewport: Viewport::new(Some(Arc::new(backend))),
                    lines,
                    failed: false,
                }
            }
            Err(error) => Self::failure(format!("{error:#}")),
        }
    }

    fn failure(reason: String) -> Self {
        let message = format!("Vulkan initialization failed: {reason}");
        eprintln!("{message}");
        Self {
            viewport: Viewport::new(None),
            lines: vec![message],
            failed: true,
        }
    }
}

impl Render for Bee {
    fn render(&mut self, _window: &mut Window, cx: &mut Context<Self>) -> impl IntoElement {
        div()
            .size_full()
            .flex()
            .flex_col()
            .gap_3()
            .bg(rgb(0x202326))
            .text_color(rgb(0xf4f4f2))
            .p_6()
            .child("Bee")
            .child(
                div()
                    .flex()
                    .flex_1()
                    .min_h_0()
                    .gap_4()
                    // 并排显示视口和设备报告。
                    .child(self.viewport.render(cx))
                    .child(
                        div()
                            .id("device-info")
                            .w(px(300.0))
                            .max_w(relative(0.4))
                            .flex_shrink_0()
                            .overflow_y_scroll()
                            .flex()
                            .flex_col()
                            .gap_2()
                            .text_color(if self.failed {
                                rgb(0xffa8a8)
                            } else {
                                rgb(0xd1d7d4)
                            })
                            .children(
                                self.lines.iter().cloned().chain(
                                    self.viewport
                                        .error()
                                        .map(|error| format!("Viewport error: {error}")),
                                ),
                            ),
                    ),
            )
    }
}

fn main() {
    Application::new().run(|cx: &mut App| {
        let bounds = Bounds::centered(None, size(px(960.0), px(640.0)), cx);
        cx.open_window(
            WindowOptions {
                window_bounds: Some(WindowBounds::Windowed(bounds)),
                ..Default::default()
            },
            |_, cx| cx.new(|_| Bee::new()),
        )
        .expect("Failed to open the Bee window");
        cx.activate(true);
    });
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn sharing_reports_missing_gpui_interop() {
        let reason = sharing_unavailable_reason();
        assert!(reason.contains("GPUI 0.2.2"));
        assert!(reason.contains("shared textures"));
        assert!(reason.contains("synchronization"));
    }

    #[cfg(target_os = "windows")]
    #[test]
    #[ignore = "requires Vulkan and Direct3D 11 graphics devices"]
    fn probe_gpui_and_vulkan_adapters_without_a_window() -> Result<(), Box<dyn std::error::Error>> {
        use windows::core::Interface;
        use windows::Win32::Foundation::HMODULE;
        use windows::Win32::Graphics::Direct3D::{
            D3D_DRIVER_TYPE_UNKNOWN, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_11_1,
        };
        use windows::Win32::Graphics::Direct3D11::{
            D3D11CreateDevice, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS,
            D3D11_SDK_VERSION,
        };
        use windows::Win32::Graphics::Dxgi::{
            CreateDXGIFactory2, IDXGIAdapter1, IDXGIFactory6, DXGI_CREATE_FACTORY_FLAGS,
        };

        // 按 GPUI 的要求筛选适配器，再比较它与 Vulkan 的 LUID。
        let backend = bee_vulkan::Backend::new(true)?;
        let report = backend.probe_sharing()?;
        let factory: IDXGIFactory6 = unsafe { CreateDXGIFactory2(DXGI_CREATE_FACTORY_FLAGS(0)) }?;
        let mut selected = None;
        for index in 0.. {
            let Ok(adapter) = (unsafe { factory.EnumAdapters(index) }) else {
                break;
            };
            let adapter: IDXGIAdapter1 = adapter.cast()?;
            let mut device = None;
            if unsafe {
                D3D11CreateDevice(
                    &adapter,
                    D3D_DRIVER_TYPE_UNKNOWN,
                    HMODULE::default(),
                    D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                    Some(&[
                        D3D_FEATURE_LEVEL_11_1,
                        D3D_FEATURE_LEVEL_11_0,
                        D3D_FEATURE_LEVEL_10_1,
                    ]),
                    D3D11_SDK_VERSION,
                    Some(&mut device),
                    None,
                    None,
                )
            }
            .is_err()
            {
                continue;
            }
            let mut features = D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS::default();
            let Some(device) = device else { continue };
            if unsafe {
                device.CheckFeatureSupport(
                    D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS,
                    &mut features as *mut _ as _,
                    std::mem::size_of_val(&features) as u32,
                )
            }
            .is_err()
                || !features
                    .ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x
                    .as_bool()
            {
                continue;
            }
            let desc = unsafe { adapter.GetDesc1() }?;
            let mut luid = [0u8; 8];
            luid[..4].copy_from_slice(&desc.AdapterLuid.LowPart.to_le_bytes());
            luid[4..].copy_from_slice(&desc.AdapterLuid.HighPart.to_le_bytes());
            selected = Some((
                String::from_utf16_lossy(&desc.Description)
                    .trim_matches(char::from(0))
                    .to_owned(),
                luid,
            ));
            break;
        }
        let (name, dxgi_luid) = selected.ok_or("no usable Direct3D 11 adapter")?;
        println!("GPUI adapter selection: {name}, LUID {dxgi_luid:02x?}");
        println!("Vulkan sharing probe: {report:?}");
        println!(
            "Adapter LUIDs match: {}",
            report.device_luid == Some(dxgi_luid)
        );
        Ok(())
    }

    #[test]
    fn unavailable_device_is_shown_as_an_error() {
        if std::env::var_os("BEE_TEST_NO_DRIVER").is_some() {
            let view = Bee::new();
            assert!(view.failed);
            assert!(view.lines[0].contains("Vulkan initialization failed"));
            assert!(
                view.lines[0].contains("could not create the Vulkan instance")
                    || view.lines[0].contains("no usable Vulkan graphics device")
                    || view.lines[0].contains("could not load the Vulkan loader"),
                "{}",
                view.lines[0]
            );
            return;
        }
        let output = std::process::Command::new(std::env::current_exe().unwrap())
            .args([
                "--exact",
                "tests::unavailable_device_is_shown_as_an_error",
                "--nocapture",
            ])
            .env("BEE_TEST_NO_DRIVER", "1")
            .env("VK_DRIVER_FILES", "bee-no-such-vulkan-driver.json")
            .env("VK_ICD_FILENAMES", "bee-no-such-vulkan-driver.json")
            .output()
            .unwrap();
        assert!(
            output.status.success(),
            "{}\n{}",
            String::from_utf8_lossy(&output.stdout),
            String::from_utf8_lossy(&output.stderr)
        );
    }
}
