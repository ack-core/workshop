
#include "scene.h"

#include <cfloat>
#include <memory>
#include <vector>
#include <unordered_map>

#include "thirdparty/upng/upng.h"

//[-0.6, 0.6, 0.6, -1][-0.6, -0.6, 0.6, -1][0.6, 0.6, 0.6, -1][-0.6, -0.6, 0.6, -1][0.6, 0.6, 0.6, -1][0.6, -0.6, 0.6, -1]
//[0.6, -0.6, 0.6, -1][0.6, 0.6, 0.6, -1][0.6, -0.6, -0.6, -1][0.6, 0.6, 0.6, -1][0.6, -0.6, -0.6, -1][0.6, 0.6, -0.6, -1]
//[0.6, 0.6, -0.6, -1][0.6, 0.6, 0.6, -1][-0.6, 0.6, -0.6, -1][0.6, 0.6, 0.6, -1][-0.6, 0.6, -0.6, -1][-0.6, 0.6, 0.6, -1]
//[-0.6, 0.6, 0.6, +1][-0.6, -0.6, 0.6, +1][0.6, 0.6, 0.6, +1][-0.6, -0.6, 0.6, +1][0.6, 0.6, 0.6, +1][0.6, -0.6, 0.6, +1]
//[0.6, -0.6, 0.6, +1][0.6, 0.6, 0.6, +1][0.6, -0.6, -0.6, +1][0.6, 0.6, 0.6, +1][0.6, -0.6, -0.6, +1][0.6, 0.6, -0.6, +1]
//[0.6, 0.6, -0.6, +1][0.6, 0.6, 0.6, +1][-0.6, 0.6, -0.6, +1][0.6, 0.6, 0.6, +1][-0.6, 0.6, -0.6, +1][-0.6, 0.6, 0.6, +1]

