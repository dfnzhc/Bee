#version 450

layout(set = 0, binding = 0, std430) readonly buffer SceneData {
    vec4 values[];
} scene;
layout(push_constant) uniform Constants {
    mat4 view_projection;
    vec3 environment;
    uint light_offset;
} constants;

layout(location = 0) out vec3 position;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 albedo;

void main() {
    uint offset = uint(gl_VertexIndex) * 3;
    position = scene.values[offset].xyz;
    normal = scene.values[offset + 1].xyz;
    albedo = scene.values[offset + 2].xyz;
    gl_Position = constants.view_projection * vec4(position, 1.0);
}
