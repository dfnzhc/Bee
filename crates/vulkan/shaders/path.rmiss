#version 460
#extension GL_EXT_ray_tracing : require
struct Payload { vec4 position; vec4 normal; vec4 albedo; };
layout(location = 0) rayPayloadInEXT Payload payload;
void main() { payload.position.w = 0.0; }
