
#include "scene.h"

#include <cfloat>
#include <memory>
#include <vector>
#include <unordered_map>

#include "thirdparty/upng/upng.h"

namespace voxel {
    struct StaticVoxelModel {
        struct Voxel {
            std::int16_t positionX, positionY, positionZ, colorIndex;
        };
        
        math::vector3f bbMin = {FLT_MAX, FLT_MAX, FLT_MAX};
        math::vector3f bbMax = {FLT_MIN, FLT_MIN, FLT_MIN};
        foundation::RenderDataPtr voxels;
    };
    struct DynamicVoxelModel {
        struct Voxel {
            std::int16_t positionX, positionY, positionZ, colorIndex;
            std::int32_t litPX, litNX, litPY, litNY, litPZ, litNZ;
        };

        math::vector3f position;
        float rotation;
        voxel::Mesh source;
        foundation::RenderDataPtr voxels;
    };
    
    class SceneInterfaceImpl : public SceneInterface {
    public:
        SceneInterfaceImpl(const foundation::RenderingInterfacePtr &rendering, const voxel::MeshFactoryPtr &factory, const char *palette);
        ~SceneInterfaceImpl() override;
        
        const std::shared_ptr<foundation::PlatformInterface> &getPlatformInterface() const override { return _platform; }
        const std::shared_ptr<foundation::RenderingInterface> &getRenderingInterface() const override { return _rendering; }

        void setCameraLookAt(const math::vector3f &position, const math::vector3f &target) override;
        void setObservingPoint(const math::vector3f &position) override;
        void setSun(const math::vector3f &direction, const math::color &rgba) override;

        SceneObjectToken addStaticModel(const char *voxPath, const int16_t(&offset)[3]) override;
        SceneObjectToken addDynamicModel(const char *voxPath, const math::vector3f &position, float rotation) override;
        
        void removeStaticModel(SceneObjectToken token) override;
        void removeDynamicModel(SceneObjectToken token) override;
        
        void updateAndDraw(float dtSec) override;
        
    private:
        void _updateMatrices();
        void _drawAxis();
        
        struct ShaderConst {
            math::vector4f sunDirection;
            math::vector4f sunColor_sunPower;
            math::vector4f observingPoint_radius;
            math::vector4f bounds;
        }
        _sunShaderConstants;
        
        struct Camera {
            math::transform3f viewMatrix;
            math::transform3f projMatrix;

            math::vector3f position;
            math::vector3f target;
            math::vector3f forward;
            math::vector3f up;
        }
        _camera;
        
        const foundation::PlatformInterfacePtr _platform;
        const foundation::RenderingInterfacePtr _rendering;
        const voxel::MeshFactoryPtr _factory;

        foundation::RenderShaderPtr _sunVoxelMapShader;
        foundation::RenderTargetPtr _sunVoxelMap;
        
        foundation::RenderShaderPtr _staticMeshGBuferShader;
        foundation::RenderShaderPtr _staticMeshScreenShader;

        foundation::RenderShaderPtr _tmpScreenQuadShader;
        foundation::RenderShaderPtr _axisShader;
        
        std::unordered_map<SceneObjectToken, std::unique_ptr<StaticVoxelModel>> _staticModels;
        std::unordered_map<SceneObjectToken, std::unique_ptr<DynamicVoxelModel>> _dynamicModels;

        foundation::RenderTexturePtr _palette;
        foundation::RenderTargetPtr _gbuffer;
    };
    
    std::shared_ptr<SceneInterface> SceneInterface::instance(const foundation::RenderingInterfacePtr &rendering, const voxel::MeshFactoryPtr &factory, const char *palette) {
        return std::make_shared<SceneInterfaceImpl>(rendering, factory, palette);
    }
}

// TODO: voxel map constants

