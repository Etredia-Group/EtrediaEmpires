#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 worldPos;
layout(location = 1) flat in uint instance;

layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform Camera {
    mat4 viewProjection;
    mat4 inverseViewProjection;
    vec4 viewport;   // x, y: framebuffer size; z: world units per screen pixel; w: 1 if target is sRGB
    vec4 raster;     // x, y: world position of province raster texel (0, 0) center; z, w: raster size in texels
    vec4 coast;      // x: map pixels per raster texel; y: raster texels per coast field texel; z, w: coast field size
    uvec4 info;      // x: instance index of the ocean plane
} camera;

struct Province {
    uint regionId;
    uint color;
};
layout(std430, binding = 1) readonly buffer Data {
    Province provinces[];
};

// Province index + 1 for every raster texel, 0 for water.
layout(binding = 2) uniform usampler2D provinceIds;
// Signed distance to the coastline in raster texels: positive in water, negative on land.
layout(binding = 3) uniform sampler2D coastDistance;

// ---------------------------------------------------------------------------------------------
// Palette (sRGB)
// ---------------------------------------------------------------------------------------------

const vec3 OCEAN_SHELF_NEAR = vec3( 88.0, 148.0, 204.0) / 255.0;
const vec3 OCEAN_SHELF_MID  = vec3( 60.0, 128.0, 196.0) / 255.0;
const vec3 OCEAN_SHELF_FAR  = vec3( 38.0, 113.0, 189.0) / 255.0;
const vec3 OCEAN_DEEP       = vec3( 20.0,  98.0, 183.0) / 255.0;
const vec3 OCEAN_ABYSS      = vec3( 15.0,  86.0, 168.0) / 255.0;

// All distances below are in province raster texels ("map art pixels").

// Terraces of the coastal shelf, distance from the shore.
const float SHELF_NEAR_END = 2.5;
const float SHELF_MID_END  = 4.5;
const float SHELF_FAR_FADE_START = 6.5;
const float SHELF_FAR_FADE_END   = 16.0;

const float GRID_SPACING = 32.0;
const float GRID_STRENGTH_OCEAN = 0.07;
const float GRID_STRENGTH_LAND  = 0.10;

const int   COUNTRY_SHADE_RADIUS = 3;
const float COUNTRY_SHADE = 0.82;

// ---------------------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------------------

vec3 unpackColor(uint c) {
    return vec3(
        c & 0xFFu,
        (c >> 8) & 0xFFu,
        (c >> 16) & 0xFFu
    ) / 255.0;
}

vec3 srgbToLinear(vec3 c) {
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), c));
}

vec3 saturateColor(vec3 c, float amount) {
    float luma = dot(c, vec3(0.2126, 0.7152, 0.0722));
    return clamp(mix(vec3(luma), c, amount), 0.0, 1.0);
}

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float valueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);

    float a = hash12(i);
    float b = hash12(i + vec2(1.0, 0.0));
    float c = hash12(i + vec2(0.0, 1.0));
    float d = hash12(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 4; ++i) {
        value += amplitude * valueNoise(p);
        p = p * 2.03 + vec2(17.1, 9.7);
        amplitude *= 0.5;
    }
    return value / 0.9375;
}

// Raster texels per screen pixel.
float artPixelSize() {
    return camera.viewport.z / camera.coast.x;
}

// Map-space grid lines with a constant on-screen thickness, faded out when they get too dense.
float gridMask(vec2 art) {
    float pixel = artPixelSize();
    vec2 cell = abs(fract(art / GRID_SPACING + 0.5) - 0.5) * GRID_SPACING / pixel;
    float line = 1.0 - smoothstep(0.6, 1.6, min(cell.x, cell.y));
    float density = smoothstep(10.0, 24.0, GRID_SPACING / pixel);
    return line * density;
}

uint fetchProvince(ivec2 texel) {
    texel = clamp(texel, ivec2(0), ivec2(camera.raster.zw) - 1);
    return texelFetch(provinceIds, texel, 0).r;
}

uint ownerColor(uint id) {
    return provinces[id - 1u].color & 0xFFFFFFu;
}

// ---------------------------------------------------------------------------------------------
// Ocean
// ---------------------------------------------------------------------------------------------

