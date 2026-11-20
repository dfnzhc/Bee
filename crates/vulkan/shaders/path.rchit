#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference : require
layout(buffer_reference, std430, buffer_reference_align = 4) readonly buffer Mesh {
    vec4 values[];
};
struct Instance {
    Mesh mesh;
    uint material;
};
layout(set = 0, binding = 2, std430) readonly buffer Instances {
    Instance values[];
} instances;
layout(set = 0, binding = 3, std430) readonly buffer Materials {
    vec4 values[];
} materials;
struct Payload { vec4 position; vec4 normal; vec4 albedo; };
layout(location = 0) rayPayloadInEXT Payload payload;
hitAttributeEXT vec2 barycentrics;
void main() {
    Instance instance = instances.values[gl_InstanceCustomIndexEXT];
    uint base = uint(gl_PrimitiveID) * 6u;
    vec3 weights = vec3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
    mat3 normal_transform = transpose(mat3(gl_WorldToObjectEXT));
    vec3 normal = normalize(normal_transform * instance.mesh.values[base + 1].xyz) * weights.x +
                  normalize(normal_transform * instance.mesh.values[base + 3].xyz) * weights.y +
                  normalize(normal_transform * instance.mesh.values[base + 5].xyz) * weights.z;
    if (dot(normal, normal) < 0.000000000001) {
        mat3 model = mat3(gl_ObjectToWorldEXT);
        vec3 a = instance.mesh.values[base].xyz;
        normal = cross(model * (instance.mesh.values[base + 2].xyz - a),
                       model * (instance.mesh.values[base + 4].xyz - a));
    }
    normal = normalize(normal);
    if (dot(normal, gl_WorldRayDirectionEXT) > 0.0) normal = -normal;
    payload.position = vec4(gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT, 1.0);
    payload.normal = vec4(normal, 0.0);
    payload.albedo = vec4(materials.values[instance.material].xyz, 0.0);
}