namespace {
    const char *g_sunVoxelMapShaderSrc = R"(
        fixed {
            cube[8] : float4 =
                [-0.5, -0.5, -0.5, 1.0][0.5, -0.5, -0.5, 1.0][-0.5, -0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0]
                [-0.5, 0.5, -0.5, 1.0][0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
        }
        const {
            sunDirection : float4
            sunColor_sunPower : float4
            observingPoint_radius : float4
            bounds : float4
        }
        inout {
            discard : float1
        }
        vssrc {
            float3 cubeCenterOffset = float3(instance_position.xyz) - const_observingPoint_radius.xyz;
            float3 vertexPosInMap = cubeCenterOffset + fixed_cube[vertex_ID].xyz + float3(32.0, 32.0, 32.0);
            
            int ypos = int(vertexPosInMap.y);
            float2 vertexPosInTexture = vertexPosInMap.xz + float2(ypos % 8, ypos / 8) * float2(64.0, 64.0);
            float2 screenCoord = vertexPosInTexture / frame_rtBounds.xy * float2(2.0, -2.0) - float2(1.0, -1.0);

            output_position = float4(screenCoord, 0.5, 1.0);
            output_discard = _step(_dot(cubeCenterOffset, cubeCenterOffset), const_observingPoint_radius.w * const_observingPoint_radius.w);
        }
        fssrc {
            output_color[0] = float4(1.0 * input_discard, 0.0, 0.0, 0.0);
        }
    )";

//            axis[3] : float3 = [0.0, 0.0, 1.0][1.0, 0.0, 0.0][0.0, 1.0, 0.0]
//            bnm[3] : float3 = [1.0, 0.0, 0.0][0.0, 0.0, 1.0][1.0, 0.0, 0.0]
//            tgt[3] : float3 = [0.0, 1.0, 0.0][0.0, 1.0, 0.0][0.0, 0.0, 1.0]


