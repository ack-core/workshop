
#include "scene.h"

#include <cfloat>
#include <memory>
#include <vector>
#include <unordered_map>

#include "thirdparty/upng/upng.h"

namespace {
    const char *g_voxelMapShaderSrc = R"(
        fixed {
            cube[8] : float4 =
                [-0.5, -0.5, -0.5, 1.0][0.5, -0.5, -0.5, 1.0][-0.5, -0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0]
                [-0.5, 0.5, -0.5, 1.0][0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
        }
        const {
            positionRadius : float4
            bounds : float4
            color : float4
        }
        inout {
            discard : float1
        }
        vssrc {
            float3 cubeCenter = float3(instance_position.xyz) - const_positionRadius.xyz;
            float4 absVertexPos = float4(cubeCenter, 0.0) + fixed_cube[vertex_ID];
            
            int ypos = int(absVertexPos.y + const_bounds.y);
            int ybase = int(const_bounds.z);
            float2 fromLight = absVertexPos.xz + const_bounds.yy + float2(ypos % ybase, ypos / ybase) * 2.0f * const_bounds.yy;
            
            output_position = float4((fromLight - const_bounds.xx) / float2(const_bounds.x, -const_bounds.x), 0.5, 1.0);
            output_discard = _step(_dot(cubeCenter, cubeCenter), const_positionRadius.w * const_positionRadius.w);
        }
        fssrc {
            output_color[0] = float4(0.33, 0.0, 0.0, input_discard);
        }
    )";
    
