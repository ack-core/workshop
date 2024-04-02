
#include "scene.h"
#include "palette.h"

#include <cfloat>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <list>

#define LINES_IN_BLOCK_MAX 7
#define LINES_IN_BLOCK_MAX_2 14
#define MSTR(s) STR(s)
#define STR(s) #s

namespace voxel {
    class LineSetImpl : public SceneInterface::LineSet {
    public:
        struct LinesBlock {
            math::vector4f position = {0, 0, 0, 1};
            math::vector4f positions[LINES_IN_BLOCK_MAX_2];
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
        void setLine(std::uint32_t index, const math::vector3f &start, const math::vector3f &end, const math::color &rgba) override {
            std::uint32_t bindex = index / LINES_IN_BLOCK_MAX;
            std::uint32_t offset = index % LINES_IN_BLOCK_MAX;
            
            if (bindex < blocks.size()) {
                blocks[bindex].positions[offset * 2 + 0] = math::vector4f(start, 0.0f);
                blocks[bindex].positions[offset * 2 + 1] = math::vector4f(end, 0.0f);
                blocks[bindex].colors[offset] = rgba;
            }
        }
        
    public:
        LineSetImpl(bool depthTested, std::uint32_t count) : depthTested(depthTested) {
            std::uint32_t left = count % LINES_IN_BLOCK_MAX;
            for (std::uint32_t i = 0; i < count / LINES_IN_BLOCK_MAX; i++) {
                blocks.emplace_back().lineCount = LINES_IN_BLOCK_MAX;
            }
            if (left) {
                blocks.emplace_back().lineCount = left;
            }
        }
        ~LineSetImpl() override {}
    };
    
    //---
    
    class BoundingBoxImpl : public SceneInterface::BoundingBox {
    public:
        struct BBoxData {
            math::color color;
            math::vector4f min;
            math::vector4f max;
        }
        bboxData;
        bool depthTested;
        
    public:
        void setBBoxData(const math::bound3f &bbox) override {
            bboxData.min = math::vector4f(bbox.xmin, bbox.ymin, bbox.zmin, 1.0f);
            bboxData.max = math::vector4f(bbox.xmax, bbox.ymax, bbox.zmax, 1.0f);
        }
        void setColor(const math::color &rgba) override {
            bboxData.color = rgba;
        }
        
    public:
        BoundingBoxImpl(bool depthTested, const math::bound3f &bbox) : bboxData{math::color(1.0f, 1.0f, 1.0f, 1.0f)}, depthTested(depthTested) {
            setBBoxData(bbox);
        }
        ~BoundingBoxImpl() override {}
    };
    
    //---

    class StaticMeshImpl : public SceneInterface::StaticMesh {
    public:
        std::unique_ptr<foundation::RenderDataPtr[]> frames;
        std::uint32_t frameIndex;
        std::uint32_t frameCount;
        math::vector4f position = {0, 0, 0, 1};
        
    public:
        void setPosition(const math::vector3f &position) override {
            this->position = position.atv4start(1.0f);
        }
        void setFrame(std::uint32_t index) override {
            frameIndex = std::min(index, frameCount - 1);
        }        
        
    public:
        StaticMeshImpl(foundation::RenderDataPtr *frameArray, std::uint32_t count) {
            frameIndex = 0;
            frameCount = count;
            frames = std::make_unique<foundation::RenderDataPtr[]>(count);

            for (std::uint32_t i = 0; i < count; i++) {
                frames[i] = std::move(frameArray[i]);
            }
        }
        ~StaticMeshImpl() override {}
    };
    
    //---
    
    class DynamicMeshImpl : public SceneInterface::DynamicMesh {
    public:
        std::unique_ptr<foundation::RenderDataPtr[]> frames;
        std::uint32_t frameIndex;
        std::uint32_t frameCount;
        math::transform3f transform;
        
    public:
        DynamicMeshImpl(foundation::RenderDataPtr *frameArray, std::uint32_t count) : transform(math::transform3f::identity()) {
            frameIndex = 0;
            frameCount = count;
            frames = std::make_unique<foundation::RenderDataPtr[]>(count);

            for (std::uint32_t i = 0; i < count; i++) {
                frames[i] = std::move(frameArray[i]);
            }
        }
        ~DynamicMeshImpl() override {}

