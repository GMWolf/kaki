#version 430

layout(location = 0) out vec2 screenPos;
void main() {
    gl_Position.x = (gl_VertexIndex == 2 ? 3.0 : -1.0);
    gl_Position.y = (gl_VertexIndex == 0 ? -3.0 : 1.0);
    gl_Position.zw = vec2(1.0);
    screenPos = gl_Position.xy;
}
