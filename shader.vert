#version 430

layout(location = 0) out vec2 UV;

void main() {
    gl_Position.x = (gl_VertexIndex == 2 ? 3.0 : -1.0);
    gl_Position.y = (gl_VertexIndex == 0 ? -3.0 : 1.0);
    gl_Position.zw = vec2(1.0);

    UV = gl_Position.xy / 2.0 + 0.5;
}
