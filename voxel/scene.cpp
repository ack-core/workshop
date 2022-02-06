
#include "scene.h"

#include <cfloat>
#include <memory>
#include <vector>
#include <unordered_map>

namespace {
    const char *g_shadowShaderSrc = R"(
        fixed {
            cube[36] : float4 =
                [-0.5, 0.5, 0.5, -1][-0.5, -0.5, 0.5, -1][0.5, 0.5, 0.5, -1][-0.5, -0.5, 0.5, -1][0.5, 0.5, 0.5, -1][0.5, -0.5, 0.5, -1]
                [0.5, -0.5, 0.5, -1][0.5, 0.5, 0.5, -1][0.5, -0.5, -0.5, -1][0.5, 0.5, 0.5, -1][0.5, -0.5, -0.5, -1][0.5, 0.5, -0.5, -1]
                [0.5, 0.5, -0.5, -1][0.5, 0.5, 0.5, -1][-0.5, 0.5, -0.5, -1][0.5, 0.5, 0.5, -1][-0.5, 0.5, -0.5, -1][-0.5, 0.5, 0.5, -1]
                [-0.5, 0.5, 0.5, +1][-0.5, -0.5, 0.5, +1][0.5, 0.5, 0.5, +1][-0.5, -0.5, 0.5, +1][0.5, 0.5, 0.5, +1][0.5, -0.5, 0.5, +1]
                [0.5, -0.5, 0.5, +1][0.5, 0.5, 0.5, +1][0.5, -0.5, -0.5, +1][0.5, 0.5, 0.5, +1][0.5, -0.5, -0.5, +1][0.5, 0.5, -0.5, +1]
                [0.5, 0.5, -0.5, +1][0.5, 0.5, 0.5, +1][-0.5, 0.5, -0.5, +1][0.5, 0.5, 0.5, +1][-0.5, 0.5, -0.5, +1][-0.5, 0.5, 0.5, +1]
        }
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
            
            octahedron.xz = cubeConst.w * octahedron.y <= 0.0 ? invOctahedron : octahedron.xz;
            
            float isCurrentHemisphere = _step(0, cubeConst.w * (toLight.y) + 1.0);
            
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
            axis[3] : float4 =
                [0.0, 0.0, 1.0, 0.0]
                [1.0, 0.0, 0.0, 0.0]
                [0.0, 1.0, 0.0, 0.0]
                
            cube[12] : float4 =
                [-0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0][0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0]

