
#include "scene.h"

#include <cfloat>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <list>

namespace voxel {
    class BoundingBoxImpl : public SceneInterface::BoundingBox {
    public:
        foundation::RenderDataPtr lines;

    public:
        BoundingBoxImpl(foundation::RenderDataPtr &&v) : lines(std::move(v)) {}
        ~BoundingBoxImpl() override {}
    };

    class StaticModelImpl : public SceneInterface::StaticModel {
    public:
        struct Voxel {
            std::int16_t positionX, positionY, positionZ, colorIndex;
        };
        
    public:
        foundation::RenderDataPtr voxels;
        
    public:
        StaticModelImpl(foundation::RenderDataPtr &&v) : voxels(std::move(v)) {}
        ~StaticModelImpl() override {}
    };
    
    class TexturedModelImpl : public SceneInterface::TexturedModel {
    public:
        foundation::RenderDataPtr vertices;
        foundation::RenderDataPtr indices;
        foundation::RenderTexturePtr texture;
    
    public:
        TexturedModelImpl(foundation::RenderDataPtr &&v, foundation::RenderDataPtr &&i, const foundation::RenderTexturePtr &t) {
            vertices = std::move(v);
            indices = std::move(i);
            texture = t;
        }
        ~TexturedModelImpl() override {}
    };
    
    class DynamicModelImpl : public SceneInterface::DynamicModel {
    public:
        struct Voxel {
            std::int16_t positionX, positionY, positionZ, colorIndex;
        };
        
    public:
        foundation::RenderDataPtr voxels;
        math::transform3f transform;
        
    public:
        DynamicModelImpl(foundation::RenderDataPtr &&v, const math::transform3f &trfm) : voxels(std::move(v)), transform(trfm) {}
        ~DynamicModelImpl() override {}
        
        void setFrame(std::uint16_t index) override {
        
        }
        void setPosition(const math::vector3f &pos) override {
            transform._41 = pos.x;
            transform._42 = pos.y;
            transform._43 = pos.z;
        }
        void setRotation(float r) override {
            math::vector3f position = transform.translation();
            transform = math::transform3f({0.0f, 1.0f, 0.0f}, r).translated(position);
        }
    };
    
    class LightSourceImpl : public SceneInterface::LightSource {
    public:
        struct ShaderConst {
            math::vector4f positionAndRadius;
            math::vector4f color;
        }
        shaderConstants;
        
    public:
        LightSourceImpl() {}
        ~LightSourceImpl() override {}
        
        void setPosition(const math::vector3f &position) override {
            shaderConstants.positionAndRadius.xyz = position;
        }
    };
    
    class SceneInterfaceImpl : public SceneInterface {
    public:
        SceneInterfaceImpl(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            const voxel::TextureProviderPtr &textureProvider,
            const char *palette
        );
        ~SceneInterfaceImpl() override;
        
        void setCameraLookAt(const math::vector3f &position, const math::vector3f &sceneCenter) override;
        void setSun(const math::vector3f &directionToSun, const math::color &rgba) override;
        
        BoundingBoxPtr addBoundingBox(const math::bound3f &bbox) override;
        StaticModelPtr addStaticModel(const voxel::Mesh &mesh, const int16_t(&offset)[3]) override;
        TexturedModelPtr addTexturedModel(const std::vector<VTXNRMUV> &vtx, const std::vector<std::uint32_t> &idx, const foundation::RenderTexturePtr &tx) override;
        DynamicModelPtr addDynamicModel(const voxel::Mesh &mesh, const math::vector3f &position, float rotation) override;
        LightSourcePtr addLightSource(const math::vector3f &position, float r, float g, float b, float radius) override;
        
        void updateAndDraw(float dtSec) override;
        
    private:
        void _drawAxis();
        
        struct ShaderConst {
            math::transform3f modelTransform;
            math::vector4f sunDirection;
            math::vector4f sunColorAndPower;
            math::vector4f observingPointAndRadius;
        }
        _shaderConstants;
        
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
        const voxel::TextureProviderPtr _textureProvider;
        const voxel::MeshProviderPtr _factory;

