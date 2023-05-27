
#include "scene.h"

#include <cfloat>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <list>

#include "thirdparty/upng/upng.h"

namespace voxel {
    class StaticModelImpl : public SceneInterface::StaticModel {
    public:
        struct Voxel {
            std::int16_t positionX, positionY, positionZ, colorIndex;
        };
        
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
        
        StaticModelPtr addStaticModel(const voxel::Mesh &mesh, const int16_t(&offset)[3]) override;
        TexturedModelPtr addTexturedModel(const std::vector<VTXNRMUV> &vtx, const std::vector<std::uint32_t> &idx, const foundation::RenderTexturePtr &tx) override;
        DynamicModelPtr addDynamicModel(const voxel::Mesh &mesh, const math::vector3f &position, float rotation) override;
        LightSourcePtr addLightSource(const math::vector3f &position, float r, float g, float b, float radius) override;
        
        void updateAndDraw(float dtSec) override;
        
    private:
        void _updateMatrices();
        void _drawPoint(const math::vector3f &p, const math::color &rgba);
        void _drawLine(const math::vector3f &a, const math::vector3f &b, const math::color &rgba);
        void _drawTriangle(const math::vector3f &a, const math::vector3f &b, const math::vector3f &c, const math::color &rgba);
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

        foundation::RenderShaderPtr _staticMeshShader;
        foundation::RenderShaderPtr _texturedMeshShader;
        foundation::RenderShaderPtr _dynamicMeshShader;
        foundation::RenderShaderPtr _pointShader;
        foundation::RenderShaderPtr _lineShader;
        foundation::RenderShaderPtr _triangleShader;
        foundation::RenderShaderPtr _axisShader;
        
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

// TODO: voxel map constants

namespace {
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
            output_color[0] = _tex2linear(0, input_uv);
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
        _updateMatrices();
    }
    