    const char *g_staticMeshGBufferShaderSrc = R"(
        fixed {
            axis[3] : float3 =
                [0.0, 0.0, 1.0]
                [1.0, 0.0, 0.0]
                [0.0, 1.0, 0.0]
                
            bnm[3] : float3 =
                [1.0, 0.0, 0.0]
                [0.0, 0.0, 1.0]
                [1.0, 0.0, 0.0]

            tgt[3] : float3 =
                [0.0, 1.0, 0.0]
                [0.0, 1.0, 0.0]
                [0.0, 0.0, 1.0]

            cube[18] : float4 =
                [-0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0]
                [0.5, -0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0]

            faceUV[4] : float2 =
                [0.0, 0.0][1.0, 0.0][0.0, 1.0][1.0, 1.0]
        }
        inout {
            corner : float3
            uvi : float3
        }
        vssrc {
            float3 cubeCenter = float3(instance_position_color.xyz);
            float3 toCamSign = _sign(frame_cameraPosition.xyz - cubeCenter);
            float4 relVertexPos = float4(toCamSign, 1.0) * fixed_cube[vertex_ID];
            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
            float3 faceNormal = toCamSign * fixed_axis[vertex_ID / 6];

            int3 faceIndices = int3(2.0 * _step(0.0, relVertexPos.yzy) + _step(0.0, relVertexPos.zxx));

            output_uvi = float3(0, 0, 0);
            output_uvi.xy = output_uvi.xy + _abs(faceNormal.x) * fixed_faceUV[faceIndices.x];
            output_uvi.xy = output_uvi.xy + _abs(faceNormal.y) * fixed_faceUV[faceIndices.y];
            output_uvi.xy = output_uvi.xy + _abs(faceNormal.z) * fixed_faceUV[faceIndices.z];
            
            float3 dirB = fixed_bnm[vertex_ID / 6];
            float3 dirT = fixed_tgt[vertex_ID / 6];
            
            output_corner.xyz = cubeCenter - 0.5 * (dirB + dirT - faceNormal);
            output_uvi.z = 0.5 * float(vertex_ID / 6);
            output_position = _transform(absVertexPos, frame_viewProjMatrix);
        }
        fssrc {
            uint3 uvi = uint3(255 * input_uvi);
            float packed = float(uvi.x << 16 | uvi.y << 8 | uvi.z) / 16777216.0;
            output_color[0] = float4(input_corner.xyz, packed);
        }
    )";
    
    static const char *g_fullScreenBaseShaderSrc = R"(
        inout {
            texcoord : float2
        }
        vssrc {
            float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
            output_position = float4(vertexCoord.xy * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.5, 1);
            output_texcoord = vertexCoord;
        }
        fssrc {
            float4 gbuffer = _tex2nearest(0, input_texcoord);
            float3 unpacking(1.0, 256.0, 65536.0);
            float3 uvi = _fract(unpacking * gbuffer.w);
            output_color[0] = float4(uvi.z, 0, 0, 1.0);
        }
    )";
    
    static const char *g_lightBillboardShaderSrc = R"(
        fixed {
            axis[3] : float3 =
                [0.0, 0.0, 1.0]
                [1.0, 0.0, 0.0]
                [0.0, 1.0, 0.0]
                
            bnm[3] : float3 =
                [1.0, 0.0, 0.0]
                [0.0, 0.0, 1.0]
                [1.0, 0.0, 0.0]

            tgt[3] : float3 =
                [0.0, 1.0, 0.0]
                [0.0, 1.0, 0.0]
                [0.0, 0.0, 1.0]
        }
        const {
            positionRadius : float4
            bounds : float4
            color : float4
        }
        fndef voxTexCoord (float3 cubepos) -> float2 {
            int ypos = int(cubepos.y + 32.0);
            float2 yoffset = float2(ypos % 8, ypos / 8) * float2(64, 64);
            return (cubepos.xz + float2(32.0, 32.0) + yoffset) / float2(512.0, 512.0);
        }
        vssrc {
            float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
            float4 billPos = float4(vertexCoord.xy * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 0.0);
            float4 viewPos = _transform(float4(const_positionRadius.xyz, 1), frame_viewMatrix) + billPos * const_positionRadius.w * 1.2;
            output_position = _transform(viewPos, frame_projMatrix);
        }
        fssrc {
            float4 gbuffer = _tex2nearest(0, fragment_coord);
            float3 uvi = _fract(float3(1.0, 256.0, 65536.0) * gbuffer.w);
            float3 relCornerPos = gbuffer.xyz - const_positionRadius.xyz;

            int faceIndex = int(2.1 * uvi.z);

            float3 dirB = fixed_bnm[faceIndex];
            float3 dirT = fixed_tgt[faceIndex];
            float3 faceNormal = fixed_axis[faceIndex];
            
            float koeff_0 = _tex2nearest(1, voxTexCoord(relCornerPos.xyz + faceNormal)).x;
            float koeff_1 = _tex2nearest(1, voxTexCoord(relCornerPos.xyz + faceNormal + dirB)).x;
            float koeff_2 = _tex2nearest(1, voxTexCoord(relCornerPos.xyz + faceNormal + dirT)).x;
            float koeff_3 = _tex2nearest(1, voxTexCoord(relCornerPos.xyz + faceNormal + dirB + dirT)).x;
            
            float m0 = _lerp(koeff_0, koeff_1, uvi.x);
            float m1 = _lerp(koeff_2, koeff_3, uvi.x);
            float k = _lerp(m0, m1, uvi.y);
            
            output_color[0] = float4(k, 0, 0, 1.0);
        }
    )";

//                [-0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
//                [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0][0.5, 0.5, -0.5, 1.0]
//                [0.5, 0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0]


    // XFace Indeces : [-y-z, -y+z, +y-z, +y+z]
    // YFace Indeces : [-z-x, -z+x, +z-x, +z+x]
    // ZFace Indeces : [-y-x, -y+x, +y-x, +y+x]
