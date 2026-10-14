#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 1) uniform sampler2D Texture;
layout (location = 0) in vec2 vUV;
layout (location = 1) in vec4 vColor;

layout (location = 0) out vec4 outColor;

// RmlUi works in sRGB with premultiplied alpha: vertex colours from the styles and generated textures
// (font glyphs) are premultiplied sRGB values. The swapchain is sRGB, so the GPU expects linear output
// and encodes it to sRGB on write; without conversion colours would be encoded twice and look washed out.

vec3 srgbToLinear(vec3 c) {
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), c));
}

vec3 linearToSrgb(vec3 c) {
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), c));
}

void main() {
    // Textures are uploaded as R8G8B8A8_SRGB, so sampling has already decoded them to linear;
    // bring them back to the sRGB values RmlUi generated before combining with the vertex colour.
    vec4 texColor = texture(Texture, vUV);
    texColor.rgb = linearToSrgb(texColor.rgb);

    vec4 color = vColor * texColor;   // premultiplied sRGB
    if (color.a <= 0.0) discard;

    // The pipeline blends with SRC_ALPHA / ONE_MINUS_SRC_ALPHA, so output straight (non-premultiplied) linear colour.
    vec3 straight = clamp(color.rgb / color.a, 0.0, 1.0);
    outColor = vec4(srgbToLinear(straight), color.a);
}
