
#include "scene.h"

#include <cfloat>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <list>

namespace voxel {
    class LineSetImpl : public SceneInterface::LineSet {
    public:
        static const std::size_t LINES_IN_BLOCK_MAX = 15;
    
    public:
        struct LinesBlock {
            math::vector4f position = {0, 0, 0, 1};
            math::vector4f positions[LINES_IN_BLOCK_MAX * 2];
            math::color colors[LINES_IN_BLOCK_MAX];
            std::uint32_t lineCount;
        };
        std::vector<LinesBlock> blocks;
        bool depthTested;
        
    public:
        void setPosition(const math::vector3f &position) override {
            for (auto &block : blocks) {
                block.position = math::vector4f(position, 1.0f);
            }
        }
        std::uint32_t addLine(const math::vector3f &start, const math::vector3f &end, const math::color &rgba) override {
            LinesBlock &block = blocks.size() == 0 || blocks.back().lineCount >= LINES_IN_BLOCK_MAX ? blocks.emplace_back() : blocks.back();
            block.positions[block.lineCount * 2 + 0] = math::vector4f(start, 0.0f);
            block.positions[block.lineCount * 2 + 1] = math::vector4f(end, 0.0f);
            block.colors[block.lineCount] = rgba;
            return std::uint32_t(blocks.size() - 1) * LINES_IN_BLOCK_MAX + block.lineCount++;
        }
        void setLine(std::uint32_t index, const math::vector3f &start, const math::vector3f &end, const math::color &rgba) override {
            std::uint32_t bindex = index / LINES_IN_BLOCK_MAX;
            std::uint32_t offset = index % LINES_IN_BLOCK_MAX;
            
            if (bindex < blocks.size()) {
                blocks[bindex].positions[offset * 2 + 0] = math::vector4f(start, 0.0f);
                blocks[bindex].positions[offset * 2 + 1] = math::vector4f(end, 0.0f);
                blocks[bindex].colors[offset] = rgba;
            }
        }
        void removeAllLines() override {
            blocks.clear();
        }

    public:
        LineSetImpl(bool depthTested, std::uint32_t startCount) : depthTested(depthTested) {
            std::uint32_t left = startCount % LINES_IN_BLOCK_MAX;
            for (std::uint32_t i = 0; i < startCount / LINES_IN_BLOCK_MAX; i++) {
                blocks.emplace_back().lineCount = LINES_IN_BLOCK_MAX;
            }
            if (left) {
                blocks.emplace_back().lineCount = left;
            }
        }
        ~LineSetImpl() override {}
    };

    class BoundingBoxImpl : public SceneInterface::BoundingBox {
    public:
        foundation::RenderDataPtr lines;
        math::vector4f position = {0, 0, 0, 1};
        
    public:
        void setPosition(const math::vector3f &position) override {
            this->position = position.atv4start(1.0f);
        }
        
    public:
        BoundingBoxImpl(foundation::RenderDataPtr &&v) : lines(std::move(v)) {}
        ~BoundingBoxImpl() override {}
    };

    class StaticModelImpl : public SceneInterface::StaticModel {
    public:
        foundation::RenderDataPtr voxels;
        math::vector4f position = {0, 0, 0, 1};
        
    public:
        void setPosition(const math::vector3f &position) override {
            this->position = position.atv4start(1.0f);
        }
        
    public:
        StaticModelImpl(foundation::RenderDataPtr &&v) : voxels(std::move(v)) {}
        ~StaticModelImpl() override {}
    };
    
    class TexturedModelImpl : public SceneInterface::TexturedModel {
    public:
        foundation::RenderDataPtr vertices;
        foundation::RenderDataPtr indices;
        foundation::RenderTexturePtr texture;
        math::vector4f position = {0, 0, 0, 1};
        
    public:
        void setPosition(const math::vector3f &position) override {
            this->position = position.atv4start(1.0f);
        }
    
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
        std::unique_ptr<foundation::RenderDataPtr[]> frames;
        std::uint32_t frameIndex;
        std::uint32_t frameCount;
        math::transform3f transform;
        
