# Shader build

Set `VULKAN_SDK` to Vulkan SDK 1.4.321.1. The build requires its `glslc`
versions: shaderc v2023.8 v2025.3, SPIRV-Tools v2025.3 v2022.4-833-g33e02568,
and glslang 11.1.0-1253-gefd24d75. Cargo compiles `foundation.vert`,
`tonemap.vert`, `tonemap.frag`, `raster.vert`, and `raster.frag` with `--target-env=vulkan1.0`, their matching
shader stages, and `-O` into its build output.
Compiler errors include the source location and stop the build.