namespace {
    const char *g_shadowShaderSrc = R"(
        fixed {
            cube[36] : float4 =
[-0.6, 0.6, 0.6, -1][-0.6, -0.6, 0.6, -1][0.6, 0.6, 0.6, -1][-0.6, -0.6, 0.6, -1][0.6, 0.6, 0.6, -1][0.6, -0.6, 0.6, -1]
[0.6, -0.6, 0.6, -1][0.6, 0.6, 0.6, -1][0.6, -0.6, -0.6, -1][0.6, 0.6, 0.6, -1][0.6, -0.6, -0.6, -1][0.6, 0.6, -0.6, -1]
[0.6, 0.6, -0.6, -1][0.6, 0.6, 0.6, -1][-0.6, 0.6, -0.6, -1][0.6, 0.6, 0.6, -1][-0.6, 0.6, -0.6, -1][-0.6, 0.6, 0.6, -1]
[-0.6, 0.6, 0.6, +1][-0.6, -0.6, 0.6, +1][0.6, 0.6, 0.6, +1][-0.6, -0.6, 0.6, +1][0.6, 0.6, 0.6, +1][0.6, -0.6, 0.6, +1]
[0.6, -0.6, 0.6, +1][0.6, 0.6, 0.6, +1][0.6, -0.6, -0.6, +1][0.6, 0.6, 0.6, +1][0.6, -0.6, -0.6, +1][0.6, 0.6, -0.6, +1]
[0.6, 0.6, -0.6, +1][0.6, 0.6, 0.6, +1][-0.6, 0.6, -0.6, +1][0.6, 0.6, 0.6, +1][-0.6, 0.6, -0.6, +1][-0.6, 0.6, 0.6, +1]        }
        const {
            lightPosition : float3
            lightRadius : float1
        }
        inout {
            dist : float4
        }
        vssrc {
            float3 cubeCenter = float3(instance_position.xyz);
            float3 toLight = const_lightPosition - cubeCenter;
            float3 toLightSign = _sign(toLight);
            
            float4 cubeConst = fixed_cube[vertex_ID];
            float3 relVertexPos = toLightSign * cubeConst.xyz;
            float3 absVertexPos = cubeCenter + relVertexPos;
            
            float3 vToLight = const_lightPosition - absVertexPos;
            float3 vToLightNrm = _norm(vToLight);
            float3 vToLightSign = _sign(vToLight);
            
            float3 octahedron = vToLightNrm / _dot(vToLightNrm, vToLightSign);
            float2 invOctahedron = vToLightSign.xz * (float2(1.0, 1.0) - _abs(octahedron.zx));
            float  isCurrentHemisphere = _step(0, cubeConst.w * (toLight.y) + 1.0);
            
            octahedron.xz = cubeConst.w * octahedron.y <= 0.0 ? invOctahedron : octahedron.xz;
            
            output_dist = float4(1, 1, 1, 1);
            output_dist[int(cubeConst.w * 0.5 + 0.5)] = _lerp(1.0, length(vToLight) / const_lightRadius, isCurrentHemisphere);
            output_position = float4((octahedron.x + octahedron.z), (octahedron.z - octahedron.x), 0.5, 1);
        }
        fssrc {
            output_color = float4(input_dist.x, input_dist.y, 1, 1);
        }
    )";

    // XFace Indeces : [-y-z, -y+z, +y-z, +y+z]
    // YFace Indeces : [-z-x, -z+x, +z-x, +z+x]
    // ZFace Indeces : [-y-x, -y+x, +y-x, +y+x]
    const char *g_staticMeshShaderSrc = R"(
        fixed {
            axis[3] : float3 =
                [0.0, 0.0, 1.0]
                [1.0, 0.0, 0.0]
                [0.0, 1.0, 0.0]

            binormal[3] : float3 =
                [1.0, 0.0, 0.0]
                [0.0, 1.0, 0.0]
                [0.0, 0.0, 1.0]

            tangent[3] : float3 =
                [0.0, 1.0, 0.0]
                [0.0, 0.0, 1.0]
                [1.0, 0.0, 0.0]

            cube[12] : float4 =
                [-0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0][0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0]

            faceUV[4] : float2 = [0.0, 0.0][1.0, 0.0][0.0, 1.0][1.0, 1.0]
        }
        inout {
            koeff : float4
            binormal_f0 : float4
            tangent_f1 : float4
            wpos_color_f2 : float3
            texcoord : float2
        }
        vssrc {
            float3 cubeCenter = float3(instance_position_color.xyz);
            float3 toCamera = frame_cameraPosition.xyz - cubeCenter;
            float3 toCamSign = _sign(toCamera);
            float4 relVertexPos = float4(toCamSign, 1.0) * fixed_cube[vertex_ID];
            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
            float3 faceNormal = toCamSign * fixed_axis[vertex_ID >> 2];
            float3 offset = relVertexPos.xyz; // + float3(0.5, 0.5, 0.5);

            // indexes of the current vertex in faceX, faceY and faceZ
            int3 faceIndeces = int3(float3(2.0, 2.0, 2.0) * _step(0.0, relVertexPos.yzy) + _step(0.0, relVertexPos.zxx));
            
            float4 koeff = float4(0.0, 0.0, 0.0, 0.0);
            koeff = koeff + _step(0.5, -faceNormal.x) * instance_lightFaceNX;
            koeff = koeff + _step(0.5,  faceNormal.x) * instance_lightFacePX;
            koeff = koeff + _step(0.5, -faceNormal.y) * instance_lightFaceNY;
            koeff = koeff + _step(0.5,  faceNormal.y) * instance_lightFacePY;
            koeff = koeff + _step(0.5, -faceNormal.z) * instance_lightFaceNZ;
            koeff = koeff + _step(0.5,  faceNormal.z) * instance_lightFacePZ;
            
            float2 texcoord = float2(0.0, 0.0);
            texcoord = texcoord + _step(0.5, _abs(faceNormal.x)) * fixed_faceUV[faceIndeces.x];
            texcoord = texcoord + _step(0.5, _abs(faceNormal.y)) * fixed_faceUV[faceIndeces.y];
            texcoord = texcoord + _step(0.5, _abs(faceNormal.z)) * fixed_faceUV[faceIndeces.z];

            output_position = _transform(absVertexPos, frame_viewProjMatrix);
            output_koeff = koeff;
            output_binormal_f0 = float4(fixed_binormal[vertex_ID >> 2], offset.x); //toCamSign.zxy *
            output_tangent_f1 = float4(fixed_tangent[vertex_ID >> 2], offset.y); //toCamSign.yzx *
            output_wpos_color_f2 = float3(output_position.z / output_position.w, instance_position_color.w, offset.z);
            output_texcoord = texcoord;
        }
        fssrc {
            float m0 = _lerp(input_koeff[0], input_koeff[1], input_texcoord.x);
            float m1 = _lerp(input_koeff[2], input_koeff[3], input_texcoord.x);
            uint  k = uint(255.0 * saturate(_pow(_smooth(0, 1, _lerp(m0, m1, input_texcoord.y)), 0.5)));

            uint3 ioffset = uint3(127.0 * float3(input_binormal_f0.w, input_tangent_f1.w, input_wpos_color_f2.z) + 128);
            float packedOffset = float(ioffset.x << 16 | ioffset.y << 8 | ioffset.z) / 16777216.0;
            
            uint3 ibinormal = uint3(127.0 * input_binormal_f0.xyz + 128.0);
            uint  packedBinormalAndOcc = k << 24 | ibinormal.z << 16 | ibinormal.y << 8 | ibinormal.x;

            uint3 itangent = uint3(127.0 * input_tangent_f1.xyz + 128.0);
            uint  packedTangentAndColor = uint(input_wpos_color_f2.y - 1.0) << 24 | itangent.z << 16 | itangent.y << 8 | itangent.x;
            
            output_color = float4(input_wpos_color_f2.x, _asFloat(packedBinormalAndOcc), _asFloat(packedTangentAndColor), packedOffset);
        }
    )";

    // XFace Indeces : [-y-z, -y+z, +y-z, +y+z]
    // YFace Indeces : [-z-x, -z+x, +z-x, +z+x]
    // ZFace Indeces : [-y-x, -y+x, +y-x, +y+x]