        foundation::RenderShaderPtr _boundingBoxShader;
        foundation::RenderShaderPtr _staticMeshShader;
        foundation::RenderShaderPtr _texturedMeshShader;
        foundation::RenderShaderPtr _dynamicMeshShader;
        foundation::RenderShaderPtr _axisShader;
        
        std::vector<std::shared_ptr<BoundingBoxImpl>> _boundingBoxes;
        std::vector<std::shared_ptr<StaticModelImpl>> _staticModels;
        std::vector<std::shared_ptr<TexturedModelImpl>> _texturedModels;
        std::vector<std::shared_ptr<DynamicModelImpl>> _dynamicModels;

        foundation::RenderTexturePtr _palette;
    };
    
    std::shared_ptr<SceneInterface> SceneInterface::instance(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const voxel::TextureProviderPtr &textureProvider,
        const char *palette
    ) {
        return std::make_shared<SceneInterfaceImpl>(platform, rendering, textureProvider, palette);
    }
}

namespace {
    const char *g_boundingBoxShaderSrc = R"(
        vssrc {
            output_position = _transform(vertex_position, _transform(frame_viewMatrix, frame_projMatrix));
        }
        fssrc {
            output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
        }
    )";
    
    const char *g_staticMeshShaderSrc = R"(
        fixed {
            axs[7] : float3 = [0.0, 0.0, 0.0][0.0, 0.0, -1.0][-1.0, 0.0, 0.0][0.0, -1.0, 0.0][0.0, 0.0, 1.0][1.0, 0.0, 0.0][0.0, 1.0, 0.0]
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
            modelTransform : matrix4
            sunDirection : float4
            sunColorAndPower : float4
            observingPointAndRadius : float4
        }
        inout {
            tmp : float4
        }
        vssrc {
            float3 cubeCenter = float3(instance_position_color.xyz);
            float3 toCamSign = _sign(frame_cameraPosition.xyz - cubeCenter);
            float4 relVertexPos = float4(toCamSign, 1.0) * fixed_cube[vertex_ID];
            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
            
            int3 faceIndices = int3(2.0 * _step(0.0, relVertexPos.yzy) + _step(0.0, relVertexPos.zxx));
            int  faceIndex = vertex_ID / 6 + int((toCamSign.zxy[vertex_ID / 6] * 1.5 + 1.5) + 1.0);

            float3 faceNormal = fixed_axs[faceIndex];
            float3 dirB = fixed_bnm[faceIndex];
            float3 dirT = fixed_tgt[faceIndex];

            output_tmp = float4(faceNormal, 1.0);
            output_position = _transform(absVertexPos, _transform(frame_viewMatrix, frame_projMatrix));
        }
        fssrc {
            output_color[0] = input_tmp;
        }
    )";

    const char *g_texturedMeshShaderSrc = R"(
        inout {
            uv : float2
            nrm : float3
        }
        vssrc {
            output_position = _transform(float4(vertex_position_u.xyz, 1.0), _transform(frame_viewMatrix, frame_projMatrix));
            output_uv = float2(vertex_position_u.w, vertex_normal_v.w);
            output_nrm = vertex_normal_v.xyz;
        }
        fssrc {
            output_color[0] = _tex2nearest(0, input_uv);
        }
    )";
    
    const char *g_dynamicMeshShaderSrc = R"(
        fixed {
            axs[7] : float3 = [0.0, 0.0, 0.0][0.0, 0.0, -1.0][-1.0, 0.0, 0.0][0.0, -1.0, 0.0][0.0, 0.0, 1.0][1.0, 0.0, 0.0][0.0, 1.0, 0.0]
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
            modelTransform : matrix4
            sunDirection : float4
            sunColorAndPower : float4
            observingPointAndRadius : float4
        }
        inout {
            tmp : float4
        }
        vssrc {
            float3 cubeCenter = float3(instance_position_color.xyz);
            float3 toCamSign = _sign(_transform(const_modelTransform, float4(frame_cameraPosition.xyz - cubeCenter, 0.0)).xyz);
            float4 relVertexPos = float4(toCamSign, 1.0) * fixed_cube[vertex_ID];
            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
            
            int3 faceIndices = int3(2.0 * _step(0.0, relVertexPos.yzy) + _step(0.0, relVertexPos.zxx));
            int  faceIndex = vertex_ID / 6 + int((toCamSign.zxy[vertex_ID / 6] * 1.5 + 1.5) + 1.0);
            
            float3 faceNormal = fixed_axs[faceIndex];
            float3 dirB = fixed_bnm[faceIndex];
            float3 dirT = fixed_tgt[faceIndex];
            
            output_tmp = float4(faceNormal, 1.0);
            output_position = _transform(absVertexPos, _transform(const_modelTransform, _transform(frame_viewMatrix, frame_projMatrix)));
        }
        fssrc {
            output_color[0] = input_tmp;
        }
    )";
}

