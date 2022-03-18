#version 430

layout(location = 0) in vec2 screenPos;

layout(set = 2, binding = 0) uniform sampler2D albedoTexture;
layout(set = 2, binding = 1) uniform sampler2D normalTexture;
layout(set = 2, binding = 2) uniform sampler2D metallicRoughnessTexture;
layout(set = 2, binding = 3) uniform sampler2D aoTexture;
layout(set = 2, binding = 4) uniform sampler2D emissiveTexture;


layout(set = 0, binding = 0, std430) buffer PositionBlock {
    float v[];
} positions;
layout(set = 0, binding = 1, std430) buffer NormalBlock {
    float v[];
} normals;
layout(set = 0, binding = 2, std430) buffer TangentBlock {
    float v[];
} tangents;
layout(set = 0, binding = 3, std430) buffer TexcoordBlock {
    float v[];
} texcoords;
layout(set = 0, binding = 4, std430) buffer IndexBlock {
    uint v[];
} indices;
layout(set = 1, binding = 5) uniform samplerCube irradianceMap;
layout(set = 1, binding = 6) uniform samplerCube specularMap;
layout(set = 1, binding = 7) uniform sampler2D brdf_lut;


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

struct Transform {
    vec3 position;
    float scale;
    vec4 orientation;
};

struct DrawInfo {
    Transform transform;

    uint indexOffset;
    uint vertexOffset;
    uint material;
    uint pad;
};

vec3 rotate(vec3 vec, vec4 q) {
    vec4 quat = q.yzwx;
    vec3 t = 2 * cross(quat.xyz, vec);
    return vec + quat.w * t + cross(quat.xyz, t);
}

vec3 applyTransform(vec3 pos, Transform transform) {
    return (rotate(pos, transform.orientation) * transform.scale) + transform.position;
}

layout(set = 1, binding = 0) uniform usampler2D visbuffer;
layout(set = 1, binding = 1, std430) buffer DrawInfoBlock {
    DrawInfo data[];
} drawInfoBuffer;

layout(push_constant) uniform constants {
    mat4 proj;
    vec3 viewPos; uint drawId;
    vec3 light; float pad1;
    vec2 winSize;
    uint material;
};


struct BarycentricDeriv
{
    vec3 m_lambda;
    vec3 m_ddx;
    vec3 m_ddy;
};

BarycentricDeriv CalcFullBary(vec4 pt0, vec4 pt1, vec4 pt2, vec2 pixelNdc)
{
    BarycentricDeriv ret;

    vec3 invW = 1.0 / (vec3(pt0.w, pt1.w, pt2.w));

    vec2 ndc0 = pt0.xy * invW.x;
    vec2 ndc1 = pt1.xy * invW.y;
    vec2 ndc2 = pt2.xy * invW.z;

    float invDet = 1.0 / (determinant(mat2x2(ndc2 - ndc1, ndc0 - ndc1)));
    ret.m_ddx = vec3(ndc1.y - ndc2.y, ndc2.y - ndc0.y, ndc0.y - ndc1.y) * invDet;
    ret.m_ddy = vec3(ndc2.x - ndc1.x, ndc0.x - ndc2.x, ndc1.x - ndc0.x) * invDet;

    vec2 deltaVec = pixelNdc - ndc0;
    float interpInvW = (invW.x + deltaVec.x * dot(invW, ret.m_ddx) + deltaVec.y * dot(invW, ret.m_ddy));
    float interpW = 1.0 / (interpInvW);

    ret.m_lambda.x = interpW * (invW[0] + deltaVec.x * ret.m_ddx.x * invW[0] + deltaVec.y * ret.m_ddy.x * invW[0]);
    ret.m_lambda.y = interpW * (0.0f    + deltaVec.x * ret.m_ddx.y * invW[1] + deltaVec.y * ret.m_ddy.y * invW[1]);
    ret.m_lambda.z = interpW * (0.0f    + deltaVec.x * ret.m_ddx.z * invW[2] + deltaVec.y * ret.m_ddy.z * invW[2]);

    ret.m_ddx *= (2.0f/winSize.x);
    ret.m_ddy *= (2.0f/winSize.y);

    //ret.m_ddy *= -1.0f;

    return ret;
}

