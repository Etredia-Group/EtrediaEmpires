#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 position;

layout(binding = 0) uniform Camera {
    mat4 viewProjection;
} camera;

layout(push_constant) uniform Model {
    mat4 model;
} model;

struct Province {
    uint regionId;
    uint color;
};
layout(std430, binding = 1) readonly buffer Data {
    Province provinces[];
};

layout(location = 0) out vec3 fragColor;

vec3 unpackColor(uint c) {
    return vec3(
        c & 0xFFu,
        (c >> 8) & 0xFFu,
        (c >> 16) & 0xFFu
    ) / 255.0;
}

void main() {
    gl_Position = camera.viewProjection * vec4(position, 0.0, 1.0);
    fragColor = unpackColor(provinces[gl_InstanceIndex].color);
}