namespace voxel {
    SceneInterfaceImpl::SceneInterfaceImpl(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const voxel::TextureProviderPtr &textureProvider,
        const char *palette
    )
    : _platform(platform)
    , _rendering(rendering)
    , _textureProvider(textureProvider)
    {
        _boundingBoxShader = rendering->createShader("scene_static_bounding_box", g_boundingBoxShaderSrc,
            { // vertex
                {"position", foundation::RenderShaderInputFormat::FLOAT4},
            },
            { // instance
            }
        );
        
        _staticMeshShader = rendering->createShader("scene_static_voxel_mesh", g_staticMeshShaderSrc,
            { // vertex
                {"ID", foundation::RenderShaderInputFormat::ID}
            },
            { // instance
                {"position_color", foundation::RenderShaderInputFormat::SHORT4},
            }
        );

        _texturedMeshShader = rendering->createShader("scene_static_textured_mesh", g_texturedMeshShaderSrc,
            { // vertex
                {"position_u", foundation::RenderShaderInputFormat::FLOAT4},
                {"normal_v", foundation::RenderShaderInputFormat::FLOAT4},
            },
            { // instance
            }
        );

        _dynamicMeshShader = rendering->createShader("scene_dynamic_voxel_mesh", g_dynamicMeshShaderSrc,
            { // vertex
                {"ID", foundation::RenderShaderInputFormat::ID}
            },
            { // instance
                {"position_color", foundation::RenderShaderInputFormat::SHORT4},
            }
        );
        
        _shaderConstants.observingPointAndRadius = math::vector4f(0.0, 0.0, 0.0, 32.0);
        _shaderConstants.sunColorAndPower = math::vector4f(1.0, 0.0, 0.0, 1.0);
        _shaderConstants.sunDirection = math::vector4f(math::vector3f(0.2, 1.0, 0.3).normalized(), 1.0f);
        _palette = _textureProvider->getOrLoad2DTexture(palette);
    }
    
    SceneInterfaceImpl::~SceneInterfaceImpl() {
        
    }
    
    void SceneInterfaceImpl::setCameraLookAt(const math::vector3f &position, const math::vector3f &sceneCenter) {
        _camera.position = position;
        _camera.target = sceneCenter;
        
        math::vector3f right = math::vector3f(0, 1, 0).cross(position - sceneCenter);
        math::vector3f nrmlook = (sceneCenter - position).normalized();
        math::vector3f nrmright = right.normalized();
        
        _camera.forward = nrmlook;
        _camera.up = nrmright.cross(nrmlook);
        
        float aspect = _platform->getScreenWidth() / _platform->getScreenHeight();
        _camera.viewMatrix = math::transform3f::lookAtRH(_camera.position, _camera.target, _camera.up);
        _camera.projMatrix = math::transform3f::perspectiveFovRH(50.0 / 180.0f * float(3.14159f), aspect, 0.1f, 10000.0f);
    }
    
    void SceneInterfaceImpl::setSun(const math::vector3f &directionToSun, const math::color &rgba) {
    
    }
    
