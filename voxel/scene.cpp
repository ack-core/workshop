
#include "scene.h"
#include "palette.h"
#include "foundation/layouts.h"

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
        LineSetImpl(std::uint32_t count) {
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
        
    public:
        void setBBoxData(const math::bound3f &bbox) override {
            bboxData.min = math::vector4f(bbox.xmin, bbox.ymin, bbox.zmin, 1.0f);
            bboxData.max = math::vector4f(bbox.xmax, bbox.ymax, bbox.zmax, 1.0f);
        }
        void setColor(const math::color &rgba) override {
            bboxData.color = rgba;
        }
        
    public:
        BoundingBoxImpl(const math::bound3f &bbox) : bboxData{math::color(1.0f, 1.0f, 1.0f, 1.0f)} {
            setBBoxData(bbox);
        }
        ~BoundingBoxImpl() override {}
    };
    
    //---

    class StaticMeshImpl : public SceneInterface::StaticMesh {
    public:
        foundation::RenderDataPtr mesh;
        math::vector4f position = {0, 0, 0, 1};
        
    public:
        void setPosition(const math::vector3f &position) override {
            this->position = position.atv4start(1.0f);
        }
        
    public:
        StaticMeshImpl(const foundation::RenderDataPtr &m) : mesh(m) {}
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
        DynamicMeshImpl(const foundation::RenderDataPtr *frameArray, std::uint32_t count) : transform(math::transform3f::identity()) {
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
        auto getFrameCount() -> std::uint32_t override {
            return frameCount;
        }
        
    };
    
    //---
    
    class TexturedMeshImpl : public SceneInterface::TexturedMesh {
    public:
        foundation::RenderDataPtr data;
        foundation::RenderTexturePtr texture;
        math::vector4f position = {0, 0, 0, 1};
        
    public:
        void setPosition(const math::vector3f &position) override {
            this->position = position.atv4start(1.0f);
        }
        
    public:
        TexturedMeshImpl(const foundation::RenderDataPtr &idx, const foundation::RenderTexturePtr &tx) : data(idx), texture(tx) {}
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
    
    class ParticlesImpl : public SceneInterface::Particles {
    public:
        foundation::RenderTexturePtr texture;
        foundation::RenderTexturePtr map;
                
        ParticlesImpl(const foundation::RenderTexturePtr &texture, const foundation::RenderTexturePtr &map, const SceneInterface::EmitterParams &emitterParams)
        : texture(texture)
        , map(map)
        , _constants({})
        {
            if (emitterParams.orientation == SceneInterface::EmitterParams::Orientation::AXIS) {
                _updateOrientation = [](ParticlesImpl &self, const math::vector3f &camDir, const math::vector3f &camRight) {
                    self._constants.position = math::vector3f(self._transform.m41, self._transform.m42, self._transform.m43);
                    self._constants.right = camRight;
                    self._constants.normal = -1.0 * camDir;
                };
            }
            else if (emitterParams.orientation == SceneInterface::EmitterParams::Orientation::WORLD) {
                _updateOrientation = [](ParticlesImpl &self, const math::vector3f &camDir, const math::vector3f &camRight) {
                    self._constants.position = math::vector3f(self._transform.m41, self._transform.m42, self._transform.m43);
                    self._constants.right = math::vector3f(self._transform.m11, self._transform.m12, self._transform.m13);
                    self._constants.normal = math::vector3f(self._transform.m21, self._transform.m22, self._transform.m23);
                };
            }
            else {
                _updateOrientation = [](ParticlesImpl &self, const math::vector3f &camDir, const math::vector3f &camRight) {
                    self._constants.position = math::vector3f(self._transform.m41, self._transform.m42, self._transform.m43);
                    self._constants.right = camRight;
                    self._constants.normal = -1.0 * camDir;
                };
            }
        }
        ~ParticlesImpl() override {}
        
        void *getUpdatedConstants(const math::vector3f &camDir, const math::vector3f &camRight) {
            _updateOrientation(*this, camDir, camRight);
            return &_constants;
        }
        void setTransform(const math::transform3f &trfm) override {
            _transform = trfm;
        }
        void setTime(float timeSec) override {
            
        }
        
    private:
        SceneInterface::EmitterParams::Orientation _orientation;
        math::transform3f _transform = math::transform3f::identity();

        struct Constants {
            math::vector3f position; float time;
            math::vector3f normal; float alpha;
            math::vector3f right; float r0;
        }
        _constants;
        
        void (*_updateOrientation)(ParticlesImpl &self, const math::vector3f &camRight, const math::vector3f &camUp);
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
        
        auto addLineSet(std::uint32_t count) -> LineSetPtr override;
        auto addBoundingBox(const math::bound3f &bbox) -> BoundingBoxPtr override;
        auto addStaticMesh(const foundation::RenderDataPtr &mesh) -> StaticMeshPtr override;
        auto addDynamicMesh(const std::vector<foundation::RenderDataPtr> &frames) -> DynamicMeshPtr override;
        auto addTexturedMesh(const foundation::RenderDataPtr &mesh, const foundation::RenderTexturePtr &texture) -> TexturedMeshPtr override;
        auto addParticles(const foundation::RenderTexturePtr &tx, const foundation::RenderTexturePtr &map, const EmitterParams &emitter) -> ParticlesPtr override;
        auto addLightSource(float r, float g, float b, float radius) -> LightSourcePtr override;
        
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
            math::transform3f plmVPMatrix; // plm matrix transforms to clip space with platform-specific z-range
            math::transform3f stdVPMatrix; // std matrix transforms to clip space with z-range [0..1]
            math::transform3f invVPMatrix; // inv matrix assumes that clip space has z-range [0..1]
            math::vector3f position;
            math::vector3f target;
            math::vector3f forward;
            math::vector3f right;
            math::vector3f up;
        }
        _camera;
        
        const foundation::PlatformInterfacePtr _platform;
        const foundation::RenderingInterfacePtr _rendering;
        
        foundation::RenderTexturePtr _palette;
        
        foundation::RenderShaderPtr _lineSetShader;
        foundation::RenderShaderPtr _boundingBoxShader;
        foundation::RenderShaderPtr _staticMeshShader;
        foundation::RenderShaderPtr _dynamicMeshShader;
        foundation::RenderShaderPtr _texturedMeshShader;
        foundation::RenderShaderPtr _particlesShader;
        
        foundation::RenderTargetPtr _gbuffer;
        foundation::RenderShaderPtr _gbufferToScreenShader;
        
        std::vector<std::shared_ptr<LineSetImpl>> _lineSets;
        std::vector<std::shared_ptr<BoundingBoxImpl>> _boundingBoxes;
        std::vector<std::shared_ptr<StaticMeshImpl>> _staticMeshes;
        std::vector<std::shared_ptr<DynamicMeshImpl>> _dynamicMeshes;
        std::vector<std::shared_ptr<TexturedMeshImpl>> _texturedMeshes;
        std::vector<std::shared_ptr<ParticlesImpl>> _particles;
    };
    
    std::shared_ptr<SceneInterface> SceneInterface::instance(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering) {
        return std::make_shared<SceneInterfaceImpl>(platform, rendering);
    }
}

