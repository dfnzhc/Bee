#version 460
#extension GL_EXT_ray_tracing : require
layout(set = 0, binding = 2, std430) readonly buffer SceneData { vec4 values[]; } scene;
struct Payload { vec4 position; vec4 normal; vec4 albedo; };
layout(location = 0) rayPayloadInEXT Payload payload;
hitAttributeEXT vec2 barycentrics;
void main() {
    uint base = uint(gl_PrimitiveID) * 9u;
    vec3 weights = vec3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
    vec3 normal = scene.values[base + 1].xyz * weights.x +
                  scene.values[base + 4].xyz * weights.y +
                  scene.values[base + 7].xyz * weights.z;
    if (dot(normal, normal) < 0.000000000001) {
        normal = cross(scene.values[base + 3].xyz - scene.values[base].xyz,
                       scene.values[base + 6].xyz - scene.values[base].xyz);
    }
    normal = normalize(normal);
    if (dot(normal, gl_WorldRayDirectionEXT) > 0.0) normal = -normal;
    payload.position = vec4(gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT, 1.0);
    payload.normal = vec4(normal, 0.0);
    payload.albedo = vec4(scene.values[base + 2].xyz * weights.x +
                          scene.values[base + 5].xyz * weights.y +
                          scene.values[base + 8].xyz * weights.z, 0.0);
}