//    const char *g_staticMeshShaderSrc = R"(
//        fixed {
//            axis[3] : float3 =
//                [0.0, 0.0, 1.0]
//                [1.0, 0.0, 0.0]
//                [0.0, 1.0, 0.0]
//
//            bnm[3] : float3 =
//                [1.0, 0.0, 0.0]
//                [0.0, 0.0, 1.0]
//                [1.0, 0.0, 0.0]
//
//            tgt[3] : float3 =
//                [0.0, 1.0, 0.0]
//                [0.0, 1.0, 0.0]
//                [0.0, 0.0, 1.0]
//
//            cube[18] : float4 =
//                [-0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0]
//                [0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
//                [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0]
//                [0.5, -0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, 0.5, -0.5, 1.0]
//                [0.5, 0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, 0.5, -0.5, 1.0]
//                [0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0]
//
//            faceUV[4] : float2 =
//                [0.0, 0.0][1.0, 0.0][0.0, 1.0][1.0, 1.0]
//
//        }
//        inout {
//            koeff : float4
//            normal : float3
//            texcoord : float2
//            tmp : float1
//        }
//        fndef voxTexCoord (float3 cubepos) -> float2 {
//            int ypos = int(cubepos.y + 32.0);
//            float2 yoffset = float2(ypos % 8, ypos / 8) * float2(64, 64);
//            return (cubepos.xz + float2(32.0, 32.0) + yoffset) / float2(512.0, 512.0);
//        }
//        vssrc {
//            float3 cubeCenter = float3(instance_position_color.xyz);
//            float3 toCamSign = _sign(frame_cameraPosition.xyz - cubeCenter);
//            float4 relVertexPos = float4(toCamSign, 1.0) * fixed_cube[vertex_ID];
//            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
//            float3 faceNormal = toCamSign * fixed_axis[vertex_ID / 6];
//
//            int3 faceIndices = int3(2.0 * _step(0.0, relVertexPos.yzy) + _step(0.0, relVertexPos.zxx));
//
//            float2 texcoord = float2(0.0, 0.0);
//            texcoord = texcoord + _abs(faceNormal.x) * fixed_faceUV[faceIndices.x];
//            texcoord = texcoord + _abs(faceNormal.y) * fixed_faceUV[faceIndices.y];
//            texcoord = texcoord + _abs(faceNormal.z) * fixed_faceUV[faceIndices.z];
//            output_texcoord = texcoord;
//
//            float3 lightOffset = cubeCenter - float3(7.0, 0.0, 7.0);
//            float3 dirB = fixed_bnm[vertex_ID / 6];
//            float3 dirT = fixed_tgt[vertex_ID / 6];
//            float3 cornerPos = lightOffset - 0.5 * (dirB + dirT - faceNormal);
//
//            output_koeff.x = _tex2nearest(0, voxTexCoord(cornerPos.xyz + faceNormal)).x;
//            output_koeff.y = _tex2nearest(0, voxTexCoord(cornerPos.xyz + dirB + faceNormal)).x;
//            output_koeff.z = _tex2nearest(0, voxTexCoord(cornerPos.xyz + dirT + faceNormal)).x;
//            output_koeff.w = _tex2nearest(0, voxTexCoord(cornerPos.xyz + dirT + dirB + faceNormal)).x;
//
//            output_position = _transform(absVertexPos, frame_viewProjMatrix);
//            output_normal = float3(faceNormal);
//        }
//        fssrc {
//            float m0 = _lerp(input_koeff[0], input_koeff[1], input_texcoord.x);
//            float m1 = _lerp(input_koeff[2], input_koeff[3], input_texcoord.x);
//            float k = _lerp(m0, m1, input_texcoord.y);
//            output_color[0] = float4(k, 0, 0, 1);
//        }
//    )";

    static const char *g_tmpScreenQuadShaderSrc = R"(
        inout {
            texcoord : float2
        }
        vssrc {
            float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
            output_position = float4(vertexCoord.xy * float2(0.76762, -(1.365)) + float2(-0.96, 0.96), 0.5, 1);
            output_texcoord = vertexCoord;
        }
        fssrc {
            //float4 gbuffer = _tex2nearest(1, input_texcoord);
            ///float3 colorOcc = _frac(float3(1.0, 256.0, 65536.0) * gbuffer.b);
            
            //float4 surface = _tex2nearest(0, float2(colorOcc.z, 0));
            
            float4 s0 = _tex2nearest(0, input_texcoord);
            float4 s1 = _tex2nearest(1, input_texcoord);
            output_color[0] = s0;//_lerp(s0, s1, _step(0.8, input_texcoord.y));
        }
    )";

