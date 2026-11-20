#version 450

layout(set = 0, binding = 3, std430) readonly buffer Lights {
    vec4 values[];
} lights;
layout(push_constant) uniform Constants {
    mat4 view_projection;
    vec3 environment;
} constants;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 albedo;
layout(location = 0) out vec4 color;

void main() {
    vec3 lighting = constants.environment;
    float normal_length_squared = dot(normal, normal);
    vec3 surface_normal = normal_length_squared > 0.0
        ? normal * inversesqrt(normal_length_squared) : vec3(0.0);
    for (uint i = 0; i + 1 < lights.values.length(); i += 2) {
        vec3 to_light = lights.values[i].xyz - position;
        float distance_squared = dot(to_light, to_light);
        if (distance_squared > 0.000001) {
            float cosine = max(dot(surface_normal, to_light * inversesqrt(distance_squared)), 0.0);
            lighting += lights.values[i + 1].xyz * (cosine / (3.14159265 * distance_squared));
        }
    }
    color = vec4(albedo * lighting, 1.0);
}
