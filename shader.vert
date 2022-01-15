#version 430

layout(location = 0) out vec2 UV;

layout(push_constant) uniform constants {
    mat4 proj;
    vec2 pos;
    vec2 pad;
    vec3 color;
};

const vec3 positions[6] = vec3[6](
    vec3(-1.f, -1.f, 0.0f),
    vec3( 1.f, -1.f, 0.0f),
    vec3(-1.f,  1.f, 0.0f),
    vec3( 1.f, -1.f, 0.0f),
    vec3(-1.f,  1.f, 0.0f),
    vec3( 1.f,  1.f, 0.0f)
);

void main() {
    gl_Position.x = (gl_VertexIndex == 2 ? 3.0 : -1.0);
    gl_Position.y = (gl_VertexIndex == 0 ? -3.0 : 1.0);
    gl_Position.zw = vec2(1.0);

    gl_Position = vec4(positions[gl_VertexIndex] + vec3(pos, 0.0), 1.0);

    gl_Position = proj * gl_Position;

    UV = positions[gl_VertexIndex].xy / 2.0 + 0.5;
}
