#version 430

struct Transform {
    vec3 position;
    float scale;
    vec4 orientation;
};

layout(location = 0) in vec2 UV;

layout(push_constant) uniform constants {
    mat4 proj;
    Transform transform;
    vec3 color;
};

layout(set = 0, binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 outColor;

void main() {
    outColor =  vec4(texture(tex, UV).rgb * color, 1);
}