//                [0.0, 0.0]
//                [0.001953125, 0.0]
//                [0.0, 0.001953125]
//                [0.001953125, 0.001953125]

//                [-0.0009765625, -0.0009765625]
//                [0.0009765625, -0.0009765625]
//                [-0.0009765625, 0.0009765625]
//                [0.0009765625, 0.0009765625]

//    static const char *g_lightBillboardShaderSrc = R"(
//        fixed {
//            offset[4] : float2 =
//                [-0.0009765625, -0.0009765625]
//                [0.0009765625, -0.0009765625]
//                [-0.0009765625, 0.0009765625]
//                [0.0009765625, 0.0009765625]
//
//            step : float2 =
//                [0.001953125, 0.001953125]
//        }
//        const {
//            positionRadius : float4
//            color : float4
//            rotation : matrix4
//        }
//        inout {
//            texcoord : float2
//        }
//        vssrc {
//            float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
//            float4 billPos = float4(vertexCoord.xy * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 0.0);
//            float4 viewPos = _transform(float4(const_positionRadius.xyz, 1), frame_viewMatrix) + billPos * const_positionRadius.w * 1.2;
//            output_position = _transform(viewPos, frame_projMatrix);
//            output_texcoord = vertexCoord;
//        }
//        fssrc {
//            float4 gbuffer = _tex2nearest(1, fragment_coord);
//
//            float4 clipSpaceWPos = float4(fragment_coord * float2(2.0, -2.0) + float2(-1.0, 1.0), gbuffer.r, 1.0);
//            float4 invWPos = _transform(clipSpaceWPos, frame_invViewProjMatrix);
//            float3 worldPosition = (invWPos / invWPos.w).xyz;
//            float3 normal = _frac(float3(1.0, 256.0, 65536.0) * gbuffer.g);
//            float3 colorOcc = _frac(float3(1.0, 256.0, 65536.0) * gbuffer.b);
//            float4 surface = _tex2nearest(0, float2(colorOcc.z, 0));
//
//            float3 toLight = const_positionRadius.xyz - worldPosition;
//            float  toLightLength = length(toLight);
//            float3 toLightNrm = toLight / toLightLength;
//            float  dist = 0.96 * toLightLength / const_positionRadius.w;
//            float3 octahedron = toLightNrm / _dot(toLightNrm, _sign(toLightNrm));
//            float2 coord = float2(0.5 + 0.5 * (octahedron.x + octahedron.z), 0.5 - 0.5 * (octahedron.z - octahedron.x));
//            int    hmIndex = int(_step(worldPosition.y, const_positionRadius.y));
//
//            float2 mm = float2(1.0, 1.0);
//
//            float2 koeff = fmod(coord, fixed_step * mm);
//            float2 origin = coord - koeff;
//            koeff = koeff / (fixed_step * mm);
//
//            float  shm0 = step(dist, _tex2nearest(2, origin + fixed_offset[0] * mm)[hmIndex]);
//            float  shm1 = step(dist, _tex2nearest(2, origin + fixed_offset[1] * mm)[hmIndex]);
//            float  shm2 = step(dist, _tex2nearest(2, origin + fixed_offset[2] * mm)[hmIndex]);
//            float  shm3 = step(dist, _tex2nearest(2, origin + fixed_offset[3] * mm)[hmIndex]);
//
//            float m0 = _lerp(shm0, shm1, koeff.x);
//            float m1 = _lerp(shm2, shm3, koeff.x);
//            float kk = _lerp(m0, m1, koeff.y);
//
//
//            //lits[i] = step(dist, shm);
//
//            //float  attenuation = pow(saturate(1.0 - toLightLength / const_positionRadius.w), 2.0);
//            //float  shading = 1.4 * (_dot(toLightNrm, normal) * 0.5 + 0.5) * gbuffer.y;
//
//            //float2 f = float2(worldPosition.xz - w0.xz);
//            //float2 f = float2(_dot(worldPosition - w0, binormalAndOcc.xyz), _dot(worldPosition - w0, tangentAndColor.xyz));
//            //output_color = float4(surface.rgb, attenuation * shading) * step(dist, shm);
//
//            float f = kk;
//
//            output_color[0] = float4(1.0, 1.0, 0.0, 1.0);
//        }
//    )";
    
    /*
            float4 gbuffer = _tex2raw(1, fragment_coord);
            float4 surface = _tex2nearest(0, float2(gbuffer.r / 256.0, 0));
            
            float4 clipSpacePos = float4(fragment_coord * float2(2.0, -2.0) + float2(-1.0, 1.0), gbuffer.w, 1.0);
            float4 worldPosition = _transform(clipSpacePos, frame_invViewProjMatrix);
            worldPosition = worldPosition / worldPosition.w;
            
            float3 unpacking(1.0, 256.0, 65536.0);
            float3 normal = 2.0 * fract(unpacking * gbuffer.z) - 1.0;
            
            float3 toLight = const_positionRadius.xyz - worldPosition.xyz;
            float  toLightLength = length(toLight);
            float3 toLightNrm = toLight / toLightLength;
            float  dist = 0.96 * (toLightLength / const_positionRadius.w);

            float3 axis = _norm(_cross(fixed_north.xyz, toLightNrm));
            float  angle = acos(_dot(toLightNrm, fixed_north.xyz));
            float3 cs = float3(_cos(angle), _sin(angle), 1.0 - _cos(angle));

            float3 octahedron = toLightNrm / _dot(toLightNrm, _sign(toLightNrm));
            float2 coord = float2(0.5 + 0.5 * (octahedron.x + octahedron.z), 0.5 - 0.5 * (octahedron.z - octahedron.x));
            float  shm = _tex2nearest(2, coord)[int(_step(worldPosition.y, const_positionRadius.y))];
            float  litSum = step(dist, shm);
            
            for (uint i = 1; i < 5; i++) {
                float noise0 = (fract(_sin(_dot(fragment_coord * float(i), float2(12.9898, 78.233) * 2.0)) * 43758.5453));
                float noise1 = (fract(_cos(_dot(fragment_coord * float(i), float2(12.9898, 78.233) * 2.0)) * 43758.5453));
            
                float y = noise0 * (1.0f - fixed_north.w) + fixed_north.w;
                float phi = noise1 * 2.0f * 3.14159265359;
                float3 spread = float3(sqrt(1.0 - y * y) * _cos(phi), y, sqrt(1.0 - y * y) * _sin(phi));
                float3 toLightSpreadedNrm = spread * cs.x + _cross(axis, spread) * cs.y + axis * _dot(axis, spread) * cs.z;
                
                float3 octahedron = toLightSpreadedNrm / _dot(toLightSpreadedNrm, _sign(toLightSpreadedNrm));
                float2 coord = float2(0.5 + 0.5 * (octahedron.x + octahedron.z), 0.5 - 0.5 * (octahedron.z - octahedron.x));
                float  shm = _tex2nearest(2, coord)[int(_step(worldPosition.y, const_positionRadius.y))];
                litSum = litSum + step(dist, shm);
            }
            
            float  attenuation = pow(saturate(1.0 - toLightLength / const_positionRadius.w), 2.0);
            float  shading = 1.4 * (_dot(toLightNrm, normal) * 0.5 + 0.5) * gbuffer.y;
            
            //output_color = float4(surface.rgb, attenuation * shading) * step(dist, shm);
            output_color = float4(surface.rgb, litSum / 5.0);
    */
    
    voxel::SceneObjectToken g_lastToken = 0x100;
    
    bool isInLight(const math::vector3f &bbMin, const math::vector3f &bbMax, const math::vector3f &lightPosition, float lightRadius) {
        math::vector3f center = bbMin + 0.5f * (bbMax - bbMin);
        return center.distanceTo(lightPosition) < lightRadius + center.distanceTo(bbMax);
    }
}