        void setTransform(const math::transform3f &trfm) override {
            transform = trfm;
        }
        void setFrame(std::uint32_t index) override {
            frameIndex = std::min(index, frameCount - 1);
        }
    };
    
    //---
    
    class TexturedMeshImpl : public SceneInterface::TexturedMesh {
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
        TexturedMeshImpl(foundation::RenderDataPtr &&v, foundation::RenderDataPtr &&i, const foundation::RenderTexturePtr &t) {
            vertices = std::move(v);
            indices = std::move(i);
            texture = t;
        }
        ~TexturedMeshImpl() override {}
    };
    
    //---
    
    class LightSourceImpl : public SceneInterface::LightSource {
    public:
        LightSourceImpl() {}
        ~LightSourceImpl() override {}
        
        void setPosition(const math::vector3f &position) override {
        }
    };
    
    //---
    
    class SceneInterfaceImpl : public SceneInterface {
    public:
        SceneInterfaceImpl(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering
        );
        ~SceneInterfaceImpl() override;
        
        void setCameraLookAt(const math::vector3f &position, const math::vector3f &sceneCenter) override;
        void setSun(const math::vector3f &directionToSun, const math::color &rgba) override;
        
        auto addLineSet(bool depthTested, std::uint32_t startCount) -> LineSetPtr override;
        auto addBoundingBox(bool depthTested, const math::bound3f &bbox) -> BoundingBoxPtr override;
        auto addStaticMesh(const std::vector<VTXSVOX> *frames, std::size_t frameCount) -> StaticMeshPtr override;
        auto addDynamicMesh(const std::vector<VTXDVOX> *frames, std::size_t frameCount) -> DynamicMeshPtr override;
        auto addTexturedMesh(const std::vector<VTXNRMUV> &vtx, const std::vector<std::uint32_t> &idx, const foundation::RenderTexturePtr &tx) -> TexturedMeshPtr override;
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
        
        foundation::RenderTexturePtr _palette;
        
        foundation::RenderShaderPtr _lineSetShader;
        foundation::RenderShaderPtr _boundingBoxShader;
        foundation::RenderShaderPtr _staticMeshShader;
        foundation::RenderShaderPtr _texturedMeshShader;
        foundation::RenderShaderPtr _dynamicMeshShader;
        foundation::RenderShaderPtr _axisShader;
        
        std::vector<std::shared_ptr<LineSetImpl>> _lineSets;
        std::vector<std::shared_ptr<BoundingBoxImpl>> _boundingBoxes;
        std::vector<std::shared_ptr<StaticMeshImpl>> _staticMeshes;
        std::vector<std::shared_ptr<TexturedMeshImpl>> _texturedMeshes;
        std::vector<std::shared_ptr<DynamicMeshImpl>> _dynamicMeshes;
    };
    
