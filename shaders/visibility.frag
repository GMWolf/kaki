#version 430

layout(location = 0) in vec2 UV;

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

layout(set = 0, binding = 0) uniform sampler2D albedoTexture;

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float random (in vec2 _st) {
    return fract(sin(dot(_st.xy,
    vec2(12.9898,78.233)))*
    43758.5453123);
}


layout(location = 0) out vec4 outColor;
void main() {

    if (texture(albedoTexture, UV).a < 0.5) {
        discard;
    }

    uint triId = gl_PrimitiveID;

    vec3 c = hsv2rgb(vec3(random(vec2(triId, 1.0)), 0.5 + 0.5 * random(vec2(triId, 1.0)), 1.0));

    outColor = vec4(c, 1);
}