namespace voxel {
    struct StaticVoxelModel {
        math::vector3f bbMin = {FLT_MAX, FLT_MAX, FLT_MAX};
        math::vector3f bbMax = {FLT_MIN, FLT_MIN, FLT_MIN};
        voxel::Mesh source;
        foundation::RenderDataPtr voxels;
    };
    
    struct LightSource {
        std::vector<const StaticVoxelModel *> receiverTmpBuffer;
        foundation::RenderTargetPtr voxelMap;

        struct ShaderConst {
            math::vector4f positionRadius;
            math::vector4f bounds;
            math::color rgba;
        }
        constants;
    };
    
    class SceneImpl : public Scene {
    public:
        SceneImpl(const voxel::MeshFactoryPtr &factory, const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering, const char *palette);
        ~SceneImpl() override;
        
        SceneObjectToken addStaticModel(const char *voxPath, const int16_t(&offset)[3]) override;
        SceneObjectToken addLightSource(const math::vector3f &position, float radius, const math::color &rgba) override;
        
        void removeStaticModel(SceneObjectToken token) override;
        void removeLightSource(SceneObjectToken token) override;
        
        void updateAndDraw(float dtSec) override;
        
    private:
        voxel::MeshFactoryPtr _factory;
        foundation::PlatformInterfacePtr _platform;
        foundation::RenderingInterfacePtr _rendering;

