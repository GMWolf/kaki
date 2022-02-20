#version 430

struct Transform {
    vec3 position;
    float scale;
    vec4 orientation;
};

layout(location = 0) in vec2 UV;
layout(location = 1) in vec3 NORMAL;
layout(location = 2) in vec4 TANGENT;
layout(location = 3) in vec3 VIEW_DIRECTION;

layout(push_constant) uniform constants {
    mat4 proj;
    vec3 viewPos; float pad0;
    Transform transform;
    vec3 light; float pad1;
};

layout(set = 0, binding = 0) uniform sampler2D albedoTexture;
layout(set = 0, binding = 1) uniform sampler2D normalTexture;
layout(set = 0, binding = 2) uniform sampler2D metallicRoughnessTexture;
layout(set = 0, binding = 3) uniform sampler2D aoTexture;
layout(location = 0) out vec4 outColor;

#define PI 3.14159265358979

float D_GGX(vec3 N, vec3 H, float a) {
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float nom = a2;
    float denom = (NdotH2*(a2- 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float G_GGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float G_smith(vec3 N, vec3 V, vec3 L, float K) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = G_GGX(NdotV, K);
    float ggx2 = G_GGX(NdotL, K);
    return ggx1 * ggx2;
}

vec3 fresnel(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0-cosTheta, 5.0);
}

struct PBRFragment {
    vec3 albedo;
    float metalicity;
    float roughness;
    vec3 emmisivity;
    vec3 normal;
};

struct LightFragment {
    vec3 lightDirection;
    vec3 radiance;
};

vec3 pbrColor(PBRFragment f, LightFragment l, vec3 viewDirection) {

    vec3 H = normalize(l.lightDirection + viewDirection);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, f.albedo, f.metalicity);

    vec3 F = fresnel(max(dot(H, viewDirection), 0.0), F0);
    float NDF = D_GGX(f.normal, H, f.roughness);
    float G = G_smith(f.normal, viewDirection, l.lightDirection, f.roughness);
    vec3 numerator = NDF * G * F;

    float denom = 4.0 * max(dot(f.normal, viewDirection), 0.0) * max(dot(f.normal, l.lightDirection), 0.0) + 0.0001;
    vec3 specular = numerator / max(denom, 0.0001);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - f.metalicity;

    float NdotL = max(dot(f.normal, l.lightDirection), 0.0);

    vec3 c;
    c = (kD * f.albedo / PI + specular) * l.radiance * NdotL;
    c += f.emmisivity;

    return c;
}


void main() {


    vec4 albedoA = texture(albedoTexture, UV).rgba;

    if (albedoA.a < 0.25) {
        discard;
    }

    vec3 albedo = albedoA.rgb;

    vec3 bitangent = TANGENT.w * cross(NORMAL, TANGENT.xyz);

    vec3 tnormal = texture(normalTexture, UV).xyz * 2.0 - 1.0;
    //tnormal.y *= -1;
    tnormal.z = sqrt(1 - dot(tnormal.xy, tnormal.xy));

    //vec3 normal = normalize(tnormal.x * TANGENT.xyz + tnormal.y * bitangent + tnormal.z * NORMAL);
    vec3 normal = mat3(normalize(TANGENT.xyz), normalize(bitangent), normalize(NORMAL)) * tnormal;
    //normal.y *= -1;

    //normal = NORMAL;
    vec2 metallicRoughness = texture(metallicRoughnessTexture, UV).bg;

    float ao = texture(aoTexture, UV).r;

    PBRFragment m;
    m.albedo = albedo;
    m.metalicity = metallicRoughness.r;
    m.roughness = metallicRoughness.g;
    m.emmisivity = vec3(0);
    m.normal = normal;

    LightFragment l;
    l.lightDirection = light;
    l.radiance = 3 * vec3(2, 1.7, 1.5);

    vec3 v = normalize(VIEW_DIRECTION);

    vec3 c = pbrColor(m, l, v) + albedo * vec3(0.03, 0.03, 0.05);

    outColor = vec4(c, 1);
}
