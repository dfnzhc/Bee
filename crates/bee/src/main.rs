use gpui::{
    div, prelude::*, px, rgb, size, App, Application, Bounds, Context, Window, WindowBounds,
    WindowOptions,
};

struct Bee {
    _backend: Option<bee_vulkan::Backend>,
    lines: Vec<String>,
    failed: bool,
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
                    _backend: Some(backend),
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
            _backend: None,
            lines: vec![message],
            failed: true,
        }
    }
}

impl Render for Bee {
    fn render(&mut self, _window: &mut Window, _cx: &mut Context<Self>) -> impl IntoElement {
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
                    .flex_col()
                    .gap_2()
                    .text_color(if self.failed {
                        rgb(0xffa8a8)
                    } else {
                        rgb(0xd1d7d4)
                    })
                    .children(self.lines.iter().cloned()),
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
