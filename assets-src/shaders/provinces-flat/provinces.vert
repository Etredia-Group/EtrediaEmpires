#version 450
#extension GL_ARB_separate_shader_objects : enable

// Per-vertex data: contour point of a province polygon and its outward miter normal.
// The ocean plane is drawn through the same multi-draw with a zero normal.
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 normal;

layout(std140, binding = 0) uniform Camera {
    mat4 viewProjection;
    mat4 inverseViewProjection;
    vec4 viewport;   // x, y: framebuffer size; z: world units per screen pixel; w: 1 if target is sRGB
    vec4 raster;     // x, y: world position of province raster texel (0, 0) center; z, w: raster size in texels
    vec4 coast;      // x: map pixels per raster texel; y: raster texels per coast field texel; z, w: coast field size
    uvec4 info;      // x: instance index of the ocean plane
} camera;

layout(location = 0) out vec2 worldPos;
layout(location = 1) flat out uint instance;

void main() {
    // Province polygons are simplified pixel contours, so they may miss up to ~1.7 map pixels of
    // their own area, plus up to a raster texel lost to rounding. They are pushed outwards and the
    // fragment shader clips them exactly against the province raster, which also closes the seams
    // between neighbouring provinces.
    float coverageExpand = 2.0 + 1.75 * camera.coast.x;
    vec2 world = position + normal * coverageExpand;

    worldPos = world;
    instance = uint(gl_InstanceIndex);
    gl_Position = camera.viewProjection * vec4(world, 0.0, 1.0);
}