    public:
        DynamicModelImpl(foundation::RenderDataPtr *frameArray, std::uint32_t count, const math::transform3f &trfm) : transform(trfm) {
            frameIndex = 0;
            frameCount = count;
            frames = std::make_unique<foundation::RenderDataPtr[]>(count);

            for (std::uint32_t i = 0; i < count; i++) {
                frames[i] = std::move(frameArray[i]);
            }
        }
        ~DynamicModelImpl() override {}
        
        void setFrame(std::uint32_t index) override {
            frameIndex = std::min(index, frameCount);
        }
        void setTransform(const math::transform3f &trfm) override {
            transform = trfm;
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
            const resource::TextureProviderPtr &textureProvider,
            const foundation::RenderTexturePtr &palette,
            const dh::DataHubPtr &dh
        );
        ~SceneInterfaceImpl() override;
        
        void setCameraLookAt(const math::vector3f &position, const math::vector3f &sceneCenter) override;
        void setSun(const math::vector3f &directionToSun, const math::color &rgba) override;
        auto addLineSet(bool depthTested, std::uint32_t startCount) -> LineSetPtr override;
        auto addBoundingBox(const math::bound3f &bbox) -> BoundingBoxPtr override;
        auto addStaticModel(const std::vector<VTXSVOX> &voxels) -> StaticModelPtr override;
        auto addTexturedModel(const std::vector<VTXNRMUV> &vtx, const std::vector<std::uint32_t> &idx, const foundation::RenderTexturePtr &tx) -> TexturedModelPtr override;
        auto addDynamicModel(const std::vector<VTXDVOX> *frames, std::size_t frameCount, const math::transform3f &transform) -> DynamicModelPtr override;
        auto addLightSource(const math::vector3f &position, float r, float g, float b, float radius) -> LightSourcePtr override;
        auto getScreenCoordinates(const math::vector3f &worldPosition) -> math::vector2f override;
        auto getWorldDirection(const math::vector2f &screenPosition) -> math::vector3f override;
        void updateAndDraw(float dtSec) override;
        
    private:
        template<typename T> void _cleanupUnused(std::vector<T> &v) {
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
        const resource::TextureProviderPtr _textureProvider;
        const foundation::RenderTexturePtr _palette;
        const dh::DataHubPtr _dh;
        
        foundation::RenderShaderPtr _lineSetShader;
        foundation::RenderShaderPtr _boundingBoxShader;
        foundation::RenderShaderPtr _staticMeshShader;
        foundation::RenderShaderPtr _texturedMeshShader;
        foundation::RenderShaderPtr _dynamicMeshShader;
        foundation::RenderShaderPtr _axisShader;
        
        std::vector<std::shared_ptr<LineSetImpl>> _lineSets;
        std::vector<std::shared_ptr<BoundingBoxImpl>> _boundingBoxes;
        std::vector<std::shared_ptr<StaticModelImpl>> _staticMeshes;
        std::vector<std::shared_ptr<TexturedModelImpl>> _texturedMeshes;
        std::vector<std::shared_ptr<DynamicModelImpl>> _dynamicMeshes;
    };
    
    std::shared_ptr<SceneInterface> SceneInterface::instance(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const resource::TextureProviderPtr &textureProvider,
        const foundation::RenderTexturePtr &palette,
        const dh::DataHubPtr &dh
    ) {
        return std::make_shared<SceneInterfaceImpl>(platform, rendering, textureProvider, palette, dh);
    }
}

namespace {
    const char *g_lineSetShaderSrc = R"(
        const {
            globalPosition : float4
            linePositions[30] : float4
            lineColors[15] : float4
        }
        inout {
            color : float4
        }
        vssrc {
            float4 position = const_linePositions[vertex_ID] + const_globalPosition;
            output_position = _transform(position, _transform(frame_viewMatrix, frame_projMatrix));
            output_color = const_lineColors[vertex_ID >> 1];
        }
        fssrc {
            output_color[0] = input_color;
        }
    )";
    
