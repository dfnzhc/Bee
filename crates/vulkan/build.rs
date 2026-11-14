use std::env;
use std::path::PathBuf;
use std::process::Command;

const COMPILER_VERSION: &str = "shaderc v2023.8 v2025.3\n\
spirv-tools v2025.3 v2022.4-833-g33e02568\n\
glslang 11.1.0-1253-gefd24d75";

fn main() {
    println!("cargo:rerun-if-env-changed=VULKAN_SDK");
    let sdk = env::var_os("VULKAN_SDK").expect("VULKAN_SDK must point to Vulkan SDK 1.4.321.1");
    let compiler = PathBuf::from(sdk).join("Bin").join("glslc.exe");
    println!("cargo:rerun-if-changed={}", compiler.display());
    // 固定编译器版本，确保嵌入的 SPIR-V 可以复现。
    let version = Command::new(&compiler)
        .arg("--version")
        .output()
        .unwrap_or_else(|error| panic!("could not run {}: {error}", compiler.display()));
    let version_text = String::from_utf8_lossy(&version.stdout).replace("\r\n", "\n");
    assert!(
        version.status.success() && version_text.starts_with(COMPILER_VERSION),
        "expected Vulkan SDK 1.4.321.1 glslc; {} reported:\n{}{}",
        compiler.display(),
        version_text,
        String::from_utf8_lossy(&version.stderr)
    );
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
        let source = format!("shaders/{name}");
        println!("cargo:rerun-if-changed={source}");
        let output_path =
            PathBuf::from(env::var_os("OUT_DIR").unwrap()).join(format!("{name}.spv"));
        let mut command = Command::new(&compiler);
        if stage.starts_with('r') {
            command.args(["--target-env=vulkan1.1", "--target-spv=spv1.4"]);
        } else {
            command.arg("--target-env=vulkan1.0");
        }
        let optimization = if stage.starts_with('r') { "-O0" } else { "-O" };
        let output = command
            .args([
                &format!("-fshader-stage={stage}"),
                optimization,
                &source,
                "-o",
            ])
            .arg(output_path)
            .output()
            .unwrap_or_else(|error| panic!("could not run {}: {error}", compiler.display()));
        assert!(
            output.status.success(),
            "GLSL compilation failed:\n{}{}",
            String::from_utf8_lossy(&output.stdout),
            String::from_utf8_lossy(&output.stderr)
        );
    }
}