            faceUV[4] : float2 = [0.0, 0.0][1.0, 0.0][0.0, 1.0][1.0, 1.0]
            lightPosition : float3 = [0, 0, 0]
        }
        inout {
            texcoord : float2
            koeff : float4
            gparams : float4
        }
        vssrc {
            float3 cubeCenter = float3(instance_position_color.xyz);
            float3 toCamera = frame_cameraPosition.xyz - cubeCenter;
            float3 toCamSign = _sign(toCamera);
            float4 relVertexPos = float4(toCamSign, 1.0) * fixed_cube[vertex_ID];
            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
            float3 faceNormal = toCamSign * fixed_axis[vertex_ID >> 2].xyz;

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

            //output_vpos = float4(absVertexPos.xyz, _step(cubeCenter.y, fixed_lightPosition.y));
            
            uint3 inormal = uint3(128.0 * faceNormal + 127.0);
            
            float viewSpaceDist = output_position.z / output_position.w;
            float packedNormal = float(inormal.x << 16 | inormal.y << 8 | inormal.z) / 16777216.0;
            
            output_position = _transform(absVertexPos, frame_viewProjMatrix);
            output_gparams = float4(0, 0, packedNormal, viewSpaceDist);
            output_texcoord = texcoord;
            output_koeff = koeff;
        }
        fssrc {
            output_color = input_gparams;

            //float3 toLight = fixed_lightPosition - input_vpos.xyz;
            //float3 toLightNrm = _norm(toLight);
            //float3 octahedron = toLightNrm / _dot(toLightNrm, _sign(toLight));
            //float  dist = (length(toLight) / 50.0f);
            //float2 coord = float2(0.5 + 0.5 * (octahedron.x + octahedron.z), 0.5 - 0.5 * (octahedron.z - octahedron.x));
            //float  shm = _tex2d(0, coord)[int(input_vpos.w)];

            //float m0 = _lerp(input_koeff[0], input_koeff[1], input_texcoord.x);
            //float m1 = _lerp(input_koeff[2], input_koeff[3], input_texcoord.x);
            //float k = _pow(_smooth(0, 1, _lerp(m0, m1, input_texcoord.y)), 0.5);// * step(0.96 * dist, shm);
            
            //output_color = float4(k, k, k, 1.0);
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
            float4 gbuffer = _tex2d(0, input_texcoord);
            float4 clipSpacePos = float4(input_texcoord * float2(2.0, -2.0) + float2(-1.0, 1.0), gbuffer.w, 1.0);
            float4 worldPosition = _transform(clipSpacePos, frame_invViewProjMatrix);
            worldPosition = worldPosition / worldPosition.w;
            
            float3 unpacking(1.0, 256.0, 65536.0);
            float3 normal = 2.0 * fract(unpacking * gbuffer.z) - 1.0;
            
            output_color = float4(normal, 1);
        }
    )";
    
    static const char *g_lightBollboardShaderSrc = R"(
        inout {
            texcoord : float2
        }
        vssrc {
            float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
            float4 billPos = float4(vertexCoord.xy * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 0.0);
            float4 viewPos = _transform(float4(0, 0, 0, 1), frame_viewMatrix) + billPos * 20.0;
            output_position = _transform(viewPos, frame_projMatrix);
            output_texcoord = vertexCoord;
        }
        fssrc {
            output_color = float4(input_texcoord, 0, 1);
        }
    )";
    
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
        math::vector4f positionRadius;
        math::color rgba;
    };
    
    class SceneImpl : public Scene {
    public:
        SceneImpl(const voxel::MeshFactoryPtr &factory, const foundation::RenderingInterfacePtr &rendering, const char *palette);
        ~SceneImpl() override;
        
        SceneObjectToken addStaticModel(const char *voxPath, const int16_t(&offset)[3]) override;
        SceneObjectToken addLightSource(const math::vector3f &position, float radius, const math::color &rgba) override;
        
        void removeStaticModel(SceneObjectToken token) override;
        void removeLightSource(SceneObjectToken token) override;
        
        void updateAndDraw(float dtSec) override;
        
    private:
        voxel::MeshFactoryPtr _factory;
        foundation::RenderingInterfacePtr _rendering;

        foundation::RenderShaderPtr _shadowShader;
        foundation::RenderShaderPtr _staticMeshShader;
        foundation::RenderShaderPtr _dynamicMeshShader;
        foundation::RenderShaderPtr _fullScreenQuadShader;
        foundation::RenderShaderPtr _lightBollboardShader;
        
        std::unordered_map<SceneObjectToken, std::unique_ptr<StaticVoxelModel>> _staticModels;
        std::unordered_map<SceneObjectToken, std::unique_ptr<LightSource>> _lightSources;

        foundation::RenderTexturePtr _gbuffer;
    };
    
    SceneImpl::SceneImpl(const voxel::MeshFactoryPtr &factory, const foundation::RenderingInterfacePtr &rendering, const char *palette) : _factory(factory), _rendering(rendering) {
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
        _shadowShader = rendering->createShader("scene_shadow", g_shadowShaderSrc,
            { // vertex
                {"ID", foundation::RenderShaderInputFormat::ID}
            },
            { // instance
                {"position", foundation::RenderShaderInputFormat::SHORT4},
            }
        );
        _fullScreenQuadShader = _rendering->createShader("scene_quad", g_fullScreenQuadShaderSrc, {
            {"ID", foundation::RenderShaderInputFormat::ID}
        });
        _lightBollboardShader = _rendering->createShader("scene_light", g_lightBollboardShaderSrc, {
            {"ID", foundation::RenderShaderInputFormat::ID}
        });
        
        _gbuffer = rendering->createRenderTarget(foundation::RenderTextureFormat::RGBA32F, _rendering->getBackBufferWidth(), _rendering->getBackBufferHeight(), true);
    }
    
    SceneImpl::~SceneImpl() {
        
    }
    
    std::shared_ptr<Scene> Scene::instance(const voxel::MeshFactoryPtr &factory, const foundation::RenderingInterfacePtr &rendering, const char *palette) {
        return std::make_shared<SceneImpl>(factory, rendering, palette);
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
        
        return INVALID_TOKEN;
    }
    
    SceneObjectToken SceneImpl::addLightSource(const math::vector3f &position, float radius, const math::color &rgba) {
        std::unique_ptr<LightSource> lightSource = std::make_unique<LightSource>();
        SceneObjectToken token = g_lastToken++ & std::uint64_t(0x7FFFFFFF);
        
        lightSource->distanceMap = _rendering->createRenderTarget(foundation::RenderTextureFormat::RGBA8UN, 1024, 1024, false);
        lightSource->positionRadius = math::vector4f(position, radius);
        lightSource->rgba = rgba;
        
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
                
                if (isInLight(model->bbMin, model->bbMax, light.positionRadius.xyz, light.positionRadius.w)) {
                    light.receiverTmpBuffer.emplace_back(model);
                }
            }
        }
        
//        for (auto &lightIt : _lightSources) {
//            LightSource &light = *lightIt.second;
//            _rendering->beginPass("scene_shadows", _shadowShader, foundation::RenderPassConfig(light.distanceMap, foundation::BlendType::MINVALUE, 1.0f, 1.0f, 1.0f));
//            _rendering->applyShaderConstants(&light.positionRadius);
//
//            for (const auto &item : light.receiverTmpBuffer) {
//                _rendering->drawGeometry(nullptr, item->positions, 36, item->positions->getCount(), foundation::RenderTopology::TRIANGLES);
//            }
//
//            _rendering->endPass();
//            light.receiverTmpBuffer.clear();
//        }
        
        // TODO:
        // 1. lights -> vector
        // 2. static_voxel_model + vertex texture fetch
        
        _rendering->beginPass("scene_static_voxel_models", _staticMeshShader, foundation::RenderPassConfig(_gbuffer, 0.0f, 0.0f, 0.0f));
        for (const auto &modelIt : _staticModels) {
            const StaticVoxelModel *model = modelIt.second.get();
            _rendering->drawGeometry(nullptr, model->voxels, 12, model->voxels->getCount(), foundation::RenderTopology::TRIANGLESTRIP);
        }
        _rendering->endPass();
        
        _rendering->beginPass("scene_final", _fullScreenQuadShader, foundation::RenderPassConfig(0.0f, 0.0f, 0.0f));
        _rendering->applyTextures(&_gbuffer, 1);
        _rendering->drawGeometry(nullptr, 4, foundation::RenderTopology::TRIANGLESTRIP);
        _rendering->endPass();
        
    }
}

