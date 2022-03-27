#version 430

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

layout(set = 0, binding = 0) uniform usampler2D visbuffer;
layout(set = 0, binding = 1, std430) buffer DrawInfoBlock {
    DrawInfo data[];
} drawInfoBuffer;

void main() {

    uvec2 id = texelFetch(visbuffer, ivec2(gl_FragCoord.xy), 0 ).rg;
    DrawInfo draw = drawInfoBuffer.data[id.x];

    gl_FragDepth = draw.material / float((1 << 16) - 1);

}