    const char *g_boundingBoxShaderSrc = R"(
        const {
            globalPosition : float4
        }
        vssrc {
            output_position = _transform(const_globalPosition + vertex_position, _transform(frame_viewMatrix, frame_projMatrix));
        }
        fssrc {
            output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
        }
    )";
    
    const char *g_staticMeshShaderSrc = R"(
        fixed {
            cube[12] : float4 =
                [-0.5, 0.5, 0.5, 1.0][-0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0][0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0]
        }
        const {
            globalPosition : float4
        }
        inout {
            albedo : float2
        }
        vssrc {
            float3 cubeCenter = float3(instance_position_color_mask.xyz) + const_globalPosition.xyz;
            float3 toCamSign = _sign(frame_cameraPosition.xyz - cubeCenter);
            
            int  faceIndex = vertex_ID / 4 + int((toCamSign.zxy[vertex_ID / 4] * 1.5 + 1.5) + 1.0);
            int  colorIndex = instance_position_color_mask.w & 0xff;
            int  mask = (instance_position_color_mask.w >> (8 + faceIndex)) & 0x1;
            
            float4 relVertexPos = float4(toCamSign, 1.0) * _lerp(float4(0.5, 0.5, 0.5, 1.0), fixed_cube[vertex_ID], float(mask));
            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos + _step(0.0, relVertexPos) * float4(float3(instance_scale.xyz), 0.0);
            
            output_albedo = float2(colorIndex & 7, (256 - colorIndex) >> 3) / float2(7.0, 31.0); //
            output_position = _transform(absVertexPos, _transform(frame_viewMatrix, frame_projMatrix));
        }
        fssrc {
            output_color[0] = _tex2nearest(0, input_albedo);
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
            cube[12] : float4 =
                [-0.5, 0.5, 0.5, 1.0][-0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0][0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0]
        }
        const {
            modelTransform : matrix4
        }
        inout {
            albedo : float2
        }
        vssrc {
            float3 cubeCenter = float3(instance_position.xyz);
            float3 worldCubePos = _transform(float4(cubeCenter, 1.0), const_modelTransform).xyz;
            float3 toCamSign = _sign(_transform(const_modelTransform, float4(frame_cameraPosition.xyz - worldCubePos, 0.0)).xyz);
            
            int  faceIndex = vertex_ID / 4 + int((toCamSign.zxy[vertex_ID / 4] * 1.5 + 1.5) + 1.0);
            int  colorIndex = instance_color_mask.r;
            int  mask = (instance_color_mask.g >> faceIndex) & 0x1;
            
            float4 relVertexPos = float4(toCamSign, 1.0) * _lerp(float4(0.5, 0.5, 0.5, 1.0), fixed_cube[vertex_ID], float(mask));
            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
            
            output_albedo = float2(colorIndex & 7, (256 - colorIndex) >> 3) / float2(7.0, 31.0); //
            output_position = _transform(absVertexPos, _transform(const_modelTransform, _transform(frame_viewMatrix, frame_projMatrix)));
        }
        fssrc {
            output_color[0] = _tex2nearest(0, input_albedo);
        }
    )";
    
/*

    
    
        fixed {
            axs[7] : float3 = [0.0, 0.0, 0.0][0.0, 0.0, -1.0][-1.0, 0.0, 0.0][0.0, -1.0, 0.0][0.0, 0.0, 1.0][1.0, 0.0, 0.0][0.0, 1.0, 0.0]
            
            cube[18] : float4 =
                [-0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0]
                [0.5, -0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0]
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
            float3 cubeCenter = float3(instance_position.xyz);
            float3 toCamSign = _sign(_transform(const_modelTransform, float4(frame_cameraPosition.xyz - cubeCenter, 0.0)).xyz);
            float4 relVertexPos = float4(toCamSign, 1.0) * fixed_cube[vertex_ID];
            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
            
            //int3 faceIndices = int3(2.0 * _step(0.0, relVertexPos.yzy) + _step(0.0, relVertexPos.zxx));
            int  faceIndex = vertex_ID / 6 + int((toCamSign.zxy[vertex_ID / 6] * 1.5 + 1.5) + 1.0);
            
            float3 faceNormal = fixed_axs[faceIndex];
            //float3 dirB = fixed_bnm[faceIndex];
            //float3 dirT = fixed_tgt[faceIndex];
            
            output_tmp = float4(faceNormal, 1.0);
            output_position = _transform(absVertexPos, _transform(const_modelTransform, _transform(frame_viewMatrix, frame_projMatrix)));
        }
        fssrc {
            output_color[0] = input_tmp;
        }

 */
    
    
}

