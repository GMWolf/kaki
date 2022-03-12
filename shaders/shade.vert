#version 430

layout(push_constant) uniform constants {
    mat4 proj;
    vec3 viewPos; uint drawId;
    vec3 light; float pad1;
    vec2 winSize;
    uint material;
};

layout(location = 0) out vec2 screenPos;
void main() {
    gl_Position.x = (gl_VertexIndex == 2 ? 3.0 : -1.0);
    gl_Position.y = (gl_VertexIndex == 0 ? -3.0 : 1.0);
    gl_Position.z = material / float((1 << 16) - 1);
    gl_Position.w = 1.0;
    screenPos = gl_Position.xy;
}