        foundation::RenderShaderPtr _voxelMapShader;
        foundation::RenderShaderPtr _staticMeshGBuferShader;
        foundation::RenderShaderPtr _fullScreenBaseShader;
        foundation::RenderShaderPtr _lightBillboardShader;

        foundation::RenderShaderPtr _tmpScreenQuadShader;
        
        std::unordered_map<SceneObjectToken, std::unique_ptr<StaticVoxelModel>> _staticModels;
        std::unordered_map<SceneObjectToken, std::unique_ptr<LightSource>> _lightSources;
        
        foundation::RenderTexturePtr _palette;
        foundation::RenderTargetPtr _gbuffer;
    };
    
    SceneImpl::SceneImpl(const voxel::MeshFactoryPtr &factory, const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering, const char *palette) {
        _factory = factory;
        _platform = platform;
        _rendering = rendering;

        _voxelMapShader = rendering->createShader("scene_shadow_voxel_map", g_voxelMapShaderSrc,
            { // vertex
                {"ID", foundation::RenderShaderInputFormat::ID},
            },
            { // instance
                {"position", foundation::RenderShaderInputFormat::SHORT4},
            }
        );
        _staticMeshGBuferShader = rendering->createShader("scene_static_voxel_mesh", g_staticMeshGBufferShaderSrc,
            { // vertex
                {"ID", foundation::RenderShaderInputFormat::ID}
            },
            { // instance
                {"position_color", foundation::RenderShaderInputFormat::SHORT4},
            }
        );
        _fullScreenBaseShader = _rendering->createShader("scene_full_screen_base", g_fullScreenBaseShaderSrc, {
            {"ID", foundation::RenderShaderInputFormat::ID}
        });
        _lightBillboardShader = _rendering->createShader("scene_light_billboard", g_lightBillboardShaderSrc, {
            {"ID", foundation::RenderShaderInputFormat::ID}
        });
        

        _tmpScreenQuadShader = _rendering->createShader("scene_screen_quad", g_tmpScreenQuadShaderSrc, {
            {"ID", foundation::RenderShaderInputFormat::ID}
        });
        
        _gbuffer = rendering->createRenderTarget(foundation::RenderTextureFormat::RGBA32F, 1, _rendering->getBackBufferWidth(), _rendering->getBackBufferHeight(), true);
        
        std::unique_ptr<std::uint8_t[]> paletteData;
        std::size_t paletteSize;

        if (_platform->loadFile(palette, paletteData, paletteSize)) {
            upng_t *upng = upng_new_from_bytes(paletteData.get(), (unsigned long)(paletteSize));

            if (upng != nullptr) {
                if (*reinterpret_cast<const unsigned *>(paletteData.get()) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK) {
                    if (upng_get_format(upng) == UPNG_RGBA8 && upng_get_width(upng) == 256 && upng_get_height(upng) == 1) {
                        _palette = _rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, 256, 1, { upng_get_buffer(upng) });
                    }
                }

                upng_free(upng);
            }
        }
        