    SceneInterface::BoundingBoxPtr SceneInterfaceImpl::addBoundingBox(const math::bound3f &bbox) {
        std::shared_ptr<BoundingBoxImpl> model = nullptr;
        math::vector4f data[24] = {
            math::vector4f(bbox.xmin, bbox.ymin, bbox.zmin, 1.0f), math::vector4f(bbox.xmax, bbox.ymin, bbox.zmin, 1.0f),
            math::vector4f(bbox.xmin, bbox.ymin, bbox.zmin, 1.0f), math::vector4f(bbox.xmin, bbox.ymin, bbox.zmax, 1.0f),
            math::vector4f(bbox.xmax, bbox.ymin, bbox.zmin, 1.0f), math::vector4f(bbox.xmax, bbox.ymin, bbox.zmax, 1.0f),
            math::vector4f(bbox.xmin, bbox.ymin, bbox.zmax, 1.0f), math::vector4f(bbox.xmax, bbox.ymin, bbox.zmax, 1.0f),

            math::vector4f(bbox.xmin, bbox.ymax, bbox.zmin, 1.0f), math::vector4f(bbox.xmax, bbox.ymax, bbox.zmin, 1.0f),
            math::vector4f(bbox.xmin, bbox.ymax, bbox.zmin, 1.0f), math::vector4f(bbox.xmin, bbox.ymax, bbox.zmax, 1.0f),
            math::vector4f(bbox.xmax, bbox.ymax, bbox.zmin, 1.0f), math::vector4f(bbox.xmax, bbox.ymax, bbox.zmax, 1.0f),
            math::vector4f(bbox.xmin, bbox.ymax, bbox.zmax, 1.0f), math::vector4f(bbox.xmax, bbox.ymax, bbox.zmax, 1.0f),

            math::vector4f(bbox.xmin, bbox.ymin, bbox.zmin, 1.0f), math::vector4f(bbox.xmin, bbox.ymax, bbox.zmin, 1.0f),
            math::vector4f(bbox.xmax, bbox.ymin, bbox.zmin, 1.0f), math::vector4f(bbox.xmax, bbox.ymax, bbox.zmin, 1.0f),
            math::vector4f(bbox.xmin, bbox.ymin, bbox.zmax, 1.0f), math::vector4f(bbox.xmin, bbox.ymax, bbox.zmax, 1.0f),
            math::vector4f(bbox.xmax, bbox.ymin, bbox.zmax, 1.0f), math::vector4f(bbox.xmax, bbox.ymax, bbox.zmax, 1.0f),
        };

        foundation::RenderDataPtr vertexData = _rendering->createData(data, 24, sizeof(math::vector4f));
        
        if (vertexData) {
            model = std::make_shared<BoundingBoxImpl>(std::move(vertexData));
            _boundingBoxes.emplace_back(model);
        }
        
        return model;
    }
    
    SceneInterface::StaticModelPtr SceneInterfaceImpl::addStaticModel(const voxel::Mesh &mesh, const int16_t(&offset)[3]) {
        std::shared_ptr<StaticModelImpl> model = nullptr;
        std::uint32_t voxelCount = mesh.frames[0].voxelCount;
        std::unique_ptr<StaticModelImpl::Voxel[]> positions = std::make_unique<StaticModelImpl::Voxel[]>(mesh.frames[0].voxelCount);
        
        for (std::uint32_t i = 0; i < voxelCount; i++) {
            positions[i].positionX = mesh.frames[0].voxels[i].positionX;
            positions[i].positionY = mesh.frames[0].voxels[i].positionY;
            positions[i].positionZ = mesh.frames[0].voxels[i].positionZ;
            positions[i].colorIndex = mesh.frames[0].voxels[i].colorIndex;
        }
        
        foundation::RenderDataPtr voxels = _rendering->createData(positions.get(), voxelCount, sizeof(StaticModelImpl::Voxel));
        
        if (voxels) {
            model = std::make_shared<StaticModelImpl>(std::move(voxels));
            _staticModels.emplace_back(model);
        }
        
        return model;
    }
    