    std::shared_ptr<SceneInterface> SceneInterface::instance(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering) {
        return std::make_shared<SceneInterfaceImpl>(platform, rendering);
    }
}

namespace {
    const char *g_lineSetShaderSrc = R"(
        const {
            globalPosition : float4
            linePositions[)" MSTR(LINES_IN_BLOCK_MAX_2) R"(] : float4
            lineColors[)" MSTR(LINES_IN_BLOCK_MAX) R"(] : float4
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
        fixed {
            lines[24] : float3 =
                [0, 0, 0][1, 0, 0][0, 1, 1][1, 1, 1]
                [0, 0, 0][0, 0, 1][1, 1, 0][1, 1, 1]
                [0, 0, 1][1, 0, 1][0, 0, 0][0, 1, 0]
                [1, 0, 0][1, 0, 1][1, 0, 0][1, 1, 0]
                [0, 1, 0][1, 1, 0][0, 0, 1][0, 1, 1]
                [0, 1, 0][0, 1, 1][1, 0, 1][1, 1, 1]
        }
        const {
            color : float4
            bbmin : float4
            bbmax : float4
        }
        vssrc {
            float4 position = _lerp(const_bbmin, const_bbmax, float4(fixed_lines[repeat_ID], 0.0));
            output_position = _transform(position, _transform(frame_viewMatrix, frame_projMatrix));
        }
        fssrc {
            output_color[0] = const_color;
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
            paletteCoordinates : float2
        }
        vssrc {
            float3 cubeCenter = float3(vertex_position_color_mask.xyz) + const_globalPosition.xyz;
            float3 toCamSign = _sign(frame_cameraPosition.xyz - cubeCenter);
            
            int faceIndex = repeat_ID / 4 + int((toCamSign.zxy[repeat_ID / 4] * 1.5 + 1.5));
            int colorIndex = vertex_position_color_mask.w & 0xff;
            int mask = (vertex_position_color_mask.w >> (8 + faceIndex)) & 0x1;
            
            float4 relVertexPos = float4(toCamSign, 1.0) * _lerp(float4(0.5, 0.5, 0.5, 1.0), fixed_cube[repeat_ID], float(mask));
            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos + _step(0.0, relVertexPos) * float4(float3(vertex_scale_reserved.xyz), 0.0);
            
            output_paletteCoordinates = float2(float(colorIndex) / 255.0, 0.0); //
            output_position = _transform(absVertexPos, _transform(frame_viewMatrix, frame_projMatrix));
        }
        fssrc {
            output_color[0] = _tex2d(0, input_paletteCoordinates);
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
            paletteCoordinates : float2
        }
        vssrc {
            float3 cubeCenter = float3(vertex_position.xyz);
            float3 worldCubePos = _transform(float4(cubeCenter, 1.0), const_modelTransform).xyz;
            float3 toCamSign = _sign(_transform(const_modelTransform, float4(frame_cameraPosition.xyz - worldCubePos, 0.0)).xyz);
            
            int faceIndex = repeat_ID / 4 + int((toCamSign.zxy[repeat_ID / 4] * 1.5 + 1.5));
            int colorIndex = vertex_color_mask.r;
            int mask = (vertex_color_mask.g >> faceIndex) & 0x1;
            
            float4 relVertexPos = float4(toCamSign, 1.0) * _lerp(float4(0.5, 0.5, 0.5, 1.0), fixed_cube[repeat_ID], float(mask));
            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
            
            output_paletteCoordinates = float2(float(colorIndex) / 255.0, 0.0); //
            output_position = _transform(absVertexPos, _transform(const_modelTransform, _transform(frame_viewMatrix, frame_projMatrix)));
        }
        fssrc {
            output_color[0] = _tex2d(0, input_paletteCoordinates);
        }
    )";
    const char *g_texturedMeshShaderSrc = R"(
        const {
            globalPosition : float4
        }
        inout {
            uv : float2
            nrm : float3
        }
        vssrc {
            output_position = _transform(float4(vertex_position_u.xyz + const_globalPosition.xyz, 1.0), _transform(frame_viewMatrix, frame_projMatrix));
            output_uv = float2(vertex_position_u.w, vertex_normal_v.w);
            output_nrm = vertex_normal_v.xyz;
        }
        fssrc {
            float paletteIndex = _tex2d(0, input_uv).r;
            output_color[0] = _tex2d(1, float2(paletteIndex, 0.0));
        }
    )";
}

namespace voxel {
    SceneInterfaceImpl::SceneInterfaceImpl(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering
    )
    : _platform(platform)
    , _rendering(rendering)
    {
        _palette = rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, 256, 1, {resource::PALETTE});
        _lineSetShader = rendering->createShader("scene_static_lineset", g_lineSetShaderSrc, foundation::InputLayout {});
        _boundingBoxShader = rendering->createShader("scene_static_bounding_box", g_boundingBoxShaderSrc, foundation::InputLayout {
            .repeat = 24
        });
        _staticMeshShader = rendering->createShader("scene_static_voxel_mesh", g_staticMeshShaderSrc, foundation::InputLayout {
            .repeat = 12,
            .attributes = {
                {"position_color_mask", foundation::InputAttributeFormat::SHORT4},
                {"scale_reserved", foundation::InputAttributeFormat::BYTE4},
            }
        });
        _dynamicMeshShader = rendering->createShader("scene_dynamic_voxel_mesh", g_dynamicMeshShaderSrc, foundation::InputLayout {
            .repeat = 12,
            .attributes = {
                {"position", foundation::InputAttributeFormat::FLOAT3},
                {"color_mask", foundation::InputAttributeFormat::BYTE4},
            }
        });
        _texturedMeshShader = rendering->createShader("scene_static_textured_mesh", g_texturedMeshShaderSrc, foundation::InputLayout {
            .attributes = {
                {"position_u", foundation::InputAttributeFormat::FLOAT4},
                {"normal_v", foundation::InputAttributeFormat::FLOAT4},
            }
        });
        
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
    
