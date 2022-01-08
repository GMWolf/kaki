#version 430

layout(location = 0) in vec2 UV;

layout(push_constant) uniform constants {
    mat4 proj;
    vec2 pos;
    vec2 pad;
    vec3 color;
};

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(pow(color, vec3(2.2)), 1.0);
}
