#version 430

layout(location = 0) in vec2 UV;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(UV, 1.0, 1.0);
}
