struct Province {
    uint regionId;
    uint color;
};
layout(std430, binding=0) buffer Data {
    Province provinces[];
};


layout(location = 0) in vec2 position;
layout(location = 1) uniform mat4 camera;
layout(location = 2) uniform mat4 transform;
layout(location = 3) uniform uint provinceIndex;

layout(location = 0) out vec3 fragColor;

vec3 unpackColor(uint c) {
    return vec3(
        c & 0xFFu,
        (c >> 8) & 0xFFu,
        (c >> 16) & 0xFFu
    ) / 255.0;
}

void main() {
    gl_Position = camera * vec4(position, 0.0, 1.0);
    fragColor = unpackColor(provinces[provinceIndex].color);
}