namespace voxel {
    SceneInterfaceImpl::SceneInterfaceImpl(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const resource::TextureProviderPtr &textureProvider,
        const foundation::RenderTexturePtr &palette,
        const dh::DataHubPtr &dh
    )
    : _platform(platform)
    , _rendering(rendering)
    , _textureProvider(textureProvider)
    , _palette(palette)
    , _dh(dh)
    {
        _lineSetShader = rendering->createShader("scene_static_lineset", g_lineSetShaderSrc,
            { // vertex
                {"ID", foundation::RenderShaderInputFormat::ID}
            },
            { // instance
            }
        );
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
                {"position_color_mask", foundation::RenderShaderInputFormat::SHORT4},
                {"scale", foundation::RenderShaderInputFormat::BYTE4},
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
                {"position", foundation::RenderShaderInputFormat::FLOAT3},
                {"color_mask", foundation::RenderShaderInputFormat::BYTE4},
            }
        );
                
        _shaderConstants.observingPointAndRadius = math::vector4f(0.0, 0.0, 0.0, 32.0);
        _shaderConstants.sunColorAndPower = math::vector4f(1.0, 0.0, 0.0, 1.0);
        _shaderConstants.sunDirection = math::vector4f(math::vector3f(0.2, 1.0, 0.3).normalized(), 1.0f);
        
        setCameraLookAt({45, 45, 45}, {0, 0, 0});
    }
    
    SceneInterfaceImpl::~SceneInterfaceImpl() {
        
    }
    
    void SceneInterfaceImpl::setCameraLookAt(const math::vector3f &position, const math::vector3f &sceneCenter) {
        _camera.position = position;
        _camera.target = sceneCenter;
        
        math::vector3f right = math::vector3f(0, 1, 0).cross(position - sceneCenter).normalized();
        math::vector3f nrmlook = (sceneCenter - position).normalized();
        math::vector3f nrmright = right.normalized();
        
        _camera.forward = nrmlook;
        _camera.up = nrmright.cross(nrmlook).normalized();
        
        float aspect = _platform->getScreenWidth() / _platform->getScreenHeight();
        _camera.viewMatrix = math::transform3f::lookAtRH(_camera.position, _camera.target, _camera.up);
        _camera.projMatrix = math::transform3f::perspectiveFovRH(50.0 / 180.0f * float(3.14159f), aspect, 0.1f, 10000.0f);
    }
    
    void SceneInterfaceImpl::setSun(const math::vector3f &directionToSun, const math::color &rgba) {
    
    }
    
    SceneInterface::LineSetPtr SceneInterfaceImpl::addLineSet(bool depthTested, std::uint32_t startCount) {
        return _lineSets.emplace_back(std::make_shared<LineSetImpl>(depthTested, startCount));
    }
    
    SceneInterface::BoundingBoxPtr SceneInterfaceImpl::addBoundingBox(const math::bound3f &bbox) {
        std::shared_ptr<BoundingBoxImpl> model = nullptr;
        math::vector4f data[24] = {
            math::vector4f(bbox.xmin, bbox.ymin, bbox.zmin, 0.0f), math::vector4f(bbox.xmax, bbox.ymin, bbox.zmin, 0.0f),
            math::vector4f(bbox.xmin, bbox.ymin, bbox.zmin, 0.0f), math::vector4f(bbox.xmin, bbox.ymin, bbox.zmax, 0.0f),
            math::vector4f(bbox.xmax, bbox.ymin, bbox.zmin, 0.0f), math::vector4f(bbox.xmax, bbox.ymin, bbox.zmax, 0.0f),
            math::vector4f(bbox.xmin, bbox.ymin, bbox.zmax, 0.0f), math::vector4f(bbox.xmax, bbox.ymin, bbox.zmax, 0.0f),
            
            math::vector4f(bbox.xmin, bbox.ymax, bbox.zmin, 0.0f), math::vector4f(bbox.xmax, bbox.ymax, bbox.zmin, 0.0f),
            math::vector4f(bbox.xmin, bbox.ymax, bbox.zmin, 0.0f), math::vector4f(bbox.xmin, bbox.ymax, bbox.zmax, 0.0f),
            math::vector4f(bbox.xmax, bbox.ymax, bbox.zmin, 0.0f), math::vector4f(bbox.xmax, bbox.ymax, bbox.zmax, 0.0f),
            math::vector4f(bbox.xmin, bbox.ymax, bbox.zmax, 0.0f), math::vector4f(bbox.xmax, bbox.ymax, bbox.zmax, 0.0f),
            
            math::vector4f(bbox.xmin, bbox.ymin, bbox.zmin, 0.0f), math::vector4f(bbox.xmin, bbox.ymax, bbox.zmin, 0.0f),
            math::vector4f(bbox.xmax, bbox.ymin, bbox.zmin, 0.0f), math::vector4f(bbox.xmax, bbox.ymax, bbox.zmin, 0.0f),
            math::vector4f(bbox.xmin, bbox.ymin, bbox.zmax, 0.0f), math::vector4f(bbox.xmin, bbox.ymax, bbox.zmax, 0.0f),
            math::vector4f(bbox.xmax, bbox.ymin, bbox.zmax, 0.0f), math::vector4f(bbox.xmax, bbox.ymax, bbox.zmax, 0.0f),
        };
        
        foundation::RenderDataPtr vertexData = _rendering->createData(data, 24, sizeof(math::vector4f));
        
        if (vertexData) {
            model = std::make_shared<BoundingBoxImpl>(std::move(vertexData));
            _boundingBoxes.emplace_back(model);
        }
        
        return model;
    }
    
    SceneInterface::StaticModelPtr SceneInterfaceImpl::addStaticModel(const std::vector<VTXSVOX> &voxels) {
        std::shared_ptr<StaticModelImpl> model = nullptr;
        foundation::RenderDataPtr voxData = _rendering->createData(voxels.data(), std::uint32_t(voxels.size()), sizeof(VTXSVOX));
        
        if (voxData) {
            model = std::make_shared<StaticModelImpl>(std::move(voxData));
            _staticMeshes.emplace_back(model);
        }
        
        return model;
    }
    
    SceneInterface::TexturedModelPtr SceneInterfaceImpl::addTexturedModel(const std::vector<VTXNRMUV> &vtx, const std::vector<std::uint32_t> &idx, const foundation::RenderTexturePtr &tx) {
        std::shared_ptr<TexturedModelImpl> model = nullptr;
        foundation::RenderDataPtr vertexData = _rendering->createData(vtx.data(), std::uint32_t(vtx.size()), sizeof(VTXNRMUV));
        foundation::RenderDataPtr indexData = _rendering->createData(idx.data(), std::uint32_t(idx.size()), sizeof(std::uint32_t));
        
        if (vertexData && indexData && tx) {
            model = std::make_shared<TexturedModelImpl>(std::move(vertexData), std::move(indexData), tx);
            _texturedMeshes.emplace_back(model);
        }
        
        return model;
    }
    
    SceneInterface::DynamicModelPtr SceneInterfaceImpl::addDynamicModel(const std::vector<VTXDVOX> *frames, std::size_t frameCount, const math::transform3f &transform) {
        std::shared_ptr<DynamicModelImpl> model = nullptr;
        std::unique_ptr<foundation::RenderDataPtr[]> voxFrames = std::make_unique<foundation::RenderDataPtr[]>(frameCount);
        
        for (std::size_t i = 0; i < frameCount; i++) {
            voxFrames[i] = _rendering->createData(frames[i].data(), std::uint32_t(frames[i].size()), sizeof(VTXDVOX));
        }
        
        if (frameCount) {
            model = std::make_shared<DynamicModelImpl>(voxFrames.get(), frameCount, transform);
            _dynamicMeshes.emplace_back(model);
        }
        
        return model;
    }
    
    SceneInterface::LightSourcePtr SceneInterfaceImpl::addLightSource(const math::vector3f &position, float r, float g, float b, float radius) {
        return nullptr;
    }
    
    math::vector2f SceneInterfaceImpl::getScreenCoordinates(const math::vector3f &worldPosition) {
        return _rendering->getScreenCoordinates(worldPosition);
    }
    
    math::vector3f SceneInterfaceImpl::getWorldDirection(const math::vector2f &screenPosition) {
        return _rendering->getWorldDirection(screenPosition);
    }
    
    void SceneInterfaceImpl::updateAndDraw(float dtSec) {
        _cleanupUnused(_boundingBoxes);
        _cleanupUnused(_staticMeshes);
        _cleanupUnused(_texturedMeshes);
        _cleanupUnused(_dynamicMeshes);
        _rendering->updateFrameConstants(_camera.viewMatrix.flat, _camera.projMatrix.flat, _camera.position.flat, _camera.forward.flat);
        
        _rendering->applyState(_lineSetShader, foundation::RenderPassCommonConfigs::CLEAR(0.1, 0.1, 0.1));
        for (const auto &set : _lineSets) {
            if (set->depthTested) {
                for (std::size_t i = 0; i < set->blocks.size(); i++) {
                    LineSetImpl::LinesBlock &block = set->blocks[i];
                    _rendering->applyShaderConstants(&set->blocks[i]);
                    _rendering->drawGeometry(nullptr, set->blocks[i].lineCount * 2, foundation::RenderTopology::LINES);
                }
            }
        }

        if (_texturedMeshes.empty() == false) {
            _rendering->applyState(_texturedMeshShader, foundation::RenderPassCommonConfigs::DEFAULT());
            for (const auto &texturedMesh : _texturedMeshes) {
                _rendering->applyTextures(&texturedMesh->texture, 1);
                _rendering->drawGeometry(texturedMesh->vertices, texturedMesh->indices, texturedMesh->indices->getCount(), foundation::RenderTopology::TRIANGLES);
            }
        }
        if (_staticMeshes.empty() == false) {
            _rendering->applyState(_staticMeshShader, foundation::RenderPassCommonConfigs::DEFAULT());
            _rendering->applyTextures(&_palette, 1);
            for (const auto &staticMesh : _staticMeshes) {
                _rendering->applyShaderConstants(&staticMesh->position);
                _rendering->drawGeometry(nullptr, staticMesh->voxels, 12, staticMesh->voxels->getCount(), foundation::RenderTopology::TRIANGLESTRIP);
            }
        }
        if (_dynamicMeshes.empty() == false) {
            _rendering->applyState(_dynamicMeshShader, foundation::RenderPassCommonConfigs::DEFAULT());
            _rendering->applyTextures(&_palette, 1);
            for (const auto &dynamicMesh : _dynamicMeshes) {
                _rendering->applyShaderConstants(&dynamicMesh->transform);
                const foundation::RenderDataPtr &voxelData = dynamicMesh->frames[dynamicMesh->frameIndex];
                const std::uint32_t voxelCount = dynamicMesh->frames[dynamicMesh->frameIndex]->getCount();
                _rendering->drawGeometry(nullptr, voxelData, 12, voxelCount, foundation::RenderTopology::TRIANGLESTRIP);
            }
        }

        if (_dh->getRootScope("graphics")->getBool("drawBBoxes")) {
            _rendering->applyState(_boundingBoxShader, foundation::RenderPassCommonConfigs::DEFAULT());
            for (const auto &boundingBox : _boundingBoxes) {
                _rendering->applyShaderConstants(&boundingBox->position);
                _rendering->drawGeometry(boundingBox->lines, 24, foundation::RenderTopology::LINES);
            }
        }
        
        _rendering->applyState(_lineSetShader, foundation::RenderPassCommonConfigs::OVERLAY(foundation::BlendType::MIXING));
        for (const auto &set : _lineSets) {
            if (set->depthTested == false) {
                for (std::size_t i = 0; i < set->blocks.size(); i++) {
                    _rendering->applyShaderConstants(&set->blocks[i]);
                    _rendering->drawGeometry(nullptr, set->blocks[i].lineCount * 2, foundation::RenderTopology::LINES);
                }
            }
        }
    }
}