    SceneInterface::TexturedModelPtr SceneInterfaceImpl::addTexturedModel(const std::vector<VTXNRMUV> &vtx, const std::vector<std::uint32_t> &idx, const foundation::RenderTexturePtr &tx) {
        std::shared_ptr<TexturedModelImpl> model = nullptr;
        foundation::RenderDataPtr vertexData = _rendering->createData(vtx.data(), std::uint32_t(vtx.size()), sizeof(VTXNRMUV));
        foundation::RenderDataPtr indexData = _rendering->createData(idx.data(), std::uint32_t(idx.size()), sizeof(std::uint32_t));
        
        if (vertexData && indexData && tx) {
            model = std::make_shared<TexturedModelImpl>(std::move(vertexData), std::move(indexData), tx);
            _texturedModels.emplace_back(model);
        }
        
        return model;
    }
    
    SceneInterface::DynamicModelPtr SceneInterfaceImpl::addDynamicModel(const voxel::Mesh &mesh, const math::vector3f &position, float rotation) {
        std::uint32_t voxelCount = mesh.frames[0].voxelCount;
        std::unique_ptr<DynamicModelImpl::Voxel[]> positions = std::make_unique<DynamicModelImpl::Voxel[]>(mesh.frames[0].voxelCount);
        std::shared_ptr<DynamicModelImpl> model = nullptr;
        
        for (std::uint32_t i = 0; i < voxelCount; i++) {
            positions[i].positionX = mesh.frames[0].voxels[i].positionX;
            positions[i].positionY = mesh.frames[0].voxels[i].positionY;
            positions[i].positionZ = mesh.frames[0].voxels[i].positionZ;
            positions[i].colorIndex = mesh.frames[0].voxels[i].colorIndex;
        }
        
        foundation::RenderDataPtr voxels = _rendering->createData(positions.get(), voxelCount, sizeof(DynamicModelImpl::Voxel));
        
        if (voxels) {
            model = std::make_shared<DynamicModelImpl>(std::move(voxels), math::transform3f({0.0f, 1.0f, 0.0f}, rotation).translated(position));
            _dynamicModels.emplace_back(model);
        }
        
        return model;
    }
    
    SceneInterface::LightSourcePtr SceneInterfaceImpl::addLightSource(const math::vector3f &position, float r, float g, float b, float radius) {
        return nullptr;
    }
    
    namespace {
        template<typename T> void cleanupUnused(std::vector<T> &v) {
            for (auto index = v.begin(); index != v.end(); ) {
                if (index->use_count() <= 1) {
                    *index = v.back();
                    v.pop_back();
                }
                else {
                    ++index;
                }
            }
        }
    }
    
