#version 450

layout(set = 0, binding = 0, std430) readonly buffer Mesh {
    vec4 values[];
} mesh;
struct Instance {
    mat4 model;
    mat3 normal;
    uvec4 material;
};
layout(set = 0, binding = 1, std430) readonly buffer Instances {
    Instance values[];
} instances;
layout(set = 0, binding = 2, std430) readonly buffer Materials {
    vec4 values[];
} materials;
layout(push_constant) uniform Constants {
    mat4 view_projection;
    vec3 environment;
} constants;

layout(location = 0) out vec3 position;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 albedo;

void main() {
    Instance instance = instances.values[gl_InstanceIndex];
    uint offset = uint(gl_VertexIndex) * 2;
    position = (instance.model * mesh.values[offset]).xyz;
    normal = normalize(instance.normal * mesh.values[offset + 1].xyz);
    albedo = materials.values[instance.material.x].xyz;
    gl_Position = constants.view_projection * vec4(position, 1.0);
}