vec3 InterpolateWithDeriv(BarycentricDeriv deriv, float v0, float v1, float v2)
{
    vec3 mergedV = vec3(v0,v1,v2);
    vec3 ret = vec3(0);
    ret.x = dot(deriv.m_lambda,mergedV);
    ret.y = dot(deriv.m_ddx * mergedV, vec3(1,1,1));
    ret.z = dot(deriv.m_ddy * mergedV, vec3(1,1,1));
    return ret;
}

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

vec3 fresnelRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

struct PBRFragment {
    vec3 albedo;
    float metalicity;
    float roughness;
    vec3 emmisivity;
    vec3 normal;
    float ao;
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

vec3 pbrIrradiance(PBRFragment f, vec3 viewDirection) {

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, f.albedo, f.metalicity);

    vec3 kS = fresnelRoughness(max(dot(f.normal, viewDirection), 0.0), F0, f.roughness);
    vec3 kD = 1.0 - kS;
    vec3 irradiance = textureLod(irradianceMap, f.normal, 0).rgb;
    vec3 diffuse = irradiance * f.albedo;
    vec3 ambient = (kD * diffuse);// * f.ao;

    return ambient;
}

vec3 pbrSpecular(PBRFragment f, vec3 viewDirection) {

    vec3 R = reflect(-viewDirection, f.normal);

    const float MAX_REFLECTION_LOD = 7.0;
    vec3 prefilteredColor = textureLod(specularMap, R,  f.roughness * MAX_REFLECTION_LOD).rgb;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, f.albedo, f.metalicity);

    vec3 F        = fresnelRoughness(max(dot(f.normal, viewDirection), 0.0), F0, f.roughness);
    vec2 envBRDF  = texture(brdf_lut, vec2(max(dot(f.normal, viewDirection), 0.0), 1.0 - f.roughness)).rg;
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    return specular; // * f.ao;
}