//    const char *g_dynamicMeshShaderSrc = R"(
//        fixed {
//            axis[3] : float4 =
//                [0.0, 0.0, 1.0, 0.0]
//                [1.0, 0.0, 0.0, 0.0]
//                [0.0, 1.0, 0.0, 0.0]
//
//            cube[12] : float4 =
//                [-0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
//                [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0][0.5, 0.5, -0.5, 1.0]
//                [0.5, 0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0]
//
//            faceUV[4] : float2 = [0.0, 0.0][1.0, 0.0][0.0, 1.0][1.0, 1.0]
//        }
//        const {
//            modelTransform : matrix4
//        }
//        inout {
//            texcoord : float2
//            koeff : float4
//        }
//        vssrc {
//            float3 cubeCenter = float3(instance_position_color.xyz);
//            float3 toCamera = frame_cameraPosition.xyz - _transform(float4(cubeCenter, 1.0), const_modelTransform).xyz;
//            float3 toCamSign = _sign(_transform(const_modelTransform, float4(toCamera, 0.0)).xyz);
//            float4 relVertexPos = float4(toCamSign, 1.0) * fixed_cube[vertex_ID];
//            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
//            float3 faceNormal = toCamSign * fixed_axis[vertex_ID >> 2].xyz;
//
//            int3 faceIndeces = int3(float3(2.0, 2.0, 2.0) * _step(0.0, relVertexPos.yzy) + _step(0.0, relVertexPos.zxx));
//
//            float4 koeff = float4(0.0, 0.0, 0.0, 0.0);
//            koeff = koeff + _step(0.5, -faceNormal.x) * instance_lightFaceNX;
//            koeff = koeff + _step(0.5,  faceNormal.x) * instance_lightFacePX;
//            koeff = koeff + _step(0.5, -faceNormal.y) * instance_lightFaceNY;
//            koeff = koeff + _step(0.5,  faceNormal.y) * instance_lightFacePY;
//            koeff = koeff + _step(0.5, -faceNormal.z) * instance_lightFaceNZ;
//            koeff = koeff + _step(0.5,  faceNormal.z) * instance_lightFacePZ;
//
//            float2 texcoord = float2(0.0, 0.0);
//            texcoord = texcoord + _step(0.5, _abs(faceNormal.x)) * fixed_faceUV[faceIndeces.x];
//            texcoord = texcoord + _step(0.5, _abs(faceNormal.y)) * fixed_faceUV[faceIndeces.y];
//            texcoord = texcoord + _step(0.5, _abs(faceNormal.z)) * fixed_faceUV[faceIndeces.z];
//
//            output_position = _transform(_transform(absVertexPos, const_modelTransform), frame_viewProjMatrix);
//            output_texcoord = texcoord;
//            output_koeff = koeff;
//        }
//        fssrc {
//            float m0 = _lerp(input_koeff[0], input_koeff[1], input_texcoord.x);
//            float m1 = _lerp(input_koeff[2], input_koeff[3], input_texcoord.x);
//            float k = _pow(_smooth(0, 1, _lerp(m0, m1, input_texcoord.y)), 0.5);
//            output_color = float4(k, k, k, 1.0);
//        }
//    )";

            //float4 clipSpaceWPos = float4(input_texcoord * float2(2.0, -2.0) + float2(-1.0, 1.0), gbuffer.r, 1.0);
            //float4 invWPos = _transform(clipSpaceWPos, frame_invViewProjMatrix);
            //float3 worldPosition = (invWPos / invWPos.w).xyz;
            
            //uint ibc = _asUint(gbuffer.g);
            //float4 binormalAndOcc = float4(uint4(ibc, ibc, ibc, ibc) >> uint4(0, 8, 16, 24) & uint4(255, 255, 255, 255));
            //binormalAndOcc = binormalAndOcc / float4(128.0, 128.0, 128.0, 255.0) - float4(1.0, 1.0, 1.0, 0.0);

            //float4 tangentAndColor = float4(uint4(itc, itc, itc, itc) >> uint4(0, 8, 16, 24) & uint4(255, 255, 255, 255));
            //tangentAndColor = tangentAndColor / float4(128.0, 128.0, 128.0, 255.0) - float4(1.0, 1.0, 1.0, 0.0);
            
            //float3 offset = 2.0 * _frac(float3(1.0, 256.0, 65536.0) * gbuffer.a) - 1.0;
            
            //float3 toCamera = frame_cameraPosition.xyz - worldPosition;
            //float3 toCamSign = _sign(toCamera);
            //float3 normal = toCamSign * _cross(binormalAndOcc.xyz, tangentAndColor.xyz);
            
            //float3 w0 = worldPosition - offset + 0.5 * normal - 0.5 * binormalAndOcc.xyz - 0.5 * tangentAndColor.xyz;
            //float3 w1 = w0 + binormalAndOcc.xyz;
            //float3 w2 = w0 + tangentAndColor.xyz;
            //float3 w3 = w0 + binormalAndOcc.xyz + tangentAndColor.xyz;
            //float4 d = float4(_len(worldPosition - w0), _len(worldPosition - w1), _len(worldPosition - w2), _len(worldPosition - w3));


    static const char *g_fullScreenQuadShaderSrc = R"(
        inout {
            texcoord : float2
        }
        vssrc {
            float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
            output_position = float4(vertexCoord.xy * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.5, 1);
            output_texcoord = vertexCoord;
        }
        fssrc {
            float4 gbuffer = _tex2raw(1, input_texcoord);
            uint itc = _asUint(gbuffer.b);
            float4 surface = _tex2nearest(0, float2(float(itc >> 24) / 255.0, 0));
            output_color = float4(surface.rgb, 1.0);
        }
    )";
    
    static const char *g_lightBollboardShaderSrc = R"(
        fixed {
            north : float4 =
                [0.0, 1.0, 0.0, 0.9999]
        }
        const {
            positionRadius : float4
            color : float4
            rotation : matrix4
        }
        inout {
            texcoord : float2
        }
        vssrc {
            float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
            float4 billPos = float4(vertexCoord.xy * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 0.0);
            float4 viewPos = _transform(float4(const_positionRadius.xyz, 1), frame_viewMatrix) + billPos * const_positionRadius.w * 1.2;
            output_position = _transform(viewPos, frame_projMatrix);
            output_texcoord = vertexCoord;
        }
        fssrc {
            float4 gbuffer = _tex2linear(1, fragment_coord);
            gbuffer.yz = _tex2raw(1, fragment_coord).yz;
            
            float4 clipSpaceWPos = float4(fragment_coord * float2(2.0, -2.0) + float2(-1.0, 1.0), gbuffer.r, 1.0);
            float4 invWPos = _transform(clipSpaceWPos, frame_invViewProjMatrix);
            float3 worldPosition = (invWPos / invWPos.w).xyz;
            
            uint ibc = _asUint(gbuffer.g);
            float4 binormalAndOcc = float4(uint4(ibc, ibc, ibc, ibc) >> uint4(0, 8, 16, 24) & uint4(255, 255, 255, 255));
            binormalAndOcc = binormalAndOcc / float4(128.0, 128.0, 128.0, 255.0) - float4(1.0, 1.0, 1.0, 0.0);

            uint itc = _asUint(gbuffer.b);
            float4 tangentAndColor = float4(uint4(itc, itc, itc, itc) >> uint4(0, 8, 16, 24) & uint4(255, 255, 255, 255));
            tangentAndColor = tangentAndColor / float4(128.0, 128.0, 128.0, 255.0) - float4(1.0, 1.0, 1.0, 0.0);
            
            float3 offset = 2.0 * _frac(float3(1.0, 256.0, 65536.0) * gbuffer.a) - 1.0;
            float4 surface = _tex2nearest(0, float2(tangentAndColor.w, 0));
            
            float3 toCamera = frame_cameraPosition.xyz - worldPosition;
            float3 toCamSign = _sign(toCamera);
            float3 normal = toCamSign * _cross(binormalAndOcc.xyz, tangentAndColor.xyz);
            
            //float3 w0 = worldPosition - offset + 0.5 * normal - 0.5 * binormalAndOcc.xyz - 0.5 * tangentAndColor.xyz;
            //float3 w1 = w0 + binormalAndOcc.xyz;
            //float3 w2 = w0 + tangentAndColor.xyz;
            //float3 w3 = w0 + binormalAndOcc.xyz + tangentAndColor.xyz;
            
            float4 lits = float4(0, 0, 0, 0);
            float3 w0 = worldPosition - offset + 0.5 * normal - 0.5 * binormalAndOcc.xyz - 0.5 * tangentAndColor.xyz;
            float3 w[4] = {
                w0,
                w0 + binormalAndOcc.xyz,
                w0 + tangentAndColor.xyz,
                w0 + binormalAndOcc.xyz + tangentAndColor.xyz,
            };
            
            for (int i = 0; i < 4; i++) {
                float3 toLight = const_positionRadius.xyz - w[i].xyz;
                float  toLightLength = length(toLight);
                float3 toLightNrm = toLight / toLightLength;
                float  dist = 0.96 * toLightLength / const_positionRadius.w;
                float3 octahedron = toLightNrm / _dot(toLightNrm, _sign(toLightNrm));
                float2 coord = float2(0.5 + 0.5 * (octahedron.x + octahedron.z), 0.5 - 0.5 * (octahedron.z - octahedron.x));
                float  shm = _tex2nearest(2, coord)[int(_step(w[i].y, const_positionRadius.y))];
                lits[i] = step(dist, shm);
            }
              
            float2 koeff = float2(_dot(worldPosition - w0, binormalAndOcc.xyz), _dot(worldPosition - w0, tangentAndColor.xyz));
              
            float m0 = _lerp(lits[0], lits[1], koeff.x);
            float m1 = _lerp(lits[2], lits[3], koeff.x);
            float k = _pow(_smooth(0, 1, _lerp(m0, m1, koeff.y)), 0.5); //_lerp(m0, m1, koeff.y);
              
            
            //float  attenuation = pow(saturate(1.0 - toLightLength / const_positionRadius.w), 2.0);
            //float  shading = 1.4 * (_dot(toLightNrm, normal) * 0.5 + 0.5) * gbuffer.y;
            
            //float2 f = float2(worldPosition.xz - w0.xz);
            //float2 f = float2(_dot(worldPosition - w0, binormalAndOcc.xyz), _dot(worldPosition - w0, tangentAndColor.xyz));
            //output_color = float4(surface.rgb, attenuation * shading) * step(dist, shm);
            
            output_color = float4(k, k, k, 0.5);
        }
    )";
    
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
        foundation::RenderDataPtr positions;
    };
    
    struct LightSource {
        std::vector<const StaticVoxelModel *> receiverTmpBuffer;
        foundation::RenderTexturePtr distanceMap;

        struct ShaderConst {
            math::vector4f positionRadius;
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

        foundation::RenderShaderPtr _shadowShader;
        foundation::RenderShaderPtr _staticMeshShader;
        foundation::RenderShaderPtr _dynamicMeshShader;
        foundation::RenderShaderPtr _fullScreenQuadShader;
        foundation::RenderShaderPtr _lightBollboardShader;
        
        std::unordered_map<SceneObjectToken, std::unique_ptr<StaticVoxelModel>> _staticModels;
        std::unordered_map<SceneObjectToken, std::unique_ptr<LightSource>> _lightSources;
        
        foundation::RenderTexturePtr _palette;
        foundation::RenderTexturePtr _gbuffer;
    };
    
    SceneImpl::SceneImpl(const voxel::MeshFactoryPtr &factory, const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering, const char *palette) {
        _factory = factory;
        _platform = platform;
        _rendering = rendering;
        
        _staticMeshShader = rendering->createShader("scene_static_voxel_mesh", g_staticMeshShaderSrc,
            { // vertex
                {"ID", foundation::RenderShaderInputFormat::ID}
            },
            { // instance
                {"position_color", foundation::RenderShaderInputFormat::SHORT4},
                {"lightFaceNX", foundation::RenderShaderInputFormat::BYTE4_NRM},
                {"lightFacePX", foundation::RenderShaderInputFormat::BYTE4_NRM},
                {"lightFaceNY", foundation::RenderShaderInputFormat::BYTE4_NRM},
                {"lightFacePY", foundation::RenderShaderInputFormat::BYTE4_NRM},
                {"lightFaceNZ", foundation::RenderShaderInputFormat::BYTE4_NRM},
                {"lightFacePZ", foundation::RenderShaderInputFormat::BYTE4_NRM},
            }
        );
        _shadowShader = rendering->createShader("scene_shadow_distance", g_shadowShaderSrc,
            { // vertex
                {"ID", foundation::RenderShaderInputFormat::ID}
            },
            { // instance
                {"position", foundation::RenderShaderInputFormat::SHORT4},
            }
        );
        _fullScreenQuadShader = _rendering->createShader("scene_screen_quad", g_fullScreenQuadShaderSrc, {
            {"ID", foundation::RenderShaderInputFormat::ID}
        });
        _lightBollboardShader = _rendering->createShader("scene_light_billboard", g_lightBollboardShaderSrc, {
            {"ID", foundation::RenderShaderInputFormat::ID}
        });
        
        _gbuffer = rendering->createRenderTarget(foundation::RenderTextureFormat::RGBA32F, _rendering->getBackBufferWidth(), _rendering->getBackBufferHeight(), true);
        
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
            std::unique_ptr<VoxelPosition[]> &positions = model->source.frames[0].positions;
            
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
            
            model->positions = _rendering->createData(positions.get(), voxelCount, sizeof(voxel::VoxelPosition));
            model->voxels = _rendering->createData(model->source.frames[0].voxels.get(), voxelCount, sizeof(voxel::VoxelInfo));
            
            if (model->positions && model->voxels) {
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
        
        lightSource->distanceMap = _rendering->createRenderTarget(foundation::RenderTextureFormat::RGBA32F, 512, 512, false);
        lightSource->constants.positionRadius = math::vector4f(position, radius);
        lightSource->constants.rgba = rgba;
        
        if (lightSource->distanceMap) {
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
            _rendering->beginPass("scene_shadows", _shadowShader, foundation::RenderPassConfig(light.distanceMap, foundation::BlendType::MINVALUE, 1.0f, 1.0f, 1.0f));
            _rendering->applyShaderConstants(&light.constants.positionRadius);

            for (const auto &item : light.receiverTmpBuffer) {
                _rendering->drawGeometry(nullptr, item->positions, 36, item->positions->getCount(), foundation::RenderTopology::TRIANGLES);
            }

            _rendering->endPass();
            light.receiverTmpBuffer.clear();
        }
        
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
    }
}

