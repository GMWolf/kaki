#version 430

layout(location = 0) in vec2 UV;

struct Transform {
    vec3 position;
    float scale;
    vec4 orientation;
};


layout(push_constant) uniform constants {
    mat4 proj;
    uint drawId;
};

layout(set = 1, binding = 0) uniform sampler2D albedoTexture;

layout(location = 0) out uvec2 outVis;
void main() {

    if (texture(albedoTexture, UV).a < 0.5) {
        discard;
    }

    uint triId = gl_PrimitiveID;

    outVis.x = drawId;
    outVis.y = triId;
}
