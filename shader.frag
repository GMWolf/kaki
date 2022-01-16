#version 430

layout(location = 0) in vec2 UV;

layout(push_constant) uniform constants {
    mat4 proj;
    vec2 pos;
    vec2 pad;
    vec3 color;
};

layout(set = 0, binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(tex, UV) * vec4(color, 1);
}