    const char *g_staticMeshGBufferShaderSrc = R"(
        fixed {
            axis[7] : float3 = [0.0, 0.0, 0.0][0.0, 0.0, -1.0][-1.0, 0.0, 0.0][0.0, -1.0, 0.0][0.0, 0.0, 1.0][1.0, 0.0, 0.0][0.0, 1.0, 0.0]
            bnm[7] : float3 = [0.0, 0.0, 0.0][1.0, 0.0, 0.0][0.0, 0.0, 1.0][1.0, 0.0, 0.0][1.0, 0.0, 0.0][0.0, 0.0, 1.0][1.0, 0.0, 0.0]
            tgt[7] : float3 = [0.0, 0.0, 0.0][0.0, 1.0, 0.0][0.0, 1.0, 0.0][0.0, 0.0, 1.0][0.0, 1.0, 0.0][0.0, 1.0, 0.0][0.0, 0.0, 1.0]

            cube[18] : float4 =
                [-0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0]
                [0.5, -0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0]
            
            faceUV[4] : float2 = [0.0, 0.0][1.0, 0.0][0.0, 1.0][1.0, 1.0]
        }
        const {
            sunDirection : float4
            sunColor_sunPower : float4
            observingPoint_radius : float4
            bounds : float4
        }
        inout {
            uvf : float4
            koeffs : float4
        }
        fndef voxTexCoord (float3 offsetFromObservingPoint, float maxRadius) -> float2 {
            float3 positionInMap = offsetFromObservingPoint + float3(32.0, 32.0, 32.0);
            int ypos = int(positionInMap.y);
            
            float2 yoffset = float2(ypos % 8, ypos / 8) * float2(64, 64);
            return (positionInMap.xz + yoffset) / float2(512.0, 512.0) * _step(_dot(offsetFromObservingPoint, offsetFromObservingPoint), maxRadius * maxRadius);
        }
        vssrc {
            float3 cubeCenter = float3(instance_position_color.xyz);
            float3 toCamSign = _sign(frame_cameraPosition.xyz - cubeCenter);
            float4 relVertexPos = float4(toCamSign, 1.0) * fixed_cube[vertex_ID];
            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
            
            int3 faceIndices = int3(2.0 * _step(0.0, relVertexPos.yzy) + _step(0.0, relVertexPos.zxx));
            int  faceIndex = vertex_ID / 6 + int((toCamSign.zxy[vertex_ID / 6] * 1.5 + 1.5) + 1.0);

            float3 faceNormal = fixed_axis[faceIndex];
            float3 dirB = fixed_bnm[faceIndex];
            float3 dirT = fixed_tgt[faceIndex];

            output_uvf = float4(0, 0, 0, 0);
            output_uvf.xy = output_uvf.xy + _abs(faceNormal.x) * fixed_faceUV[faceIndices.x];
            output_uvf.xy = output_uvf.xy + _abs(faceNormal.y) * fixed_faceUV[faceIndices.y];
            output_uvf.xy = output_uvf.xy + _abs(faceNormal.z) * fixed_faceUV[faceIndices.z];

            float3 faceStart = cubeCenter - 0.5 * (dirB + dirT - faceNormal) + faceNormal - const_observingPoint_radius.xyz;
            output_koeffs.x = _tex2nearest(0, voxTexCoord(faceStart, const_observingPoint_radius.w)).x;
            output_koeffs.y = _tex2nearest(0, voxTexCoord(faceStart + dirB, const_observingPoint_radius.w)).x;
            output_koeffs.z = _tex2nearest(0, voxTexCoord(faceStart + dirT, const_observingPoint_radius.w)).x;
            output_koeffs.w = _tex2nearest(0, voxTexCoord(faceStart + dirB + dirT, const_observingPoint_radius.w)).x;
            
            output_position = _transform(absVertexPos, _transform(frame_viewMatrix, frame_projMatrix));
        }
        fssrc {
            float2 t = (input_uvf.xy);
            float m0 = _lerp(input_koeffs.x, input_koeffs.y, t.x);
            float m1 = _lerp(input_koeffs.z, input_koeffs.w, t.x);
            float oc = _lerp(m0, m1, t.y);

            oc = 0.5 + 0.5 * _pow(1.0 - _pow(oc, 4.6), 2.0);
            output_color[0] = float4(oc, oc, oc, 1);
        }
    )";
    
    static const char *g_staticMeshScreenShaderSrc = R"(
        fixed {
            ambient[7] : float3 = [0.00, 0.00, 0.00][0.24, 0.24, 0.23][0.25, 0.25, 0.24][0.23, 0.23, 0.22][0.26, 0.26, 0.25][0.28, 0.28, 0.27][0.30, 0.30, 0.30]
            axis[7] : float3 = [0.0, 0.0, 0.0][0.0, 0.0, -1.0][-1.0, 0.0, 0.0][0.0, -1.0, 0.0][0.0, 0.0, 1.0][1.0, 0.0, 0.0][0.0, 1.0, 0.0]
            bnm[7] : float3 = [0.0, 0.0, 0.0][1.0, 0.0, 0.0][0.0, 0.0, 1.0][1.0, 0.0, 0.0][1.0, 0.0, 0.0][0.0, 0.0, 1.0][1.0, 0.0, 0.0]
            tgt[7] : float3 = [0.0, 0.0, 0.0][0.0, 1.0, 0.0][0.0, 1.0, 0.0][0.0, 0.0, 1.0][0.0, 1.0, 0.0][0.0, 1.0, 0.0][0.0, 0.0, 1.0]
            koeffs[4] : float1 = [0.00][0.70][0.85][0.96]
        }
        const {
            sunDirection : float4
            sunColor_sunPower : float4
            observingPoint_radius : float4
            bounds : float4
        }
        inout {
            texcoord : float2
        }
        fndef voxTexCoord (float3 offsetFromObservingPoint, float maxRadius) -> float2 {
            float3 positionInMap = offsetFromObservingPoint + float3(32.0, 32.0, 32.0);
            int ypos = int(positionInMap.y);
            
            float2 yoffset = float2(ypos % 8, ypos / 8) * float2(64, 64);
            return (positionInMap.xz + yoffset) / float2(512.0, 512.0) * _step(_dot(offsetFromObservingPoint, offsetFromObservingPoint), maxRadius * maxRadius);
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
            
            float3 cornerPosition;
            cornerPosition.x = _dot(gbuffer0.xy, float2(1.0, 1.0 / 255.0));
            cornerPosition.y = _dot(gbuffer0.zw, float2(1.0, 1.0 / 255.0));
            cornerPosition.z = _dot(gbuffer1.xy, float2(1.0, 1.0 / 255.0));
            cornerPosition = cornerPosition * 2048.0 - 1024.0;

            float exist = 6.1 * gbuffer2.x;
            int faceIndex = int(exist);
            exist = _clamp(exist);

            float3 dirB = fixed_bnm[faceIndex];
            float3 dirT = fixed_tgt[faceIndex];
            float3 faceNormal = fixed_axis[faceIndex];
            float4 vox;
            
            vox.x = _tex2nearest(3, voxTexCoord(cornerPosition.xyz + faceNormal - const_observingPoint_radius.xyz, const_observingPoint_radius.w)).x;
            vox.y = _tex2nearest(3, voxTexCoord(cornerPosition.xyz + faceNormal + dirB - const_observingPoint_radius.xyz, const_observingPoint_radius.w)).x;
            vox.z = _tex2nearest(3, voxTexCoord(cornerPosition.xyz + faceNormal + dirT - const_observingPoint_radius.xyz, const_observingPoint_radius.w)).x;
            vox.w = _tex2nearest(3, voxTexCoord(cornerPosition.xyz + faceNormal + dirB + dirT - const_observingPoint_radius.xyz, const_observingPoint_radius.w)).x;

            int4 vi = int4(3.0 * _clamp(2.667 * vox));
            
            vox.x = fixed_koeffs[vi.x];
            vox.y = fixed_koeffs[vi.y];
            vox.z = fixed_koeffs[vi.z];
            vox.w = fixed_koeffs[vi.w];
            
            float2 smoothCoord = gbuffer1.zw;
            
            float m0 = _lerp(vox.x, vox.y, smoothCoord.x);
            float m1 = _lerp(vox.z, vox.w, smoothCoord.x);
            float oc = _lerp(m0, m1, smoothCoord.y);

            oc = exist * _pow(1.0 - _pow(oc, 2.6), 2.0);
            output_color[0] = float4(oc, oc, oc, 1.0);
        }
    )";
    
    static const char *g_tmpScreenQuadShaderSrc = R"(
        inout {
            texcoord : float2
        }
        vssrc {
            float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
            output_position = float4(vertexCoord.xy * float2(0.76762, -(1.365)) + float2(-0.96, 0.96), 0.7, 1);
            output_texcoord = vertexCoord;
        }
        fssrc {
            float4 s0 = _tex2nearest(0, input_texcoord);
            output_color[0] = s0 * 0.5;//_lerp(s0, s1, _step(0.8, input_texcoord.y));
        }
    )";

    voxel::SceneObjectToken g_lastToken = 0x100;
}

