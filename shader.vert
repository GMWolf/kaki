#version 430


layout(location = 0) in vec3 POSITION;
layout(location = 1) in vec3 NORMAL;
layout(location = 2) in vec2 UV;

layout(location = 0) out vec2 UV_OUT;

struct Transform {
    vec3 position;
    float scale;
    vec4 orientation;
};

layout(push_constant) uniform constants {
    mat4 proj;
    Transform transform;
    vec3 color;
};

vec3 rotate(vec3 vec, vec4 quat) {
    vec3 t = 2 * cross(quat.xyz, vec);
    return vec + quat.w * t + cross(quat.xyz, t);
}

vec3 applyTransform(vec3 pos, Transform transform) {
    return (rotate(pos, transform.orientation) * transform.scale) + transform.position;
}

void main() {
    vec3 pos = applyTransform(POSITION, transform);
    gl_Position = proj * vec4(pos , 1.0);
    UV_OUT = UV;
}
