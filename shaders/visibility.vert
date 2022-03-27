#version 430

layout(location = 0) out vec2 UV_OUT;

layout(set = 0, binding = 0, std430) buffer PositionBlock {
    float v[];
} positions;
layout(set = 0, binding = 3, std430) buffer TexcoordBlock {
    float v[];
} texcoords;

struct Transform {
    vec3 position;
    float scale;
    vec4 orientation;
};

struct DrawInfo {
    uint transformOffset;
    uint indexOffset;
    uint vertexOffset;
    uint material;
    uint pad;
};

layout(set = 0, binding = 4, std430) buffer DrawInfoBlock {
    DrawInfo data[];
} drawInfoBuffer;

layout(set = 0, binding = 5, std430) buffer TransformBlock {
    Transform transform[];
} transformBuffer;

layout(push_constant) uniform constants {
    mat4 proj;
    uint drawIndex;
};

vec3 rotate(vec3 vec, vec4 q) {
    vec4 quat = q.yzwx;
    vec3 t = 2 * cross(quat.xyz, vec);
    return vec + quat.w * t + cross(quat.xyz, t);
}

vec3 applyTransform(vec3 pos, Transform transform) {
    return (rotate(pos, transform.orientation) * transform.scale) + transform.position;
}

void main() {

    DrawInfo drawInfo = drawInfoBuffer.data[drawIndex];

    vec3 in_pos = vec3(positions.v[gl_VertexIndex * 3 + 0u], positions.v[gl_VertexIndex * 3 + 1u], positions.v[gl_VertexIndex * 3 + 2u]);
    vec2 in_uv = vec2(texcoords.v[gl_VertexIndex * 2 + 0u], texcoords.v[gl_VertexIndex * 2 + 1u]);

    vec3 pos = applyTransform(in_pos, transformBuffer.transform[drawInfo.transformOffset]);
    gl_Position = proj * vec4(pos , 1.0);
    UV_OUT = in_uv;
}