        if (_palette == nullptr) {
            _platform->logError("[Scene] '%s' not found", palette);
        }
        
    }
    
    SceneImpl::~SceneImpl() {
        
    }
    
    std::shared_ptr<Scene> Scene::instance(const voxel::MeshFactoryPtr &factory, const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering, const char *palette) {
        return std::make_shared<SceneImpl>(factory, platform, rendering, palette);
    }
    
    SceneObjectToken SceneImpl::addStaticModel(const char *voxPath, const int16_t(&offset)[3]) {
        std::unique_ptr<StaticVoxelModel> model = std::make_unique<StaticVoxelModel>();
        SceneObjectToken token = g_lastToken++ & std::uint64_t(0x7FFFFFFF);
        
        if (_factory->createMesh(voxPath, offset, model->source)) {
            std::uint32_t voxelCount = model->source.frames[0].voxelCount;
            std::unique_ptr<VoxelInfo[]> &positions = model->source.frames[0].voxels;
            
            for (std::uint32_t i = 0; i < voxelCount; i++) {
                if (model->bbMin.x > float(positions[i].positionX)) {
                    model->bbMin.x = float(positions[i].positionX);
                }
                if (model->bbMin.y > float(positions[i].positionY)) {
                    model->bbMin.y = float(positions[i].positionY);
                }
                if (model->bbMin.z > float(positions[i].positionZ)) {
                    model->bbMin.z = float(positions[i].positionZ);
                }
                if (model->bbMax.x < float(positions[i].positionX)) {
                    model->bbMax.x = float(positions[i].positionX);
                }
                if (model->bbMax.y < float(positions[i].positionY)) {
                    model->bbMax.y = float(positions[i].positionY);
                }
                if (model->bbMax.z < float(positions[i].positionZ)) {
                    model->bbMax.z = float(positions[i].positionZ);
                }
            }
            
            model->voxels = _rendering->createData(model->source.frames[0].voxels.get(), voxelCount, sizeof(voxel::VoxelInfo));
            
            if (model->voxels) {
                _staticModels.emplace(token, std::move(model));
                return token;
            }
        }
        
        _platform->logError("[Scene] model '%s' not loaded", voxPath);
        return INVALID_TOKEN;
    }
    
    SceneObjectToken SceneImpl::addLightSource(const math::vector3f &position, float radius, const math::color &rgba) {
        std::unique_ptr<LightSource> lightSource = std::make_unique<LightSource>();
        SceneObjectToken token = g_lastToken++ & std::uint64_t(0x7FFFFFFF);
        
        // Possible sets:
        // 64x64x64 cube is 512x512 texture for the light of max radius = 31
        // 256x256x256 cube is 4096x4096 texture for the light of max radius = 127
        
        lightSource->voxelMap = _rendering->createRenderTarget(foundation::RenderTextureFormat::R8UN, 1, 512, 512, false);
        lightSource->constants.positionRadius = math::vector4f(position, radius);
        lightSource->constants.bounds = math::vector4f(256.0f, 32.0f, 8.0f, 0.0f);
        lightSource->constants.rgba = rgba;
        
        if (lightSource->voxelMap) {
            _lightSources.emplace(token, std::move(lightSource));
        }
        
        return INVALID_TOKEN;
    }
    
    void SceneImpl::removeStaticModel(SceneObjectToken token) {
        _staticModels.erase(token);
    }
    
    void SceneImpl::removeLightSource(SceneObjectToken token) {
        _lightSources.erase(token);
    }
    
    void SceneImpl::updateAndDraw(float dtSec) {
        foundation::RenderTexturePtr textures[4] = {nullptr};

        for (const auto &modelIt : _staticModels) {
            const StaticVoxelModel *model = modelIt.second.get();
            
            for (auto &lightIt : _lightSources) {
                LightSource &light = *lightIt.second;
                
                if (isInLight(model->bbMin, model->bbMax, light.constants.positionRadius.xyz, light.constants.positionRadius.w)) {
                    light.receiverTmpBuffer.emplace_back(model);
                }
            }
        }
        
        for (auto &lightIt : _lightSources) {
            LightSource &light = *lightIt.second;
            
            _rendering->applyState(_voxelMapShader, foundation::RenderPassCommonConfigs::CLEAR(light.voxelMap, foundation::BlendType::ADDITIVE, 0.0f, 0.0f, 0.0f, 0.0f));
            _rendering->applyShaderConstants(&light.constants);
            
            for (const auto &item : light.receiverTmpBuffer) {
                _rendering->drawGeometry(nullptr, item->voxels, 8, item->voxels->getCount(), foundation::RenderTopology::POINTS);
            }

            light.receiverTmpBuffer.clear();
        }
        
        _rendering->applyState(_staticMeshGBuferShader, foundation::RenderPassCommonConfigs::CLEAR(_gbuffer, foundation::BlendType::DISABLED, 0.0f, 0.0f, 0.0f));

        for (const auto &modelIt : _staticModels) {
            const StaticVoxelModel *model = modelIt.second.get();
            _rendering->drawGeometry(nullptr, model->voxels, 18, model->voxels->getCount(), foundation::RenderTopology::TRIANGLES);
        }

        textures[0] = _gbuffer->getTexture(0);
        _rendering->applyState(_fullScreenBaseShader, foundation::RenderPassCommonConfigs::DEFAULT());
        _rendering->applyTextures(textures, 1);
        _rendering->drawGeometry(nullptr, 4, foundation::RenderTopology::TRIANGLESTRIP);

        _rendering->applyState(_lightBillboardShader, foundation::RenderPassCommonConfigs::OVERLAY(foundation::BlendType::DISABLED));
        for (auto &lightIt : _lightSources) {
            LightSource &light = *lightIt.second;
            textures[1] = light.voxelMap->getTexture(0);
            _rendering->applyTextures(textures, 2);
            _rendering->applyShaderConstants(&light.constants);
            _rendering->drawGeometry(nullptr, 4, foundation::RenderTopology::TRIANGLESTRIP);
        }
        

//        textures[0] = _lightSources.begin()->second->voxelMap->getTexture(0);
//        _rendering->applyState(_tmpScreenQuadShader, foundation::RenderPassCommonConfigs::OVERLAY(foundation::BlendType::DISABLED));
//        _rendering->applyTextures(textures, 1);
//        _rendering->drawGeometry(nullptr, 4, foundation::RenderTopology::TRIANGLESTRIP);

        /*
        
        // TODO:
        // 1. lights -> vector
        
        _rendering->beginPass("scene_static_voxel_models", _staticMeshShader, foundation::RenderPassConfig(_gbuffer, 0.0f, 0.0f, 0.0f));
        for (const auto &modelIt : _staticModels) {
            const StaticVoxelModel *model = modelIt.second.get();
            _rendering->drawGeometry(nullptr, model->voxels, 12, model->voxels->getCount(), foundation::RenderTopology::TRIANGLESTRIP);
        }
        _rendering->endPass();
        
        foundation::RenderTexturePtr textures[3] = {_palette, _gbuffer, nullptr};
        
        _rendering->beginPass("scene_base", _fullScreenQuadShader, foundation::RenderPassConfig(0.0f, 0.0f, 0.0f));
        _rendering->applyTextures(textures, 2);
        _rendering->drawGeometry(nullptr, 4, foundation::RenderTopology::TRIANGLESTRIP);
        _rendering->endPass();
        
        _rendering->beginPass("scene_lightbillboards", _lightBollboardShader, foundation::RenderPassConfig(foundation::BlendType::ADDITIVE, foundation::ZType::DISABLED));
        for (auto &lightIt : _lightSources) {
            LightSource &light = *lightIt.second;
            textures[2] = light.distanceMap;
            _rendering->applyTextures(textures, 3);
            _rendering->applyShaderConstants(&light.constants);
            _rendering->drawGeometry(nullptr, 4, foundation::RenderTopology::TRIANGLESTRIP);
        }
        _rendering->endPass();
        */
    }
}