vec3 shadeOcean(vec2 art) {
    vec2 uv = (art / camera.coast.y + 0.5) / camera.coast.zw;
    float shoreDistance = texture(coastDistance, uv).r;

    // Ragged terrace edges instead of perfect offset curves of the coastline.
    float d = shoreDistance + (fbm(art * 0.15) - 0.5) * 1.6;
    float edge = 0.5 * max(artPixelSize(), 0.75);

    // Large, soft deep-water patches far away from the shore.
    float abyss = smoothstep(0.45, 0.8, fbm(art * 0.012)) * smoothstep(10.0, 30.0, shoreDistance);
    vec3 color = mix(OCEAN_DEEP, OCEAN_ABYSS, abyss);

    color = mix(OCEAN_SHELF_FAR, color, smoothstep(SHELF_FAR_FADE_START, SHELF_FAR_FADE_END, d));
    color = mix(OCEAN_SHELF_MID, color, smoothstep(SHELF_MID_END - edge, SHELF_MID_END + edge, d));
    color = mix(OCEAN_SHELF_NEAR, color, smoothstep(SHELF_NEAR_END - edge, SHELF_NEAR_END + edge, d));

    color *= 1.0 + (valueNoise(art * 0.35) - 0.5) * 0.025;
    color *= 1.0 - GRID_STRENGTH_OCEAN * gridMask(art);
    return color;
}

// ---------------------------------------------------------------------------------------------
// Land
// ---------------------------------------------------------------------------------------------

const ivec2 DIRECTIONS[8] = ivec2[](
    ivec2(1, 0), ivec2(-1, 0), ivec2(0, 1), ivec2(0, -1),
    ivec2(1, 1), ivec2(-1, 1), ivec2(1, -1), ivec2(-1, -1)
);

vec3 shadeLand(vec2 art, ivec2 texel, uint ownId) {
    uint ownColorBits = ownerColor(ownId);
    vec3 base = unpackColor(ownColorBits);

    // When zoomed out a raster texel gets smaller than a screen pixel; widen borders to stay visible.
    int stride = max(1, int(artPixelSize() + 0.5));

    bool coast = false;
    bool provinceBorder = false;
    bool countryBorder = false;
    float countryDistance = 1.0e6;

    for (int d = 0; d < 8; ++d) {
        bool axis = d < 4;
        float stepLength = axis ? 1.0 : 1.41421356;

        for (int r = 1; r <= COUNTRY_SHADE_RADIUS; ++r) {
            uint neighbour = fetchProvince(texel + DIRECTIONS[d] * (r * stride));
            if (neighbour == ownId) continue;

            if (neighbour == 0u) {
                coast = coast || (axis && r == 1);
                continue;
            }

            if (ownerColor(neighbour) != ownColorBits) {
                countryDistance = min(countryDistance, float(r) * stepLength);
                countryBorder = countryBorder || (axis && r == 1);
                break;
            }

            provinceBorder = provinceBorder || (axis && r == 1);
        }
    }

    // Subtle per-pixel grain plus low-frequency tint variation.
    float grain = (hash12(vec2(texel)) - 0.5) * 0.035 + (valueNoise(art * 0.05) - 0.5) * 0.04;
    vec3 color = base * (1.0 + grain);

    if (provinceBorder) color = base * 0.84;
    if (coast) color = saturateColor(base, 1.15) * 0.72;

    color *= mix(COUNTRY_SHADE, 1.0, smoothstep(1.0, float(COUNTRY_SHADE_RADIUS) + 0.5, countryDistance));

    if (countryBorder) color = saturateColor(base, 1.6) * 0.9;

    color *= 1.0 - GRID_STRENGTH_LAND * gridMask(art);
    return color;
}

void main() {
    vec3 color;
    // Position in province raster texels
    vec2 art = (worldPos - camera.raster.xy) / camera.coast.x;

    if (instance == camera.info.x) {
        color = shadeOcean(art);
    }
    else {
        ivec2 texel = ivec2(floor(art + 0.5));
        uint ownId = fetchProvince(texel);

        // Pixel-exact province coverage (see COVERAGE_EXPAND in the vertex shader).
        if (ownId != instance + 1u) discard;

        color = shadeLand(art, texel, ownId);
    }

    outColor = vec4(camera.viewport.w > 0.5 ? srgbToLinear(color) : color, 1.0);
}