// TODO: shader evolution
// + const {...} const {...} <- few 'const' blocks
// + myOpenArray[] : float4 <- open arrays in constant block
// + structsArray[] : struct { position: float4 color: float4 } <- arrays of structures in constant block
// + fssrc [tex2d tex3d cube] {...} <- types of samplers
// + _staticloop() {...}
//
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
            output_position = _transform(position, frame_plmVPMatrix);
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
            output_position = _transform(position, frame_plmVPMatrix);
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
            normal[6] : float3 =
                [0.5, 0.5, 0.0][0.0, 0.5, 0.5][0.5, 0.0, 0.5][0.5, 0.5, 1.0][1.0, 0.5, 0.5][0.5, 1.0, 0.5]
        }
        const {
            globalPosition : float4
        }
        inout {
            normalc : float4
        }
        vssrc {
            float3 cubeCenter = float3(vertex_position_color_mask.xyz) + const_globalPosition.xyz;
            float3 toCamSign = _sign(frame_cameraPosition.xyz - cubeCenter);
            
            uint faceIndex = uint(repeat_ID / 4) + uint((toCamSign.zxy[repeat_ID / 4] * 1.5 + 1.5));
            uint colorIndex = uint(vertex_position_color_mask.w & 0xff);
            uint mask = uint((vertex_position_color_mask.w >> (uint(8) + faceIndex)) & 1);
            
            float4 relVertexPos = float4(toCamSign, 1.0) * _lerp(float4(0.5, 0.5, 0.5, 1.0), fixed_cube[repeat_ID], float(mask));
            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos + _step(0.0, relVertexPos) * float4(float3(vertex_scale_reserved.xyz), 0.0);
            
            output_normalc = float4(fixed_normal[faceIndex], float(colorIndex) / 255.0); //
            output_position = _transform(absVertexPos, frame_plmVPMatrix);
        }
        fssrc {
            output_color[0] = input_normalc;
        }
    )";
    const char *g_dynamicMeshShaderSrc = R"(
        fixed {
            cube[12] : float4 =
                [-0.5, 0.5, 0.5, 1.0][-0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0][0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0]
            normal[6] : float4 =
                [0.0, 0.0, -1.0, 0.0][-1.0, 0.0, 0.0, 0.0][0.0, -1.0, 0.0, 0.0][0.0, 0.0, 1.0, 0.0][1.0, 0.0, 0.0, 0.0][0.0, 1.0, 0.0, 0.0]
        }
        const {
            modelTransform : matrix4
        }
        inout {
            normalc : float4
        }
        vssrc {
            float3 cubeCenter = float3(vertex_position.xyz);
            float3 worldCubePos = _transform(float4(cubeCenter, 1.0), const_modelTransform).xyz;
            float3 toCamSign = _sign(_transform(const_modelTransform, float4(frame_cameraPosition.xyz - worldCubePos, 0.0)).xyz);
            
            uint faceIndex = uint(repeat_ID / 4) + uint((toCamSign.zxy[repeat_ID / 4] * 1.5 + 1.5));
            uint colorIndex = vertex_color_mask.r;
            uint mask = (vertex_color_mask.g >> faceIndex) & uint(1);
            
            float4 relVertexPos = float4(toCamSign, 1.0) * _lerp(float4(0.5, 0.5, 0.5, 1.0), fixed_cube[repeat_ID], float(mask));
            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
            
            output_normalc = float4(_transform(fixed_normal[faceIndex], const_modelTransform).xyz * 0.5 + 0.5, float(colorIndex) / 255.0); //
            output_position = _transform(absVertexPos, _transform(const_modelTransform, frame_plmVPMatrix));
        }
        fssrc {
            output_color[0] = input_normalc;
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
            output_position = _transform(float4(vertex_position.xyz + const_globalPosition.xyz, 1.0), frame_plmVPMatrix);
            output_uv = vertex_uv;
            output_nrm = vertex_normal.xyz;
        }
        fssrc {
            float paletteIndex = _tex2d(0, input_uv).r; //
            output_color[0] = float4(input_nrm * 0.5 + 0.5, paletteIndex);
        }
    )";
    const char *g_particlesShaderSrc = R"(
        fixed {
            quad[4] : float2 = [-0.5, 0.5][0.5, 0.5][-0.5, -0.5][0.5, -0.5]
            uv[4] : float2 = [0, 0][1, 0][0, 1][1, 1]
        }
        const {
            position_t : float4
            normal_a : float4
            right : float4
        }
        inout {
            uv : float2
        }
        vssrc {
            float3 up = _cross(const_normal_a.xyz, const_right.xyz);
            float3 relVertexPos = const_right.xyz * fixed_quad[repeat_ID].x + up * fixed_quad[repeat_ID].y;
            output_position = float4(const_position_t.xyz + relVertexPos, 1.0);
            output_uv = fixed_uv[repeat_ID];
        }
        fssrc {
            output_color[0] = float4(1, 0, 0, 1);
        }
    )";
    const char *g_gbufferToScreenShaderSrc = R"(
        fixed {
            rndv[64] : float3 =
                [0.80,0.76,0.69][-0.67,0.62,-0.25][0.32,-0.06,-0.68][-0.07,-0.60,-0.03][0.16,-0.11,0.46][-0.33,-0.20,0.11][-0.18,0.25,0.17][-0.07,-0.22,0.20]
                [-0.89,-0.90,-0.28][-0.65,0.44,-0.54][0.42,-0.38,-0.50][0.39,0.32,-0.33][-0.09,0.48,-0.10][0.01,0.06,-0.39][-0.07,-0.21,-0.27][-0.30,0.03,0.01]
                [1.18,0.28,-0.46][-0.65,0.53,0.45][-0.26,-0.66,-0.25][0.07,-0.25,0.54][0.30,0.13,0.37][0.28,-0.21,0.18][-0.15,-0.27,0.17][0.04,-0.09,-0.28]
                [-0.78,-0.67,0.79][-0.85,-0.37,-0.21][0.07,0.34,0.67][0.20,-0.43,0.37][0.39,-0.11,-0.29][0.01,0.39,0.07][0.31,-0.16,0.06][0.19,-0.01,0.23]
                [-0.30,0.92,-0.87][-0.08,-0.10,0.94][0.40,0.46,-0.43][0.19,0.31,0.48][-0.12,0.46,0.14][-0.27,0.12,0.27][-0.29,0.18,-0.08][0.27,0.11,0.08]
                [1.12,0.43,0.50][-0.24,-0.56,-0.73][-0.60,0.35,-0.28][0.22,0.20,-0.52][0.38,-0.27,-0.19][-0.03,0.35,-0.19][-0.22,-0.26,0.07][-0.11,0.05,-0.27]
                [0.43,-1.16,-0.41][0.42,0.66,-0.54][0.35,-0.08,0.66][-0.44,0.02,0.41][0.46,0.20,0.01][0.36,-0.17,0.04][0.12,0.22,0.25][0.10,-0.24,0.15]
                [-0.45,-0.84,0.88][-0.64,-0.31,-0.63][-0.31,0.68,0.02][-0.28,0.35,-0.40][0.05,-0.01,0.50][0.14,-0.22,-0.30][0.11,0.32,0.10][-0.29,0.05,0.04]
            quad[4] : float4 = [-1, 1, 0.1, 1][1, 1, 0.1, 1][-1, -1, 0.1, 1][1, -1, 0.1, 1]
            uv[4] : float2 = [0, 0][1, 0][0, 1][1, 1]
        }
        inout {
            uv : float2
        }
        fndef getWPos(float2 uv, float depth) -> float3 {
            float4 clipSpasePos = float4(uv * float2(2.0, -2.0) - float2(1.0, -1.0), depth, 1);
            float4 wpos = _transform(clipSpasePos, frame_invVPMatrix);
            return wpos.xyz / wpos.w;
        }
        fndef getDPos(float3 wpos) -> float3 {
            float4 clipSpasePos = _transform(float4(wpos, 1.0), frame_stdVPMatrix);
            return float3(0.5, -0.5, 1.0) * clipSpasePos.xyz / clipSpasePos.w + float3(0.5, 0.5, 0.0);
        }
        vssrc {
            output_position = fixed_quad[repeat_ID];
            output_uv = fixed_uv[repeat_ID];
        }
        fssrc {
            float4 gbuffer = _tex2d(1, input_uv);
            float  gdepth = _tex2d(2, input_uv).r;
            float3 normal = _norm(float3(2.0, 2.0, 2.0) * gbuffer.rgb - float3(1.0, 1.0, 1.0));
            float3 color = _tex2d(0, float2(gbuffer.w, 0)).rgb;
            float3 wpos = getWPos(input_uv, gdepth);
            float3 wnpos = wpos + 0.1 * normal;
            float  lit = 0.0;
            float  rnd = _sin(_dot(wpos, float3(12.9898, 78.233, 37.719)));
            
            for (int i = 0; i < 12; i++) {
                rnd = 0.1 + _frac(rnd * 143758.5453);
                float3 rv = _norm(fixed_rndv[i]) * rnd;
                float3 dpos = getDPos(wnpos + _sign(_dot(normal, rv)) * rv);
                float  rdepth = _tex2d(2, dpos.xy).r;
                lit += _step(rdepth, dpos.z) + _clamp(20000.0 * (rdepth - dpos.z));
            }
            
            lit = _step(0.0000001, gdepth) * (lit * 0.0625 * 0.5 + 0.5);
            output_color[0] = float4(lit * color, 1);
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
        _staticMeshShader = rendering->createShader("scene_static_voxel_mesh", g_staticMeshShaderSrc, layouts::VTXSVOX);
        _dynamicMeshShader = rendering->createShader("scene_dynamic_voxel_mesh", g_dynamicMeshShaderSrc, layouts::VTXDVOX);
        _texturedMeshShader = rendering->createShader("scene_static_textured_mesh", g_texturedMeshShaderSrc, layouts::VTXNRMUV);
        _particlesShader = rendering->createShader("scene_particles", g_particlesShaderSrc, foundation::InputLayout {
            .repeat = 4
        });
        _gbufferToScreenShader = rendering->createShader("scene_gbuffer_to_screen", g_gbufferToScreenShaderSrc, foundation::InputLayout {
            .repeat = 4
        });
        
        // TODO: memory test for wasm-backend
        _gbuffer = _rendering->createRenderTarget(foundation::RenderTextureFormat::RGBA8UN, 1, _rendering->getBackBufferWidth(), _rendering->getBackBufferHeight(), true);
        _platform->setResizeHandler([&]() {
            _gbuffer = _rendering->createRenderTarget(foundation::RenderTextureFormat::RGBA8UN, 1, _rendering->getBackBufferWidth(), _rendering->getBackBufferHeight(), true);
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

        _camera.right = right.normalized();
        _camera.forward = nrmlook;
        _camera.up = _camera.right.cross(nrmlook).normalized();
        
        float aspect = _platform->getScreenWidth() / _platform->getScreenHeight();

        math::transform3f viewMatrix = math::transform3f::lookAtRH(_camera.position, _camera.target, _camera.up);
        
        _camera.plmVPMatrix = viewMatrix * math::transform3f::platformPerspectiveFovRH(50.0 / 180.0f * float(3.14159f), aspect, 0.1f, 10000.0f);
        _camera.stdVPMatrix = viewMatrix * math::transform3f::perspectiveFovRH(50.0 / 180.0f * float(3.14159f), aspect, 0.1f, 10000.0f);
        _camera.invVPMatrix = _camera.stdVPMatrix.inverted();
    }
    
    void SceneInterfaceImpl::setSun(const math::vector3f &directionToSun, const math::color &rgba) {
    
    }
    
    SceneInterface::LineSetPtr SceneInterfaceImpl::addLineSet(std::uint32_t count) {
        return _lineSets.emplace_back(std::make_shared<LineSetImpl>(count));
    }
    
    SceneInterface::BoundingBoxPtr SceneInterfaceImpl::addBoundingBox(const math::bound3f &bbox) {
        return _boundingBoxes.emplace_back(std::make_shared<BoundingBoxImpl>(bbox));
    }
    
    SceneInterface::StaticMeshPtr SceneInterfaceImpl::addStaticMesh(const foundation::RenderDataPtr &mesh) {
        std::shared_ptr<StaticMeshImpl> result = std::make_shared<StaticMeshImpl>(mesh);
        return _staticMeshes.emplace_back(result);
    }
    
    SceneInterface::DynamicMeshPtr SceneInterfaceImpl::addDynamicMesh(const std::vector<foundation::RenderDataPtr> &frames) {
        std::shared_ptr<DynamicMeshImpl> result = std::make_shared<DynamicMeshImpl>(frames.data(), std::uint32_t(frames.size()));
        return _dynamicMeshes.emplace_back(result);
    }
    
    SceneInterface::TexturedMeshPtr SceneInterfaceImpl::addTexturedMesh(const foundation::RenderDataPtr &mesh, const foundation::RenderTexturePtr &texture) {
        std::shared_ptr<TexturedMeshImpl> result = std::make_shared<TexturedMeshImpl>(mesh, texture);
        return _texturedMeshes.emplace_back(result);
    }
    
    SceneInterface::ParticlesPtr SceneInterfaceImpl::addParticles(const foundation::RenderTexturePtr &tx, const foundation::RenderTexturePtr &map, const EmitterParams &params) {
        std::shared_ptr<ParticlesImpl> result = std::make_shared<ParticlesImpl>(tx, map, params);
        return _particles.emplace_back(result);
    }
    
    SceneInterface::LightSourcePtr SceneInterfaceImpl::addLightSource(float r, float g, float b, float radius) {
        return nullptr;
    }
    
    math::vector2f SceneInterfaceImpl::getScreenCoordinates(const math::vector3f &worldPosition) {
        math::vector4f tpos = math::vector4f(worldPosition, 1.0f);
        
        tpos = tpos.transformed(_camera.stdVPMatrix);
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
        float relScreenPosX = 2.0f * screenPosition.x / _platform->getScreenWidth() - 1.0f;
        float relScreenPosY = 1.0f - 2.0f * screenPosition.y / _platform->getScreenHeight();
        
        const math::vector4f worldPos = math::vector4f(relScreenPosX, relScreenPosY, 0.0f, 1.0f).transformed(_camera.invVPMatrix);
        return math::vector3f(worldPos.x / worldPos.w, worldPos.y / worldPos.w, worldPos.z / worldPos.w).normalized();
    }
    
    // Next:
    // + Particles
    // + Skybox + texture types
    //
    //
    void SceneInterfaceImpl::updateAndDraw(float dtSec) {
        _cleanupUnused(_boundingBoxes);
        _cleanupUnused(_staticMeshes);
        _cleanupUnused(_dynamicMeshes);
        _cleanupUnused(_texturedMeshes);
        _cleanupUnused(_particles);
        _rendering->updateFrameConstants(_camera.plmVPMatrix, _camera.stdVPMatrix, _camera.invVPMatrix, _camera.position, _camera.forward);
        
        _rendering->forTarget(_gbuffer, nullptr, math::color{0.0, 0.0, 0.0, 1.0}, [&](foundation::RenderingInterface &rendering) {
            rendering.applyShader(_staticMeshShader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::DISABLED, foundation::DepthBehavior::TEST_AND_WRITE);
            for (const auto &staticMesh : _staticMeshes) {
                rendering.applyShaderConstants(&staticMesh->position);
                rendering.draw(staticMesh->mesh);
            }
            
            rendering.applyShader(_dynamicMeshShader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::DISABLED, foundation::DepthBehavior::TEST_AND_WRITE);
            for (const auto &dynamicMesh : _dynamicMeshes) {
                rendering.applyShaderConstants(&dynamicMesh->transform);
                rendering.draw(dynamicMesh->frames[0]);
            }
            
            rendering.applyShader(_texturedMeshShader, foundation::RenderTopology::TRIANGLES, foundation::BlendType::DISABLED, foundation::DepthBehavior::TEST_AND_WRITE);
            for (const auto &texturedMesh : _texturedMeshes) {
                rendering.applyShaderConstants(&texturedMesh->position);
                _rendering->applyTextures({
                    {texturedMesh->texture, foundation::SamplerType::NEAREST}
                });
                rendering.draw(texturedMesh->data);
            }
        });
        _rendering->forTarget(nullptr, nullptr, math::color{0.0, 0.0, 0.0, 0.0}, [&](foundation::RenderingInterface &rendering) {
            rendering.applyShader(_gbufferToScreenShader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
            rendering.applyTextures({
                {_palette, foundation::SamplerType::NEAREST},
                {_gbuffer->getTexture(0), foundation::SamplerType::NEAREST},
                {_gbuffer->getDepth(), foundation::SamplerType::NEAREST},
            });
            rendering.draw();
        });
        _rendering->forTarget(nullptr, _gbuffer->getDepth(), std::nullopt, [&](foundation::RenderingInterface &rendering) {
            rendering.applyShader(_lineSetShader, foundation::RenderTopology::LINES, foundation::BlendType::MIXING, foundation::DepthBehavior::TEST_ONLY);
            for (const auto &set : _lineSets) {
                for (std::size_t i = 0; i < set->blocks.size(); i++) {
                    rendering.applyShaderConstants(&set->blocks[i]);
                    rendering.draw(set->blocks[i].lineCount * 2);
                }
            }
            rendering.applyShader(_boundingBoxShader, foundation::RenderTopology::LINES, foundation::BlendType::MIXING, foundation::DepthBehavior::TEST_ONLY);
            for (const auto &bbox : _boundingBoxes) {
                rendering.applyShaderConstants(&bbox->bboxData);
                rendering.draw();
            }
        });
    }
}

// + Ground color influence effect
// + Cubemap shadows
//