    SceneInterface::BoundingBoxPtr SceneInterfaceImpl::addBoundingBox(bool depthTested, const math::bound3f &bbox) {
        return _boundingBoxes.emplace_back(std::make_shared<BoundingBoxImpl>(depthTested, bbox));
    }
    
    SceneInterface::StaticMeshPtr SceneInterfaceImpl::addStaticMesh(const std::vector<VTXSVOX> *frames, std::size_t frameCount) {
        std::shared_ptr<StaticMeshImpl> mesh = nullptr;
        std::unique_ptr<foundation::RenderDataPtr[]> voxFrames = std::make_unique<foundation::RenderDataPtr[]>(frameCount);
        
        for (std::size_t i = 0; i < frameCount; i++) {
            voxFrames[i] = _rendering->createData(frames[i].data(), _staticMeshShader->getInputLayout(),  std::uint32_t(frames[i].size()));
        }
        if (frameCount) {
            mesh = std::make_shared<StaticMeshImpl>(voxFrames.get(), frameCount);
            _staticMeshes.emplace_back(mesh);
        }
        
        return mesh;
    }
    
    SceneInterface::DynamicMeshPtr SceneInterfaceImpl::addDynamicMesh(const std::vector<VTXDVOX> *frames, std::size_t frameCount) {
        std::shared_ptr<DynamicMeshImpl> mesh = nullptr;
        std::unique_ptr<foundation::RenderDataPtr[]> voxFrames = std::make_unique<foundation::RenderDataPtr[]>(frameCount);

        for (std::size_t i = 0; i < frameCount; i++) {
            voxFrames[i] = _rendering->createData(frames[i].data(), std::uint32_t(frames[i].size()), sizeof(VTXDVOX));
        }

        if (frameCount) {
            mesh = std::make_shared<DynamicMeshImpl>(voxFrames.get(), frameCount);
            _dynamicMeshes.emplace_back(mesh);
        }
        
        return mesh;
    }
    
    SceneInterface::TexturedMeshPtr SceneInterfaceImpl::addTexturedMesh(const std::vector<VTXNRMUV> &vtx, const std::vector<std::uint32_t> &idx, const foundation::RenderTexturePtr &tx) {
        std::shared_ptr<TexturedMeshImpl> mesh = nullptr;
        foundation::RenderDataPtr vertexData = _rendering->createData(vtx.data(), _texturedMeshShader->getInputLayout(), std::uint32_t(vtx.size()));
        foundation::RenderDataPtr indexData = _rendering->createData(idx.data(), std::uint32_t(idx.size()), sizeof(std::uint32_t));

        if (vertexData && indexData && tx) {
            mesh = std::make_shared<TexturedMeshImpl>(std::move(vertexData), std::move(indexData), tx);
            _texturedMeshes.emplace_back(mesh);
        }
        
        return mesh;
    }
    
    SceneInterface::LightSourcePtr SceneInterfaceImpl::addLightSource(const math::vector3f &position, float r, float g, float b, float radius) {
        return nullptr;
    }
    
