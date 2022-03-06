#version 430

layout(location = 0) out vec2 UV_OUT;
layout(location = 1) out vec3 NORMAL_OUT;
layout(location = 2) out vec4 TANGENT_OUT;
layout(location = 3) out vec3 VIEW_DIRECTION;

layout(set = 1, binding = 0, std430) buffer PositionBlock {
    float v[];
} positions;
layout(set = 1, binding = 1, std430) buffer NormalBlock {
    float v[];
} normals;
layout(set = 1, binding = 2, std430) buffer TangentBlock {
    float v[];
} tangents;
layout(set = 1, binding = 3, std430) buffer TexcoordBlock {
    float v[];
} texcoords;

struct Transform {
    vec3 position;
    float scale;
    vec4 orientation;
};

layout(push_constant) uniform constants {
    mat4 proj;
    vec3 viewPos; float pad0;
    Transform transform;
    vec3 light; float pad1;
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

    vec3 in_pos = vec3(positions.v[gl_VertexIndex * 3 + 0u], positions.v[gl_VertexIndex * 3 + 1u], positions.v[gl_VertexIndex * 3 + 2u]);
    vec3 in_normal = vec3(normals.v[gl_VertexIndex * 3 + 0u], normals.v[gl_VertexIndex * 3 + 1u], normals.v[gl_VertexIndex * 3 + 2u]);
    vec4 in_tangent = vec4(tangents.v[gl_VertexIndex * 4 + 0u], tangents.v[gl_VertexIndex * 4 + 1u], tangents.v[gl_VertexIndex * 4 + 2u], tangents.v[gl_VertexIndex * 4 + 3u]);
    vec2 in_uv = vec2(texcoords.v[gl_VertexIndex * 2 + 0u], texcoords.v[gl_VertexIndex * 2 + 1u]);

    vec3 pos = applyTransform(in_pos, transform);
    gl_Position = proj * vec4(pos , 1.0);
    UV_OUT = in_uv;
    NORMAL_OUT = rotate(in_normal, transform.orientation);
    TANGENT_OUT = vec4(rotate(in_tangent.xyz, transform.orientation), in_tangent.w);
    VIEW_DIRECTION = normalize(pos - viewPos);
}
