#version 450

layout(set = 0, binding = 0) uniform sampler2D scene_color;
layout(push_constant) uniform Parameters { float exposure_ev; } parameters;
layout(location = 0) out vec4 display_color;

void main() {
    vec4 source = texelFetch(scene_color, ivec2(gl_FragCoord.xy), 0);
    vec3 linear = max(source.rgb * exp2(clamp(parameters.exposure_ev, -126.0, 126.0)), vec3(0.0));
    vec3 mapped = vec3(1.0) - vec3(1.0) / (vec3(1.0) + linear);
    vec3 srgb = mix(1.055 * pow(mapped, vec3(1.0 / 2.4)) - 0.055,
                    12.92 * mapped, lessThanEqual(mapped, vec3(0.0031308)));
    display_color = vec4(srgb, clamp(source.a, 0.0, 1.0));
}