namespace voxel {
    SceneInterfaceImpl::SceneInterfaceImpl(const foundation::RenderingInterfacePtr &rendering, const voxel::MeshFactoryPtr &factory, const char *palette)
    : _platform(rendering->getPlatformInterface())
    , _rendering(rendering)
    , _factory(factory)
    {
        _sunVoxelMapShader = rendering->createShader("scene_voxel_map", g_sunVoxelMapShaderSrc,
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
        _staticMeshScreenShader = _rendering->createShader("scene_full_screen_base", g_staticMeshScreenShaderSrc, {
            {"ID", foundation::RenderShaderInputFormat::ID}
        });
        _tmpScreenQuadShader = _rendering->createShader("scene_screen_quad", g_tmpScreenQuadShaderSrc, {
            {"ID", foundation::RenderShaderInputFormat::ID}
        });
        
        _sunShaderConstants.observingPoint_radius = math::vector4f(0.0, 0.0, 0.0, 32.0);
        _sunShaderConstants.bounds = math::vector4f(256.0f, 32.0f, 8.0f, 0.0f);
        _sunShaderConstants.sunColor_sunPower = math::vector4f(1.0, 0.0, 0.0, 1.0);
        _sunVoxelMap = rendering->createRenderTarget(foundation::RenderTextureFormat::R8UN, 1, 512, 512, false);
        _gbuffer = rendering->createRenderTarget(foundation::RenderTextureFormat::RGBA8UN, 4, _rendering->getBackBufferWidth(), _rendering->getBackBufferHeight(), true);
        
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
    
    SceneInterfaceImpl::~SceneInterfaceImpl() {
        
    }
    
    void SceneInterfaceImpl::setCameraLookAt(const math::vector3f &position, const math::vector3f &target) {
        _camera.position = position;
        _camera.target = target;

        math::vector3f right = math::vector3f(0, 1, 0).cross(position - target);
        math::vector3f nrmlook = (target - position).normalized();
        math::vector3f nrmright = right.normalized();

        _camera.forward = nrmlook;
        _camera.up = nrmright.cross(nrmlook);
        _updateMatrices();
    }
    
    void SceneInterfaceImpl::setObservingPoint(const math::vector3f &position) {
        
    }
    
    void SceneInterfaceImpl::setSun(const math::vector3f &direction, const math::color &rgba) {
    
    }
    
    SceneObjectToken SceneInterfaceImpl::addStaticModel(const char *voxPath, const int16_t(&offset)[3]) {
        std::unique_ptr<StaticVoxelModel> model = std::make_unique<StaticVoxelModel>();
        SceneObjectToken token = g_lastToken++;
        voxel::Mesh source;

        if (_factory->createMesh(voxPath, offset, source)) {
            std::uint32_t voxelCount = source.frames[0].voxelCount;
            std::unique_ptr<StaticVoxelModel::Voxel[]> positions = std::make_unique<StaticVoxelModel::Voxel[]>(source.frames[0].voxelCount);
            
            for (std::uint32_t i = 0; i < voxelCount; i++) {
                positions[i].positionX = source.frames[0].voxels[i].positionX;
                positions[i].positionY = source.frames[0].voxels[i].positionY;
                positions[i].positionZ = source.frames[0].voxels[i].positionZ;
                positions[i].colorIndex = source.frames[0].voxels[i].colorIndex;
                
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
            
            model->voxels = _rendering->createData(positions.get(), voxelCount, sizeof(StaticVoxelModel::Voxel));
            
            if (model->voxels) {
                _staticModels.emplace(token, std::move(model));
                return token;
            }
        }
        
        _platform->logError("[Scene] model '%s' not loaded", voxPath);
        return INVALID_TOKEN;
    }
    
    SceneObjectToken SceneInterfaceImpl::addDynamicModel(const char *voxPath, const math::vector3f &position, float rotation) {
        return INVALID_TOKEN;
    }
    
    void SceneInterfaceImpl::removeStaticModel(SceneObjectToken token) {
        _staticModels.erase(token);
    }
    
    void SceneInterfaceImpl::removeDynamicModel(SceneObjectToken token) {

    }
    
    void SceneInterfaceImpl::updateAndDraw(float dtSec) {
        _rendering->updateFrameConstants(_camera.viewMatrix.flat16, _camera.projMatrix.flat16, _camera.position.flat3, _camera.forward.flat3);

        // sun voxel map
        _rendering->applyState(_sunVoxelMapShader, foundation::RenderPassCommonConfigs::CLEAR(_sunVoxelMap, foundation::BlendType::AGREGATION, 0.0f, 0.0f, 0.0f, 0.0f));
        _rendering->applyShaderConstants(&_sunShaderConstants);

        for (const auto &modelIt : _staticModels) {
            const StaticVoxelModel *model = modelIt.second.get();
            _rendering->drawGeometry(nullptr, model->voxels, 8, model->voxels->getCount(), foundation::RenderTopology::POINTS);
        }

        foundation::RenderTexturePtr textures[4] = {nullptr};

        // gbuffer
        //_rendering->applyState(_staticMeshGBuferShader, foundation::RenderPassCommonConfigs::CLEAR(_gbuffer, foundation::BlendType::DISABLED, 0.0f, 0.0f, 0.0f));
        _rendering->applyState(_staticMeshGBuferShader, foundation::RenderPassCommonConfigs::CLEAR(0.0, 0.0, 0.0));
        _rendering->applyShaderConstants(&_sunShaderConstants);
        _rendering->applyTextures(&_sunVoxelMap->getTexture(0), 1);

        for (const auto &modelIt : _staticModels) {
            const StaticVoxelModel *model = modelIt.second.get();
            _rendering->drawGeometry(nullptr, model->voxels, 18, model->voxels->getCount(), foundation::RenderTopology::TRIANGLES);
        }
        
        // base image
//        textures[0] = _gbuffer->getTexture(0);
//        textures[1] = _gbuffer->getTexture(1);
//        textures[2] = _gbuffer->getTexture(2);
//        textures[3] = _sunVoxelMap->getTexture(0);
//
//        _rendering->applyState(_staticMeshScreenShader, foundation::RenderPassCommonConfigs::DEFAULT());
//        _rendering->applyShaderConstants(&_sunShaderConstants);
//        _rendering->applyTextures(textures, 4);
//        _rendering->drawGeometry(nullptr, 4, foundation::RenderTopology::TRIANGLESTRIP);

//        textures[0] = _sunVoxelMap->getTexture(0);
//        _rendering->applyState(_tmpScreenQuadShader, foundation::RenderPassCommonConfigs::OVERLAY(foundation::BlendType::MIXING));
//        _rendering->applyTextures(textures, 1);
//        _rendering->drawGeometry(nullptr, 4, foundation::RenderTopology::TRIANGLESTRIP);
    }
    
    void SceneInterfaceImpl::_updateMatrices() {
        float aspect = _platform->getScreenWidth() / _platform->getScreenHeight();
        _camera.viewMatrix = math::transform3f::lookAtRH(_camera.position, _camera.target, _camera.up);
        _camera.projMatrix = math::transform3f::perspectiveFovRH(50.0 / 180.0f * float(3.14159f), aspect, 0.1f, 10000.0f);
    }
        
    void SceneInterfaceImpl::_drawAxis() {
        static const char *axisShaderSrc = R"(
            fixed {
                coords[10] : float4 =
                    [0.0, 0.0, 0.0, 1.0][1000.0, 0.0, 0.0, 1.0]
                    [0.0, 0.0, 0.0, 1.0][0.0, 1000.0, 0.0, 1.0]
                    [0.0, 0.0, 0.0, 1.0][0.0, 0.0, 1000.0, 1.0]
                    [1.0, 0.0, 0.0, 1.0][1.0, 0.0, 1.0, 1.0]
                    [0.0, 0.0, 1.0, 1.0][1.0, 0.0, 1.0, 1.0]
                colors[5] : float4 =
                    [1.0, 0.0, 0.0, 1.0]
                    [0.0, 1.0, 0.0, 1.0]
                    [0.0, 0.0, 1.0, 1.0]
                    [0.5, 0.5, 0.5, 1.0]
                    [0.5, 0.5, 0.5, 1.0]
            }
            inout {
                color : float4
            }
            vssrc {
                output_position = _transform(fixed_coords[vertex_ID], _transform(frame_viewMatrix, frame_projMatrix));
                output_color = fixed_colors[vertex_ID >> 1];
            }
            fssrc {
                output_color[0] = input_color;
            }
        )";

        if (_axisShader == nullptr) {
            _axisShader = _rendering->createShader("primitives_axis", axisShaderSrc, {
                {"ID", foundation::RenderShaderInputFormat::ID}
            });
        }
        
        _rendering->applyState(_axisShader, foundation::RenderPassCommonConfigs::CLEAR(0.0, 0.0, 0.0));
        _rendering->drawGeometry(nullptr, 10, foundation::RenderTopology::LINES);
    }
}

