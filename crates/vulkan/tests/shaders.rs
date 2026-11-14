use std::path::PathBuf;
use std::process::Command;

fn compiler() -> PathBuf {
    PathBuf::from(std::env::var_os("VULKAN_SDK").expect("VULKAN_SDK is required"))
        .join("Bin")
        .join("glslc.exe")
}

#[test]
fn shader_build_is_reproducible() {
    for (name, stage) in [
        ("foundation.vert", "vert"),
        ("tonemap.vert", "vert"),
        ("tonemap.frag", "frag"),
        ("raster.vert", "vert"),
        ("raster.frag", "frag"),
        ("path.rgen", "rgen"),
        ("path.rmiss", "rmiss"),
        ("path.rchit", "rchit"),
    ] {
        let source = PathBuf::from(env!("CARGO_MANIFEST_DIR"))
            .join("shaders")
            .join(name);
        let output = PathBuf::from(env!("OUT_DIR")).join(format!("{name}.reproduced.spv"));
        let mut command = Command::new(compiler());
        let target = if stage.starts_with('r') {
            command.args(["--target-env=vulkan1.1", "--target-spv=spv1.4"]);
            "vulkan1.1spv1.4"
        } else {
            command.arg("--target-env=vulkan1.0");
            "vulkan1.0"
        };
        let optimization = if stage.starts_with('r') { "-O0" } else { "-O" };
        let result = command
            .args([&format!("-fshader-stage={stage}"), optimization])
            .arg(source)
            .arg("-o")
            .arg(&output)
            .output()
            .unwrap();
        assert!(
            result.status.success(),
            "{}",
            String::from_utf8_lossy(&result.stderr)
        );
        let validator = Command::new(compiler().with_file_name("spirv-val.exe"))
            .args(["--target-env", target])
            .arg(&output)
            .output()
            .unwrap();
        assert!(
            validator.status.success(),
            "{}",
            String::from_utf8_lossy(&validator.stderr)
        );
        let bytes = std::fs::read(output).unwrap();
        assert_eq!(&bytes[..4], &[0x03, 0x02, 0x23, 0x07]);
        assert_eq!(
            bytes,
            std::fs::read(PathBuf::from(env!("OUT_DIR")).join(format!("{name}.spv"))).unwrap()
        );
        if name == "foundation.vert" {
            assert_eq!(bytes, bee_vulkan::FOUNDATION_SHADER);
        }
    }
}

#[test]
fn shader_errors_identify_the_source_and_line() {
    let source = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("tests/fixtures/invalid.vert");
    let output = PathBuf::from(env!("OUT_DIR")).join("invalid.spv");
    let result = Command::new(compiler())
        .args(["--target-env=vulkan1.0", "-fshader-stage=vert", "-O"])
        .arg(source)
        .arg("-o")
        .arg(output)
        .output()
        .unwrap();
    let error = String::from_utf8_lossy(&result.stderr);
    assert!(!result.status.success());
    assert!(error.contains("invalid.vert:2:"), "{error}");
    assert!(error.contains("error"), "{error}");
}