    math::vector2f SceneInterfaceImpl::getScreenCoordinates(const math::vector3f &worldPosition) {
        const math::transform3f vp = _camera.viewMatrix * _camera.projMatrix;
        math::vector4f tpos = math::vector4f(worldPosition, 1.0f);
        
        tpos = tpos.transformed(vp);
        tpos.x /= tpos.w;
        tpos.y /= tpos.w;
        tpos.z /= tpos.w;
        
        math::vector2f result = math::vector2f(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
        
        if (tpos.z > 0.0f) {
            result.x = (tpos.x + 1.0f) * 0.5f * _platform->getScreenWidth();
            result.y = (1.0f - tpos.y) * 0.5f * _platform->getScreenHeight();
        }
        
        return result;
    }
    
    math::vector3f SceneInterfaceImpl::getWorldDirection(const math::vector2f &screenPosition) {
        const math::transform3f inv = (_camera.viewMatrix * _camera.projMatrix).inverted();
        
        float relScreenPosX = 2.0f * screenPosition.x / _platform->getScreenWidth() - 1.0f;
        float relScreenPosY = 1.0f - 2.0f * screenPosition.y / _platform->getScreenHeight();
        
        const math::vector4f worldPos = math::vector4f(relScreenPosX, relScreenPosY, 0.0f, 1.0f).transformed(inv);
        return math::vector3f(worldPos.x / worldPos.w, worldPos.y / worldPos.w, worldPos.z / worldPos.w).normalized();
    }
    
    void SceneInterfaceImpl::updateAndDraw(float dtSec) {
        _cleanupUnused(_boundingBoxes);
        _cleanupUnused(_staticMeshes);
        _cleanupUnused(_texturedMeshes);
        _cleanupUnused(_dynamicMeshes);
        _rendering->updateFrameConstants(_camera.viewMatrix, _camera.projMatrix, _camera.position, _camera.forward);
        
        _rendering->applyState(_staticMeshShader, foundation::RenderTopology::TRIANGLESTRIP, foundation::RenderPassCommonConfigs::CLEAR(0.1, 0.1, 0.1));
        _rendering->applyTextures({
            {_palette, foundation::SamplerType::NEAREST}
        });
        for (const auto &staticMesh : _staticMeshes) {
            _rendering->applyShaderConstants(&staticMesh->position);
            _rendering->draw(staticMesh->frames[0]);
        }
        _rendering->applyState(_dynamicMeshShader, foundation::RenderTopology::TRIANGLESTRIP, foundation::RenderPassCommonConfigs::DEFAULT());
        _rendering->applyTextures({
            {_palette, foundation::SamplerType::NEAREST}
        });
        for (const auto &dynamicMesh : _dynamicMeshes) {
            _rendering->applyShaderConstants(&dynamicMesh->transform);
            _rendering->draw(dynamicMesh->frames[0]);
        }
        _rendering->applyState(_texturedMeshShader, foundation::RenderTopology::TRIANGLES, foundation::RenderPassCommonConfigs::DEFAULT());
        for (const auto &texturedMesh : _texturedMeshes) {
            _rendering->applyTextures({
                {texturedMesh->texture, foundation::SamplerType::NEAREST},
                {_palette, foundation::SamplerType::NEAREST}
            });
            _rendering->applyShaderConstants(&texturedMesh->position);
            _rendering->draw(texturedMesh->vertices, texturedMesh->indices);
        }
        
        _rendering->applyState(_lineSetShader, foundation::RenderTopology::LINES, foundation::RenderPassCommonConfigs::DEFAULT());
        for (const auto &set : _lineSets) {
            if (set->depthTested) {
                for (std::size_t i = 0; i < set->blocks.size(); i++) {
                    _rendering->applyShaderConstants(&set->blocks[i]);
                    _rendering->draw(set->blocks[i].lineCount * 2);
                }
            }
        }
        _rendering->applyState(_boundingBoxShader, foundation::RenderTopology::LINES, foundation::RenderPassCommonConfigs::DEFAULT());
        for (const auto &bbox : _boundingBoxes) {
            if (bbox->depthTested) {
                _rendering->applyShaderConstants(&bbox->bboxData);
                _rendering->draw();
            }
        }
        _rendering->applyState(_lineSetShader, foundation::RenderTopology::LINES, foundation::RenderPassCommonConfigs::OVERLAY(foundation::BlendType::MIXING));
        for (const auto &set : _lineSets) {
            if (set->depthTested == false) {
                for (std::size_t i = 0; i < set->blocks.size(); i++) {
                    _rendering->applyShaderConstants(&set->blocks[i]);
                    _rendering->draw(set->blocks[i].lineCount * 2);
                }
            }
        }
        _rendering->applyState(_boundingBoxShader, foundation::RenderTopology::LINES, foundation::RenderPassCommonConfigs::OVERLAY(foundation::BlendType::MIXING));
        for (const auto &bbox : _boundingBoxes) {
            if (bbox->depthTested == false) {
                _rendering->applyShaderConstants(&bbox->bboxData);
                _rendering->draw();
            }
        }
    }
}

