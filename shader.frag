#version 430

struct Transform {
    vec3 position;
    float scale;
    vec4 orientation;
};

layout(push_constant) uniform constants {
    mat4 proj;
    Transform transform;
    vec3 color;
};

layout(location = 0) out vec4 outColor;

void main() {
    outColor =  vec4(color, 1);
}
