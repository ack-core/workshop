
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
            float3 relVertexPos = fixed_cube[vertex_ID].xyz;
            float4 absVertexPos = float4(cubeCenter + relVertexPos, 0.0);
            
            int ypos = int(absVertexPos.y + const_bounds.y);
            int ybase = int(const_bounds.z);
            float2 fromLight = absVertexPos.xz + const_bounds.yy + float2(ypos % ybase, ypos / ybase) * 2.0f * const_bounds.yy;
            
            output_position = float4((fromLight - const_bounds.xx) / float2(const_bounds.x, -const_bounds.x), 0.5, 1.0);
            output_discard = _step(_dot(cubeCenter, cubeCenter), const_positionRadius.w * const_positionRadius.w);
        }
        fssrc {
            output_color[0] = float4(0.75, 0.0, 0.0, input_discard);
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
            uvf : float3
        }
        vssrc {
            float3 cubeCenter = float3(instance_position_color.xyz);
            float3 toCamSign = _sign(frame_cameraPosition.xyz - cubeCenter);
            float4 relVertexPos = float4(toCamSign, 1.0) * fixed_cube[vertex_ID];
            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
            
            int3 faceIndices = int3(2.0 * _step(0.0, relVertexPos.yzy) + _step(0.0, relVertexPos.zxx));
            int  faceID = vertex_ID / 6;

            float3 faceNormal = toCamSign * fixed_axis[faceID];

            output_uvf = float3(0, 0, 0);
            output_uvf.xy = output_uvf.xy + _abs(faceNormal.x) * fixed_faceUV[faceIndices.x];
            output_uvf.xy = output_uvf.xy + _abs(faceNormal.y) * fixed_faceUV[faceIndices.y];
            output_uvf.xy = output_uvf.xy + _abs(faceNormal.z) * fixed_faceUV[faceIndices.z];
            
            float3 dirB = fixed_bnm[faceID];
            float3 dirT = fixed_tgt[faceID];
            
            output_corner = cubeCenter - 0.5 * (dirB + dirT - faceNormal);
            output_uvf.z = 0.16667 * (float(faceID) + (toCamSign.zxy[faceID] * 1.5 + 1.5) + 1.0);
            output_position = _transform(absVertexPos, frame_viewProjMatrix);
        }
        fssrc {
            float3 scaledCorner = (input_corner.xyz + 1024.0) / 2048.0f;

            output_color[0].xy = _frac(float2(1.0, 255.0) * scaledCorner.xx);
            output_color[0].x -= output_color[0].y * (1.0 / 255.0);

            output_color[0].zw = _frac(float2(1.0, 255.0) * scaledCorner.yy);
            output_color[0].z -= output_color[0].w * (1.0 / 255.0);

            output_color[1].xy = _frac(float2(1.0, 255.0) * scaledCorner.zz);
            output_color[1].x -= output_color[1].y * (1.0 / 255.0);
            output_color[1].zw = input_uvf.xy;
            
            output_color[2].x = input_uvf.z;
        }
    )";
    
    static const char *g_fullScreenBaseShaderSrc = R"(
        fixed {
            ambient[7] : float3 =
                [0.00, 0.00, 0.00]
                [0.24, 0.24, 0.23]
                [0.25, 0.25, 0.24]
                [0.23, 0.23, 0.22]
                [0.26, 0.26, 0.25]
                [0.28, 0.28, 0.27]
                [0.30, 0.30, 0.30]
        }
        inout {
            texcoord : float2
        }
        vssrc {
            float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
            output_position = float4(vertexCoord.xy * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.5, 1);
            output_texcoord = vertexCoord;
        }
        fssrc {
            float4 gbuffer0 = _tex2nearest(0, input_texcoord);
            float4 gbuffer1 = _tex2nearest(1, input_texcoord);
            float4 gbuffer2 = _tex2nearest(2, input_texcoord);
            int faceIndex = int(6.1 * gbuffer2.x);
            
            float3 cornerPosition;
            cornerPosition.x = _dot(gbuffer0.xy, float2(1.0, 1.0 / 255.0));
            cornerPosition.y = _dot(gbuffer0.zw, float2(1.0, 1.0 / 255.0));
            cornerPosition.z = _dot(gbuffer1.xy, float2(1.0, 1.0 / 255.0));
            cornerPosition = cornerPosition * 2048.0 - 1024.0;

            //output_color[0] = float4(gbuffer2.x, 0, 0, 1.0);
            output_color[0] = float4(fixed_ambient[faceIndex], 1.0);
        }
    )";
    
    static const char *g_lightBillboardShaderSrc = R"(
        fixed {
            axis[7] : float3 =
                [0.0, 0.0, 0.0]
                [0.0, 0.0, -1.0]
                [-1.0, 0.0, 0.0]
                [0.0, -1.0, 0.0]
                [0.0, 0.0, 1.0]
                [1.0, 0.0, 0.0]
                [0.0, 1.0, 0.0]
            
            bnm[7] : float3 =
                [0.0, 0.0, 0.0]
                [1.0, 0.0, 0.0]
                [0.0, 0.0, 1.0]
                [1.0, 0.0, 0.0]
                [1.0, 0.0, 0.0]
                [0.0, 0.0, 1.0]
                [1.0, 0.0, 0.0]
            
            tgt[7] : float3 =
                [0.0, 0.0, 0.0]
                [0.0, 1.0, 0.0]
                [0.0, 1.0, 0.0]
                [0.0, 0.0, 1.0]
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
            float4 gbuffer0 = _tex2nearest(0, fragment_coord);
            float4 gbuffer1 = _tex2nearest(1, fragment_coord);
            float4 gbuffer2 = _tex2nearest(2, fragment_coord);

            int faceIndex = int(6.1 * gbuffer2.x);
            
            float3 cornerPosition;
            cornerPosition.x = _dot(gbuffer0.xy, float2(1.0, 1.0 / 255.0));
            cornerPosition.y = _dot(gbuffer0.zw, float2(1.0, 1.0 / 255.0));
            cornerPosition.z = _dot(gbuffer1.xy, float2(1.0, 1.0 / 255.0));
            cornerPosition = cornerPosition * 2048.0 - 1024.0;
            
            float3 relCornerPos = cornerPosition - const_positionRadius.xyz;
            
            float3 dirB = fixed_bnm[faceIndex];
            float3 dirT = fixed_tgt[faceIndex];
            float3 faceNormal = fixed_axis[faceIndex];
            
            float koeff_0 = _tex2nearest(3, voxTexCoord(relCornerPos.xyz + faceNormal)).x;
            float koeff_1 = _tex2nearest(3, voxTexCoord(relCornerPos.xyz + faceNormal + dirB)).x;
            float koeff_2 = _tex2nearest(3, voxTexCoord(relCornerPos.xyz + faceNormal + dirT)).x;
            float koeff_3 = _tex2nearest(3, voxTexCoord(relCornerPos.xyz + faceNormal + dirB + dirT)).x;

            float2 f = gbuffer1.zw; //_smooth(0.0, 1.0, gbuffer1.zw);
            float2 smoothCoord = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
            
            float m0 = _lerp(koeff_0, koeff_1, smoothCoord.x);
            float m1 = _lerp(koeff_2, koeff_3, smoothCoord.x);
            float k = _lerp(m0, m1, smoothCoord.y);

            //output_color[0] = float4(fixed_axis[faceIndex] * 0.5 + 0.5, 1.0);
            output_color[0] = float4(k, k, k, 1.0);
        }
    )";

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
        
        _gbuffer = rendering->createRenderTarget(foundation::RenderTextureFormat::RGBA8UN, 4, _rendering->getBackBufferWidth(), _rendering->getBackBufferHeight(), true);
        //_gbuffer = rendering->createRenderTarget(foundation::RenderTextureFormat::RGBA32F, 1, _rendering->getBackBufferWidth(), _rendering->getBackBufferHeight(), true);
        
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

        // models among the lights
        for (const auto &modelIt : _staticModels) {
            const StaticVoxelModel *model = modelIt.second.get();
            
            for (auto &lightIt : _lightSources) {
                if (isInLight(model->bbMin, model->bbMax, lightIt.second->constants.positionRadius.xyz, lightIt.second->constants.positionRadius.w)) {
                    lightIt.second->receiverTmpBuffer.emplace_back(model);
                }
            }
        }
        
        // per-light voxel cube
        for (auto &lightIt : _lightSources) {
            _rendering->applyState(_voxelMapShader, foundation::RenderPassCommonConfigs::CLEAR(lightIt.second->voxelMap, foundation::BlendType::MAXVALUE, 0.0f, 0.0f, 0.0f, 0.0f));
            _rendering->applyShaderConstants(&lightIt.second->constants);
            
            for (const auto &item : lightIt.second->receiverTmpBuffer) {
                _rendering->drawGeometry(nullptr, item->voxels, 8, item->voxels->getCount(), foundation::RenderTopology::POINTS);
            }

            lightIt.second->receiverTmpBuffer.clear();
        }
        
        // gbuffer
        _rendering->applyState(_staticMeshGBuferShader, foundation::RenderPassCommonConfigs::CLEAR(_gbuffer, foundation::BlendType::DISABLED, 0.0f, 0.0f, 0.0f));
        for (const auto &modelIt : _staticModels) {
            const StaticVoxelModel *model = modelIt.second.get();
            _rendering->drawGeometry(nullptr, model->voxels, 18, model->voxels->getCount(), foundation::RenderTopology::TRIANGLES);
        }
        
        // base image
        textures[0] = _gbuffer->getTexture(0);
        textures[1] = _gbuffer->getTexture(1);
        textures[2] = _gbuffer->getTexture(2);
        textures[3] = _gbuffer->getTexture(3);

        _rendering->applyState(_fullScreenBaseShader, foundation::RenderPassCommonConfigs::DEFAULT());
        _rendering->applyTextures(textures, 4);
        _rendering->drawGeometry(nullptr, 4, foundation::RenderTopology::TRIANGLESTRIP);

        // per-light billboard
        _rendering->applyState(_lightBillboardShader, foundation::RenderPassCommonConfigs::OVERLAY(foundation::BlendType::ADDITIVE));
        for (auto &lightIt : _lightSources) {
            textures[3] = lightIt.second->voxelMap->getTexture(0);
            _rendering->applyTextures(textures, 4);
            _rendering->applyShaderConstants(&lightIt.second->constants);
            _rendering->drawGeometry(nullptr, 4, foundation::RenderTopology::TRIANGLESTRIP);
        }
        
    }
}

