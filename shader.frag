#version 430

layout(location = 0) in vec2 UV;

layout(push_constant) uniform constants {
    float time;
};

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(UV, sin(time) * 0.5 + 0.5, 1.0);
}