    void SceneInterfaceImpl::setSun(const math::vector3f &directionToSun, const math::color &rgba) {
    
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
    
    SceneInterface::TexturedModelPtr
    SceneInterfaceImpl::addTexturedModel(const std::vector<VTXNRMUV> &vtx, const std::vector<std::uint32_t> &idx, const foundation::RenderTexturePtr &tx) {
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
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    }
    
    int dbg_counter = 0; //330;
    
    namespace voxel {
    
    
namespace {
    struct Edge {
        std::uint32_t a;
        std::uint32_t b;
        
        Edge(std::uint32_t first, std::uint32_t second) : a(first), b(second) {}
        
        bool operator==(const Edge &other) const {
            return (a == other.a && b == other.b) || (a == other.b && b == other.a);
        }
    };
    
//    struct EdgeHash
//    {
//        std::size_t operator()(const Edge& e) const noexcept
//        {
//            return e.a * e.b;
//        }
//    };
    
    bool isEdgeIntersect(const voxel::SceneInterface::VTXNRMUV &a1, const voxel::SceneInterface::VTXNRMUV &a2, const voxel::SceneInterface::VTXNRMUV &b1, const voxel::SceneInterface::VTXNRMUV &b2) {
            float d = (a2.x - a1.x) * (b2.z - b1.z) - (a2.z - a1.z) * (b2.x - b1.x);

            if (std::fabs(d) < std::numeric_limits<float>::epsilon()) {
                return false;
            }

            float u = ((b1.x - a1.x) * (b2.z - b1.z) - (b1.z - a1.z) * (b2.x - b1.x)) / d;
            float v = ((b1.x - a1.x) * (a2.z - a1.z) - (b1.z - a1.z) * (a2.x - a1.x)) / d;

            if (u <= 0.0f || u >= 1.0f || v <= 0.0f || v >= 1.0f)
            {
                return false;
            }

            return true;
    }

    void makeIndices(std::vector<voxel::SceneInterface::VTXNRMUV> &points, std::vector<Edge> &completeEdges, std::list<Edge> &computingEdges, std::vector<std::uint32_t> &indices) {
        computingEdges.clear();
        completeEdges.clear();
        computingEdges.emplace_back(Edge(0, 1));
        
        auto tryAddCompleteEdge = [&points, &computingEdges, &completeEdges](const Edge &newEdge) {
            for (auto index = completeEdges.rbegin(); index != completeEdges.rend(); ++index) {
                if (*index == newEdge || isEdgeIntersect(points[newEdge.a], points[newEdge.b], points[index->a], points[index->b])) {
                    return false;
                }
            }
            for (auto index = computingEdges.begin(); index != computingEdges.end(); ++index) {
                if (isEdgeIntersect(points[newEdge.a], points[newEdge.b], points[index->a], points[index->b])) {
                    return false;
                }
            }
            
            completeEdges.emplace_back(newEdge);
            return true;
        };

        auto tryAddComputingEdge = [&points, &computingEdges, &completeEdges](const Edge &newEdge) { //, &completeEdges
            bool doNotAdd = false;
            
            for (auto index = completeEdges.rbegin(); index != completeEdges.rend(); ++index) {
                if (*index == newEdge) {
                    doNotAdd = true;
                    break;
                }
                if (isEdgeIntersect(points[newEdge.a], points[newEdge.b], points[index->a], points[index->b])) {
                    return false;
                }
            }
            
            for (auto index = computingEdges.begin(); index != computingEdges.end(); ) {
                if (*index == newEdge) {
                    completeEdges.emplace_back(newEdge);
//                    points[newEdge.a].ny += 1.0f; points[newEdge.b].ny += 1.0f;
//                    points[newEdge.a].nx -= 1.0f; points[newEdge.b].nx -= 1.0f;
                    
                    index = computingEdges.erase(index);
                    doNotAdd = true;
                    break;
                }
                else {
                    if (isEdgeIntersect(points[newEdge.a], points[newEdge.b], points[index->a], points[index->b])) {
                        return false;
                    }
                    
                    ++index;
                }
            }
            
            if (doNotAdd == false) {
                computingEdges.emplace_back(newEdge);
                //points[newEdge.a].nx += 1.0f; points[newEdge.b].nx += 1.0f;
            }
            
            return true;
        };
        
        
        for (int cc = 0; cc < dbg_counter; cc++) {
        //while (computingEdges.empty() == false) {
//            if (cc == dbg_counter - 1) {
//                printf("");
//            }
            
            const Edge edge = computingEdges.front();
            const voxel::SceneInterface::VTXNRMUV &ea = points[edge.a];
            const voxel::SceneInterface::VTXNRMUV &eb = points[edge.b];
            const math::vector2f ab {eb.x - ea.x, eb.z - ea.z};
            const math::vector2f edgen = ab.normalized();
            
            computingEdges.pop_front();
            completeEdges.emplace_back(edge);
            points[edge.a].nx -= 1.0f; points[edge.b].nx -= 1.0f;
            points[edge.a].ny += 1.0f; points[edge.b].ny += 1.0f;

            float minDot = 1.0f;
            
            struct {
                std::uint32_t index;
                float cosa;
            }
            minIndeces[16];
            int minIndexCount = 0;
            
            for (std::uint32_t i = 0; i < points.size(); i++) {
                const voxel::SceneInterface::VTXNRMUV &pp = points[i];
                const math::vector2f ap {pp.x - ea.x, pp.z - ea.z};
                
                if (ab.cross(ap) < 0.0f) { //isPointAtLeft(points[i], edge)
                    const math::vector2f dirA = ap.normalized(); //math::vector2f(pp.x - ea.x, pp.z - ea.z)
                    const math::vector2f dirB = math::vector2f(pp.x - eb.x, pp.z - eb.z).normalized();
                    const float currentDot = dirA.dot(dirB);

                    if (currentDot < minDot - std::numeric_limits<float>::epsilon()) {
                        minDot = currentDot;
                        minIndexCount = 0;
                        minIndeces[minIndexCount].index = i;
                        minIndeces[minIndexCount].cosa = dirA.dot(edgen);
                        minIndexCount++;
                    }
                    else if (std::fabs(currentDot - minDot) < std::numeric_limits<float>::epsilon() && minIndexCount < 16) {
                        float cosa = dirA.dot(edgen);
                        
                        if (cosa >= 0.0f) {
                            minIndeces[minIndexCount].index = i;
                            minIndeces[minIndexCount].cosa = cosa; //dirA.dot(edgen);
                            minIndexCount++;
                        }
                    }
                }
            }
            
            for (int i = 0; i < minIndexCount - 1; ) {
                if (minIndeces[i].cosa > minIndeces[i + 1].cosa) {
                    std::swap(minIndeces[i], minIndeces[i + 1]);
                    i = 0;
                }
                else i++;
            }
            
            minIndeces[minIndexCount].index = edge.b;
            minIndeces[minIndexCount].cosa = 1.0f;
            
            for (int i = 1; i < minIndexCount + 1; i++) {
                const std::uint32_t pre = minIndeces[i - 1].index;
                const std::uint32_t cur = minIndeces[i].index;

                if (cur != edge.b) {
                    if (tryAddCompleteEdge(Edge(edge.a, cur))) {
                        indices.emplace_back(pre);
                        indices.emplace_back(cur);
                        indices.emplace_back(edge.a);
                    }
                    //completeEdges.emplace_back();
                }
                
                //const std::size_t pointsMax = points.size() - 2;
                
//                if (pre < pointsMax && cur < pointsMax && edge.a < pointsMax) {
//                }
                
                const Edge newEdge = Edge(pre, cur);
                tryAddComputingEdge(newEdge);
            }
            
            if (minIndexCount) {
                const Edge newEdge = Edge(edge.a, minIndeces[0].index);

                if (tryAddComputingEdge(newEdge)) {
                    indices.emplace_back(edge.a);
                    indices.emplace_back(edge.b);
                    indices.emplace_back(minIndeces[minIndexCount - 1].index);
                }
            }
            
        }
    }
    
    std::vector<voxel::SceneInterface::VTXNRMUV> points;
    std::vector<std::uint32_t> indices;
    
    std::vector<Edge> completeEdges;
    std::list<Edge> computingEdges;
}
    
    
    
    void SceneInterfaceImpl::updateAndDraw(float dtSec) {
        _rendering->updateFrameConstants(_camera.viewMatrix.flat16, _camera.projMatrix.flat16, _camera.position.flat3, _camera.forward.flat3);
        _drawAxis();
        
        /*
        points.clear();
        indices.clear();
        
        srand(100);
        
//        for (int c = 2; c < 34; c++) {
//            points.emplace_back(SceneInterface::VTXNRMUV{float(2), 0.0f, float(c), 0.0f, 0,0,0, 0.0f});
//            points.emplace_back(SceneInterface::VTXNRMUV{float(33), 0.0f, float(c), 0.0f, 0,0,0, 0.0f});
//        }
//        for (int i = 2; i < 34; i++) {
//            points.emplace_back(SceneInterface::VTXNRMUV{float(i), 0.0f, float(2), 0.0f, 0,0,0, 0.0f});
//            points.emplace_back(SceneInterface::VTXNRMUV{float(i), 0.0f, float(33), 0.0f, 0,0,0, 0.0f});
//        }


        for (int i = 2; i < 34; i++) {
            for (int c = 2; c < 34; c++) {
                if (i == 2 || c == 2 || i == 33 || c == 33) {
                //if ((rand() % 100) > 20) {
                    points.emplace_back(SceneInterface::VTXNRMUV{float(i), 0.0f, float(c), 0.0f, 0,0,0, 0.0f});
                }
            }
        }
        
        makeIndices(points, completeEdges, computingEdges, indices);
        
        for (int i = 0; i < points.size(); i++) {
            _drawPoint(math::vector3f(points[i].x, points[i].y, points[i].z), math::color(1.0, 0.0, 0.0, 1.0));
        }


        for (auto index = computingEdges.begin(); index != computingEdges.end(); ++index) {
            VTXNRMUV &p0 = points[index->a];
            VTXNRMUV &p1 = points[index->b];
            
            if (index == computingEdges.begin()) {
                _drawLine(math::vector3f(p0.x, p0.y, p0.z), math::vector3f(p1.x, p1.y, p1.z), math::color(0.0f, 1.0f, 0.0f, 1.0f));
            }
            else {
                _drawLine(math::vector3f(p0.x, p0.y, p0.z), math::vector3f(p1.x, p1.y, p1.z), math::color(0.5f, 0.5f, 0.5f, 1.0f));
            }
        }
        */
        
//        for (auto index = completeEdges.begin(); index != completeEdges.end(); ++index) {
//            VTXNRMUV &p0 = points[index->a];
//            VTXNRMUV &p1 = points[index->b];
//
//            _drawLine(math::vector3f(p0.x, p0.y, p0.z), math::vector3f(p1.x, p1.y, p1.z), math::color(0.5f, 0.25f, 0.0f, 1.0f));
//        }
        

//        _drawLine(math::vector3f(3.1, 0, 3.0), math::vector3f(3.1, 0, 7.0), math::color(1.0f, 1.0f, 0.0f, 1.0f));
//        _drawLine(math::vector3f(3.0, 0, 1.0), math::vector3f(3.0, 0, 7.0), math::color(1.0f, 1.0f, 0.0f, 1.0f));
//
//        printf("%d\n", isLinesIntersect({3.0, 3.0}, {3.1, 7.0}, {3.0, 1.0}, {3.0, 7.0}));

        for (int i = 0; i < indices.size() / 3; i++) {
            VTXNRMUV &p0 = points[indices[i * 3 + 0]];
            VTXNRMUV &p1 = points[indices[i * 3 + 1]];
            VTXNRMUV &p2 = points[indices[i * 3 + 2]];
            
            _drawTriangle(math::vector3f(p0.x, p0.y - 0.1f, p0.z), math::vector3f(p1.x, p1.y - 0.1f, p1.z), math::vector3f(p2.x, p2.y - 0.1f, p2.z), math::color(1.0f, 1.0f, 1.0f, 1.0f));
            //_drawLine(math::vector3f(p0.x, p0.y, p0.z), math::vector3f(p1.x, p1.y, p1.z), math::color(1.0f, 1.0f, 1.0f, 1.0f));
            //_drawLine(math::vector3f(p1.x, p1.y, p1.z), math::vector3f(p2.x, p2.y, p2.z), math::color(1.0f, 1.0f, 1.0f, 1.0f));
            //_drawLine(math::vector3f(p2.x, p2.y, p2.z), math::vector3f(p0.x, p0.y, p0.z), math::color(1.0f, 1.0f, 1.0f, 1.0f));
        }
        
        _rendering->applyState(_texturedMeshShader, foundation::RenderPassCommonConfigs::DEFAULT());
        for (const auto &texturedModel : _texturedModels) {
            _rendering->applyTextures(&texturedModel->texture, 1);
            _rendering->drawGeometry(texturedModel->vertices, texturedModel->indices, texturedModel->indices->getCount(), foundation::RenderTopology::TRIANGLES);
        }
        
//        if (prepared == false) {
//            prepared = true;
//
//            std::vector<math::vector2f> points;
//            std::vector<bool> flags;
//            std::vector<std::uint32_t> indices;
//
//            for (int i = 0; i < 9; i++) {
//                for (int c = 0; c < 9; c++) {
//                    //if ((rand() % 100) > 30) {
//                        flags.emplace_back(i != 3 || c >= 5);
//                        points.emplace_back(math::vector2f(c + 6, i + 6));
//                    //}
//                }
//            }
//
//            makeIndices(points, triangles, indices);
//        }
//
//        _drawLine({5, 0, 5}, {5, 0, 15}, {0.2, 0.2, 0.2, 0.5});
//        _drawLine({5, 0, 5}, {15, 0, 5}, {0.2, 0.2, 0.2, 0.5});
//        _drawLine({5, 0, 15}, {15, 0, 15}, {0.2, 0.2, 0.2, 0.5});
//        _drawLine({15, 0, 5}, {15, 0, 15}, {0.2, 0.2, 0.2, 0.5});

//        for (std::size_t i = 0; i < points.size(); i++) {
//            if (i == minimalIndex) {
//                _drawPoint(points[i].position, {1.0, 0.0, 0.0, 1.0});
//            }
//            else if (i == secondIndex) {
//                _drawPoint(points[i].position, {0.0, 0.0, 1.0, 1.0});
//            }
//            else {
//                _drawPoint(points[i].position, {0.0, 1.0, 0.0, 1.0});
//            }
//        }
        
//        for (auto &item : computingEdges) {
//            _drawLine(points[item.a].position, points[item.b].position, {0.5, 0.5, 0.5, 1.0});
//        }

        //_drawTriangle({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f});

//        srand(0);
//        for (auto &item : triangles) {
//            _drawTriangle(item.a, item.b, item.c, {float(rand() % 10) / 10.0f, float(rand() % 10) / 10.0f, float(rand() % 10) / 10.0f, 1.0});
//        }

  
//        _rendering->applyState(_staticMeshShader, foundation::RenderPassCommonConfigs::DEFAULT());
//        _rendering->applyShaderConstants(&_shaderConstants);
//        for (const auto &staticModel : _staticModels) {
//            _rendering->drawGeometry(nullptr, staticModel->voxels, 18, staticModel->voxels->getCount(), foundation::RenderTopology::TRIANGLES);
//        }
//
//        _rendering->applyState(_dynamicMeshShader, foundation::RenderPassCommonConfigs::DEFAULT());
//        for (const auto &dynamicModel : _dynamicModels) {
//            _shaderConstants.modelTransform = dynamicModel->transform;
//            _rendering->applyShaderConstants(&_shaderConstants);
//            _rendering->drawGeometry(nullptr, dynamicModel->voxels, 18, dynamicModel->voxels->getCount(), foundation::RenderTopology::TRIANGLES);
//        }



        // sun voxel map
//        _rendering->applyState(_sunVoxelMapShader, foundation::RenderPassCommonConfigs::CLEAR(_sunVoxelMap, foundation::BlendType::AGREGATION, 0.0f, 0.0f, 0.0f, 0.0f));
//        _rendering->applyShaderConstants(&_sunShaderConstants);
//
//        for (const auto &model : _staticModels) {
//            _rendering->drawGeometry(nullptr, model->voxels, 8, model->voxels->getCount(), foundation::RenderTopology::POINTS);
//        }
//
//        foundation::RenderTexturePtr textures[4] = {nullptr};
        
        // gbuffer
        //_rendering->applyState(_staticMeshGBuferShader, foundation::RenderPassCommonConfigs::CLEAR(_gbuffer, foundation::BlendType::DISABLED, 0.0f, 0.0f, 0.0f));
//        _rendering->applyState(_staticMeshGBuferShader, foundation::RenderPassCommonConfigs::CLEAR(0.0, 0.0, 0.0));
//        _rendering->applyShaderConstants(&_sunShaderConstants);
//        _rendering->applyTextures(&_sunVoxelMap->getTexture(0), 1);
//
//        for (const auto &model : _staticModels) {
//            _rendering->drawGeometry(nullptr, model->voxels, 18, model->voxels->getCount(), foundation::RenderTopology::TRIANGLES);
//        }
        
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
    
    void SceneInterfaceImpl::_drawPoint(const math::vector3f &p, const math::color &rgba) {
        static const char *pointShaderSrc = R"(
            const {
                coord : float4
                color : float4
            }
            inout {
                color : float4
            }
            vssrc {
                output_position = _transform(const_coord, _transform(frame_viewMatrix, frame_projMatrix));
                output_color = const_color;
            }
            fssrc {
                output_color[0] = input_color;
            }
        )";
        
        if (_pointShader == nullptr) {
            _pointShader = _rendering->createShader("scene_primitives_point", pointShaderSrc, {
                {"ID", foundation::RenderShaderInputFormat::ID}
            });
        }
        
        struct {
            math::vector4f coord;
            math::vector4f color;
        }
        constants {math::vector4f(p, 1.0), rgba};
        
        _rendering->applyState(_pointShader, foundation::RenderPassCommonConfigs::DEFAULT());
        _rendering->applyShaderConstants(&constants);
        _rendering->drawGeometry(nullptr, 1, foundation::RenderTopology::POINTS);
    }
    
    void SceneInterfaceImpl::_drawLine(const math::vector3f &a, const math::vector3f &b, const math::color &rgba) {
        static const char *lineShaderSrc = R"(
            const {
                coords[2] : float4
                color : float4
            }
            inout {
                color : float4
            }
            vssrc {
                output_position = _transform(const_coords[vertex_ID], _transform(frame_viewMatrix, frame_projMatrix));
                output_color = const_color * vertex_ID;
            }
            fssrc {
                output_color[0] = input_color;
            }
        )";
        
        if (_lineShader == nullptr) {
            _lineShader = _rendering->createShader("scene_primitives_line", lineShaderSrc, {
                {"ID", foundation::RenderShaderInputFormat::ID}
            });
        }
        
        struct {
            math::vector4f coordA;
            math::vector4f coordB;
            math::vector4f color;
        }
        constants {math::vector4f(a, 1.0), math::vector4f(b, 1.0), rgba};
        
        _rendering->applyState(_lineShader, foundation::RenderPassCommonConfigs::DEFAULT());
        _rendering->applyShaderConstants(&constants);
        _rendering->drawGeometry(nullptr, 2, foundation::RenderTopology::LINES);
    }
    
    void SceneInterfaceImpl::_drawTriangle(const math::vector3f &a, const math::vector3f &b, const math::vector3f &c, const math::color &rgba) {
        static const char *triangleShaderSrc = R"(
            const {
                coords[3] : float4
                color : float4
            }
            inout {
                color : float4
            }
            vssrc {
                output_position = _transform(const_coords[vertex_ID], _transform(frame_viewMatrix, frame_projMatrix));
                output_color = const_color;
            }
            fssrc {
                output_color[0] = input_color;
            }
        )";
        
        if (_triangleShader == nullptr) {
            _triangleShader = _rendering->createShader("scene_primitives_triangle", triangleShaderSrc, {
                {"ID", foundation::RenderShaderInputFormat::ID}
            });
        }
        
        struct {
            math::vector4f coordA;
            math::vector4f coordB;
            math::vector4f coordC;
            math::vector4f color;
        }
        constants {math::vector4f(a, 1.0), math::vector4f(b, 1.0), math::vector4f(c, 1.0), rgba};
        
        _rendering->applyState(_triangleShader, foundation::RenderPassCommonConfigs::DEFAULT());
        _rendering->applyShaderConstants(&constants);
        _rendering->drawGeometry(nullptr, 3, foundation::RenderTopology::TRIANGLES);
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
            _axisShader = _rendering->createShader("scene_primitives_axis", axisShaderSrc, {
                {"ID", foundation::RenderShaderInputFormat::ID}
            });
        }
        
        _rendering->applyState(_axisShader, foundation::RenderPassCommonConfigs::CLEAR(0.0, 0.0, 0.0));
        _rendering->drawGeometry(nullptr, 10, foundation::RenderTopology::LINES);
    }
}

