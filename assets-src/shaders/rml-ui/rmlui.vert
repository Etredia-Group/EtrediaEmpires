#version 330

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(push_constant) uniform Transform
{
    mat4 mvp;
    vec2 translate;
} transform;

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec2 vUV;
layout (location = 1) out vec4 vColor;

void main() {
    vUV = inUV;
    vColor = inColor;
    vec2 translatedPos = inPosition + transform.translate.xy;
    vec4 outPos = transform.mvp * vec4(translatedPos, 0, 1);
    gl_Position = outPos;
}