    void SceneInterfaceImpl::updateAndDraw(float dtSec) {
        cleanupUnused(_boundingBoxes);
        cleanupUnused(_staticModels);
        cleanupUnused(_texturedModels);
        cleanupUnused(_dynamicModels);
        
        _rendering->updateFrameConstants(_camera.viewMatrix.flat16, _camera.projMatrix.flat16, _camera.position.flat3, _camera.forward.flat3);
        _drawAxis();
        
        _rendering->applyState(_boundingBoxShader, foundation::RenderPassCommonConfigs::DEFAULT());
        for (const auto &boundingBox : _boundingBoxes) {
            _rendering->drawGeometry(boundingBox->lines, 24, foundation::RenderTopology::LINES);
        }
        
        _rendering->applyState(_texturedMeshShader, foundation::RenderPassCommonConfigs::DEFAULT());
        for (const auto &texturedModel : _texturedModels) {
            _rendering->applyTextures(&texturedModel->texture, 1);
            _rendering->drawGeometry(texturedModel->vertices, texturedModel->indices, texturedModel->indices->getCount(), foundation::RenderTopology::TRIANGLES);
        }
    }
    
//    void SceneInterfaceImpl::_drawPoint(const math::vector3f &p, const math::color &rgba) {
//        static const char *pointShaderSrc = R"(
//            const {
//                coord : float4
//                color : float4
//            }
//            inout {
//                color : float4
//            }
//            vssrc {
//                output_position = _transform(const_coord, _transform(frame_viewMatrix, frame_projMatrix));
//                output_color = const_color;
//            }
//            fssrc {
//                output_color[0] = input_color;
//            }
//        )";
//
//        if (_pointShader == nullptr) {
//            _pointShader = _rendering->createShader("scene_primitives_point", pointShaderSrc, {
//                {"ID", foundation::RenderShaderInputFormat::ID}
//            });
//        }
//
//        struct {
//            math::vector4f coord;
//            math::vector4f color;
//        }
//        constants {math::vector4f(p, 1.0), rgba};
//
//        _rendering->applyState(_pointShader, foundation::RenderPassCommonConfigs::DEFAULT());
//        _rendering->applyShaderConstants(&constants);
//        _rendering->drawGeometry(nullptr, 1, foundation::RenderTopology::POINTS);
//    }
//
//    void SceneInterfaceImpl::_drawLine(const math::vector3f &a, const math::vector3f &b, const math::color &rgba) {
//        static const char *lineShaderSrc = R"(
//            const {
//                coords[2] : float4
//                color : float4
//            }
//            inout {
//                color : float4
//            }
//            vssrc {
//                output_position = _transform(const_coords[vertex_ID], _transform(frame_viewMatrix, frame_projMatrix));
//                output_color = const_color * vertex_ID;
//            }
//            fssrc {
//                output_color[0] = input_color;
//            }
//        )";
//
//        if (_lineShader == nullptr) {
//            _lineShader = _rendering->createShader("scene_primitives_line", lineShaderSrc, {
//                {"ID", foundation::RenderShaderInputFormat::ID}
//            });
//        }
//
//        struct {
//            math::vector4f coordA;
//            math::vector4f coordB;
//            math::vector4f color;
//        }
//        constants {math::vector4f(a, 1.0), math::vector4f(b, 1.0), rgba};
//
//        _rendering->applyState(_lineShader, foundation::RenderPassCommonConfigs::DEFAULT());
//        _rendering->applyShaderConstants(&constants);
//        _rendering->drawGeometry(nullptr, 2, foundation::RenderTopology::LINES);
//    }
//
//    void SceneInterfaceImpl::_drawTriangle(const math::vector3f &a, const math::vector3f &b, const math::vector3f &c, const math::color &rgba) {
//        static const char *triangleShaderSrc = R"(
//            const {
//                coords[3] : float4
//                color : float4
//            }
//            inout {
//                color : float4
//            }
//            vssrc {
//                output_position = _transform(const_coords[vertex_ID], _transform(frame_viewMatrix, frame_projMatrix));
//                output_color = const_color;
//            }
//            fssrc {
//                output_color[0] = input_color;
//            }
//        )";
//
//        if (_triangleShader == nullptr) {
//            _triangleShader = _rendering->createShader("scene_primitives_triangle", triangleShaderSrc, {
//                {"ID", foundation::RenderShaderInputFormat::ID}
//            });
//        }
//
//        struct {
//            math::vector4f coordA;
//            math::vector4f coordB;
//            math::vector4f coordC;
//            math::vector4f color;
//        }
//        constants {math::vector4f(a, 1.0), math::vector4f(b, 1.0), math::vector4f(c, 1.0), rgba};
//
//        _rendering->applyState(_triangleShader, foundation::RenderPassCommonConfigs::DEFAULT());
//        _rendering->applyShaderConstants(&constants);
//        _rendering->drawGeometry(nullptr, 3, foundation::RenderTopology::TRIANGLES);
//    }
    
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
            _axisShader = _rendering->createShader("scene_primitives_axis", axisShaderSrc, {
                {"ID", foundation::RenderShaderInputFormat::ID}
            });
        }
        
        _rendering->applyState(_axisShader, foundation::RenderPassCommonConfigs::CLEAR(0.0, 0.0, 0.0));
        _rendering->drawGeometry(nullptr, 10, foundation::RenderTopology::LINES);
    }
}