layout(location = 0) out vec4 outColor;
void main() {

    uvec2 id = texelFetch(visbuffer, ivec2(gl_FragCoord.xy), 0 ).rg;

    DrawInfo draw = drawInfoBuffer.data[id.x];

    uvec3 tri;
    tri.x = indices.v[draw.indexOffset + id.y * 3 + 0] + draw.vertexOffset;
    tri.y = indices.v[draw.indexOffset + id.y * 3 + 1] + draw.vertexOffset;
    tri.z = indices.v[draw.indexOffset + id.y * 3 + 2] + draw.vertexOffset;

    vec3 inpos0 = vec3(positions.v[tri.x * 3 + 0], positions.v[tri.x * 3 + 1], positions.v[tri.x * 3 + 2]);
    vec3 inpos1 = vec3(positions.v[tri.y * 3 + 0], positions.v[tri.y * 3 + 1], positions.v[tri.y * 3 + 2]);
    vec3 inpos2 = vec3(positions.v[tri.z * 3 + 0], positions.v[tri.z * 3 + 1], positions.v[tri.z * 3 + 2]);

    vec4 pos0 = vec4(applyTransform(inpos0, draw.transform), 1);
    vec4 pos1 = vec4(applyTransform(inpos1, draw.transform), 1);
    vec4 pos2 = vec4(applyTransform(inpos2, draw.transform), 1);

    BarycentricDeriv bary = CalcFullBary(proj * pos0, proj * pos1, proj * pos2, screenPos);

    vec3 position = vec3(InterpolateWithDeriv(bary, pos0.x, pos1.x, pos2.x).x,
                         InterpolateWithDeriv(bary, pos0.y, pos1.y, pos2.y).x,
                         InterpolateWithDeriv(bary, pos0.z, pos1.z, pos2.z).x);

    vec2 uv0 = vec2(texcoords.v[tri.x * 2 + 0], texcoords.v[tri.x * 2 + 1]);
    vec2 uv1 = vec2(texcoords.v[tri.y * 2 + 0], texcoords.v[tri.y * 2 + 1]);
    vec2 uv2 = vec2(texcoords.v[tri.z * 2 + 0], texcoords.v[tri.z * 2 + 1]);

    vec3 uvx = InterpolateWithDeriv(bary, uv0.x, uv1.x, uv2.x);
    vec3 uvy = InterpolateWithDeriv(bary, uv0.y, uv1.y, uv2.y);

    vec2 uv = vec2(uvx.x, uvy.x);
    vec2 duv_dx = vec2(uvx.y, uvy.y);
    vec2 duv_dy = vec2(uvx.z, uvy.z);

    vec3 normal0 = vec3(normals.v[tri.x * 3 + 0], normals.v[tri.x * 3 + 1], normals.v[tri.x * 3 + 2]);
    vec3 normal1 = vec3(normals.v[tri.y * 3 + 0], normals.v[tri.y * 3 + 1], normals.v[tri.y * 3 + 2]);
    vec3 normal2 = vec3(normals.v[tri.z * 3 + 0], normals.v[tri.z * 3 + 1], normals.v[tri.z * 3 + 2]);
    vec3 normal = vec3(InterpolateWithDeriv(bary, normal0.x, normal1.x, normal2.x).x,
                       InterpolateWithDeriv(bary, normal0.y, normal1.y, normal2.y).x,
                       InterpolateWithDeriv(bary, normal0.z, normal1.z, normal2.z).x);
    normal = rotate(normal, draw.transform.orientation);

    vec4 tangent0 = vec4(tangents.v[tri.x * 4 + 0], tangents.v[tri.x * 4 + 1], tangents.v[tri.x * 4 + 2], tangents.v[tri.x * 4 + 3]);
    vec4 tangent1 = vec4(tangents.v[tri.y * 4 + 0], tangents.v[tri.y * 4 + 1], tangents.v[tri.y * 4 + 2], tangents.v[tri.y * 4 + 3]);
    vec4 tangent2 = vec4(tangents.v[tri.z * 4 + 0], tangents.v[tri.z * 4 + 1], tangents.v[tri.z * 4 + 2], tangents.v[tri.z * 4 + 3]);
    vec3 tangent = vec3(InterpolateWithDeriv(bary, tangent0.x, tangent1.x, tangent2.x).x,
                        InterpolateWithDeriv(bary, tangent0.y, tangent1.y, tangent2.y).x,
                        InterpolateWithDeriv(bary, tangent0.z, tangent1.z, tangent2.z).x);
    tangent = rotate(tangent, draw.transform.orientation);

    vec3 bitangent = tangent0.w * cross(normal, tangent);

    vec3 t_normal = textureGrad(normalTexture, uv, duv_dx, duv_dy).xyz * 2.0 - 1.0;
    t_normal.z = sqrt(1 - dot(t_normal.xy, t_normal.xy));

    PBRFragment m;
    m.albedo = textureGrad(albedoTexture, uv, duv_dx, duv_dy).rgb;
    m.metalicity = textureGrad(metallicRoughnessTexture, uv, duv_dx, duv_dy).r;
    m.roughness = textureGrad(metallicRoughnessTexture, uv, duv_dx, duv_dy).g;
    m.emmisivity = textureGrad(emissiveTexture, uv, duv_dx, duv_dy).rgb;
    m.normal = mat3(normalize(tangent), normalize(bitangent), normalize(normal)) * t_normal;
    m.ao = textureGrad(aoTexture, uv, duv_dx, duv_dy).x;

    vec3 v = normalize(viewPos - position);
    LightFragment l;
    l.lightDirection = light;
    l.radiance = 4 * vec3(2, 1.7, 1.5);

    vec3 c = pbrColor(m, l, v);

    c += pbrIrradiance(m, v);

    c += pbrSpecular(m,v);

    //c = texture(specularMap, m.normal).rgb;


    //vec3 F        = fresnelRoughness(max(dot(m.normal, v), 0.0), F0, m.roughness);

    //c = F;

    c += m.emmisivity;

    c *= 0.5;

    // = tangent;
    outColor = vec4(c, 1.0);
}
