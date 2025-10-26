
#include "scene.h"
#include "palette.h"
#include "foundation/layouts.h"

#include <cfloat>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <list>

namespace voxel {
    ParticlesParams::ParticlesParams(const util::Description &emitterDesc) {
        const util::Description &desc = *emitterDesc.getSubDesc("emitter");
        additiveBlend = *desc.get<bool>("additiveBlend");
        orientation = *desc.get<voxel::ParticlesParams::ParticlesOrientation>("particleOrientation");
        bakingTimeSec = float(*desc.get<std::uint32_t>("bakingFrameTimeMs")) / 1000.0f;
        minXYZ = *desc.get<math::vector3f>("minXYZ");
        maxXYZ = *desc.get<math::vector3f>("maxXYZ");
        maxSize = *desc.get<math::vector2f>("maxSize");
    }
}

namespace voxel {
    class LineSetImpl : public SceneInterface::LineSet {
    public:
        struct Line {
            math::color rgba;
            math::vector3f start;
            math::vector3f end;
            bool arrowHead = false;
        };

        std::vector<Line> lines;
        math::vector3f position;
        
        struct {
            math::color rgba;
            math::vector4f binormal;
            math::vector4f tangent;
            math::vector4f positions[2];
        }
        shaderConstants;
        
    public:
        void setPosition(const math::vector3f &pos) override {
            position = pos;
        }
        void setLine(std::uint32_t index, const math::vector3f &start, const math::vector3f &end, const math::color &rgba, bool isArrow) override {
            if (index >= lines.size()) {
                lines.resize(index + 1);
            }
            
            lines[index].start = start;
            lines[index].end = end;
            lines[index].rgba = rgba;
            lines[index].arrowHead = isArrow;
        }
        void capLineCount(std::uint32_t limit) override {
            lines.resize(limit);
        }
        void fillShaderConstants(const Line &line) {
            shaderConstants.rgba = line.rgba;
            shaderConstants.positions[0].xyz = line.start + position;
            shaderConstants.positions[1].xyz = line.end + position;
            
            const math::vector3f lineDir = (line.end - line.start).normalized();
            const math::vector3f axis = lineDir.dot({0, 1, 0}) > 0.9f ? math::vector3f(0, 0, 1) : math::vector3f(0, 1, 0);

            shaderConstants.binormal.xyz = lineDir.cross(axis).normalized();
            shaderConstants.tangent.xyz = shaderConstants.binormal.xyz.cross(lineDir);
        }
        
    public:
        LineSetImpl() {}
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
    
    class VoxelMeshImpl : public SceneInterface::VoxelMesh {
    public:
        std::unique_ptr<foundation::RenderDataPtr[]> frames;
        std::uint32_t frameIndex;
        std::uint32_t frameCount;
        math::transform3f transform;
        
        // for editor
        util::IntegerOffset3D originVoxelOffset;
        util::IntegerOffset3D currentVoxelOffset;
        
    public:
        VoxelMeshImpl(const foundation::RenderDataPtr *frameArray, std::uint32_t count, util::IntegerOffset3D voxelOffset) : transform(math::transform3f::identity()) {
            frameIndex = 0;
            frameCount = count;
            frames = std::make_unique<foundation::RenderDataPtr[]>(count);
            originVoxelOffset = voxelOffset;
            currentVoxelOffset = {0, 0, 0};

            for (std::uint32_t i = 0; i < count; i++) {
                frames[i] = std::move(frameArray[i]);
            }
        }
        ~VoxelMeshImpl() override {}
        
        void resetOffset() override {
            currentVoxelOffset = {0, 0, 0};
        }
        auto getCenterOffset() const -> util::IntegerOffset3D override {
            return {
                originVoxelOffset.x + currentVoxelOffset.x,
                originVoxelOffset.y + currentVoxelOffset.y,
                originVoxelOffset.z + currentVoxelOffset.z
            };
        }
        void setCenterOffset(const util::IntegerOffset3D& offset) override {
            currentVoxelOffset.x = offset.x - originVoxelOffset.x;
            currentVoxelOffset.y = offset.y - originVoxelOffset.y;
            currentVoxelOffset.z = offset.z - originVoxelOffset.z;
        }
        void setPosition(const math::vector3f &position) override {
            transform = math::transform3f::identity().translated(position);
        }
        void setTransform(const math::transform3f &trfm) override {
            transform = trfm;
        }
        void setFrame(std::uint32_t index) override {
            frameIndex = std::min(index, frameCount - 1);
        }
        auto getFinalTransform() const -> math::transform3f {
            math::vector3f offset;
            offset.x = -float(currentVoxelOffset.x);
            offset.y = -float(currentVoxelOffset.y);
            offset.z = -float(currentVoxelOffset.z);
            return math::transform3f::identity().translated(offset) * transform;
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
    
    class ParticleEmitterImpl : public SceneInterface::Particles {
        static constexpr float TYPE_MASK_NEWBORN = 1.0f;
        static constexpr float TYPE_MASK_OLD = 2.0f;
        static constexpr float TYPE_MASK_ALL = 3.0f;

    public:
        bool additiveBlend;
        std::uint32_t particleCount;
        foundation::RenderTexturePtr texture;
        foundation::RenderTexturePtr map;
                
        ParticleEmitterImpl(const foundation::RenderTexturePtr &texture, const foundation::RenderTexturePtr &map, const ParticlesParams &particlesParams)
        : additiveBlend(particlesParams.additiveBlend)
        , particleCount(map->getHeight() / VERTICAL_PIXELS_PER_PARTICLE)
        , texture(texture)
        , map(map)
        , _secondsPerTextureWidth(particlesParams.bakingTimeSec * float(map->getWidth()))
        , _bakingTimeSec(particlesParams.bakingTimeSec)
        {
            if (particlesParams.orientation == ParticlesParams::ParticlesOrientation::AXIS) {
                _updateOrientation = [](ParticleEmitterImpl &self, const math::vector3f &camDir, const math::vector3f &camRight) {
                    self._constants.position = math::vector3f(self._transform.m41, self._transform.m42, self._transform.m43);
                    self._constants.right = camRight;
                    self._constants.normal = camRight.cross(math::vector3f(self._transform.m21, self._transform.m22, self._transform.m23));
                };
            }
            else if (particlesParams.orientation == ParticlesParams::ParticlesOrientation::WORLD) {
                _updateOrientation = [](ParticleEmitterImpl &self, const math::vector3f &camDir, const math::vector3f &camRight) {
                    self._constants.position = math::vector3f(self._transform.m41, self._transform.m42, self._transform.m43);
                    self._constants.right = math::vector3f(self._transform.m11, self._transform.m12, self._transform.m13);
                    self._constants.normal = math::vector3f(self._transform.m21, self._transform.m22, self._transform.m23);
                };
            }
            else {
                _updateOrientation = [](ParticleEmitterImpl &self, const math::vector3f &camDir, const math::vector3f &camRight) {
                    self._constants.position = math::vector3f(self._transform.m41, self._transform.m42, self._transform.m43);
                    self._constants.right = camRight;
                    self._constants.normal = -1.0 * camDir;
                };
            }
            
            _constants.alpha = 1.0;
            _constants.t0_t1_mask_cap = math::vector4f {0, 0, TYPE_MASK_NEWBORN, 1.0f};
            _constants.hpix = 1.0f / float(map->getWidth());
            _constants.vpix = 1.0f / float(map->getHeight());
            _constants.minXYZ = particlesParams.minXYZ;
            _constants.maxXYZ = particlesParams.maxXYZ;
            _constants.maxW = particlesParams.maxSize.x;
            _constants.maxH = particlesParams.maxSize.y;
        }
        ~ParticleEmitterImpl() override {}
        
        void *getUpdatedConstants(const math::vector3f &camDir, const math::vector3f &camRight) {
            _updateOrientation(*this, camDir, camRight);
            return &_constants;
        }
        void setTransform(const math::transform3f &trfm) override {
            _transform = trfm;
        }
        void setTime(float totalTimeSec, float fadingTimeSec) override {
            float koeff = std::max(totalTimeSec, 0.0f) / _secondsPerTextureWidth;
            if (koeff >= 1.0f) {
                koeff = koeff - float(int(koeff));
                _constants.t0_t1_mask_cap.z = TYPE_MASK_ALL;
            }
            else {
                _constants.t0_t1_mask_cap.z = TYPE_MASK_NEWBORN;
            }
            
            float aligned = std::floor(koeff / _constants.hpix) * _constants.hpix;
            
            _constants.t0_t1_mask_cap.x = koeff;
            _constants.t0_t1_mask_cap.y = aligned + _constants.hpix > 1.0f - std::numeric_limits<float>::epsilon() ? 0.0f : aligned + _constants.hpix;
            _constants.t0_t1_mask_cap.w = fadingTimeSec / _bakingTimeSec;
        }
        
    private:
        math::transform3f _transform = math::transform3f::identity();
        const float _secondsPerTextureWidth;
        const float _bakingTimeSec;

        struct Constants {
            math::vector4f t0_t1_mask_cap;
            math::vector3f position; float alpha;
            math::vector3f normal; float maxW;
            math::vector3f right; float maxH;
            math::vector3f minXYZ; float hpix;
            math::vector3f maxXYZ; float vpix;
        }
        _constants;
        
        void (*_updateOrientation)(ParticleEmitterImpl &self, const math::vector3f &camRight, const math::vector3f &camUp);
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
        
        auto addLineSet() -> LineSetPtr override;
        auto addBoundingBox(const math::bound3f &bbox) -> BoundingBoxPtr override;
        auto addVoxelMesh(const std::vector<foundation::RenderDataPtr> &frames, const util::IntegerOffset3D &originOffset) -> VoxelMeshPtr override;
        auto addTexturedMesh(const foundation::RenderDataPtr &mesh, const foundation::RenderTexturePtr &texture) -> TexturedMeshPtr override;
        auto addParticles(const foundation::RenderTexturePtr &tx, const foundation::RenderTexturePtr &map, const ParticlesParams &params) -> ParticlesPtr override;
        auto addLightSource(float r, float g, float b, float radius) -> LightSourcePtr override;
        auto getCameraPosition() const -> math::vector3f override;
        auto getScreenCoordinates(const math::vector3f &worldPosition) const -> math::vector2f override;
        auto getWorldDirection(const math::vector2f &screenPosition, math::vector3f *outCamPosition) const -> math::vector3f override;
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
        
        foundation::RenderShaderPtr _lineShader;
        foundation::RenderShaderPtr _boundingBoxShader;
        foundation::RenderShaderPtr _voxelMeshShader;
        foundation::RenderShaderPtr _texturedMeshShader;
        foundation::RenderShaderPtr _particlesShader;
        
        foundation::RenderTargetPtr _gbuffer;
        foundation::RenderShaderPtr _gbufferToScreenShader;
        
        std::vector<std::shared_ptr<LineSetImpl>> _lineSets;
        std::vector<std::shared_ptr<BoundingBoxImpl>> _boundingBoxes;
        std::vector<std::shared_ptr<VoxelMeshImpl>> _voxelMeshes;
        std::vector<std::shared_ptr<TexturedMeshImpl>> _texturedMeshes;
        std::vector<std::shared_ptr<ParticleEmitterImpl>> _particleEmitters;
    };
    
    std::shared_ptr<SceneInterface> SceneInterface::instance(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering) {
        return std::make_shared<SceneInterfaceImpl>(platform, rendering);
    }
}

// TODO: shader evolution
// + const {...} const {...} <- few 'const' blocks
// + myOpenArray[] : float4 <- open arrays in constant block
// + fssrc [tex2d tex3d cube] {...} <- types of samplers
//
namespace {
    const char *g_lineShaderSrc = R"(
        fixed {
            offset[34] : float3 =
                [0.00, 0.00, 0][0.00, 0.00, 0]
                [0.00, 0.00, 0][0.00, 1.00, 0.5][0.00, 1.00, 0.5][0.71, 0.71, 0.5]
                [0.00, 0.00, 0][0.71, 0.71, 0.5][0.71, 0.71, 0.5][1.00, 0.00, 0.5]
                [0.00, 0.00, 0][1.00, 0.00, 0.5][1.00, 0.00, 0.5][0.71, -0.71, 0.5]
                [0.00, 0.00, 0][0.71, -0.71, 0.5][0.71, -0.71, 0.5][0.00, -1.00, 0.5]
                [0.00, 0.00, 0][0.00, -1.00, 0.5][0.00, -1.00, 0.5][-0.71, -0.71, 0.5]
                [0.00, 0.00, 0][-0.71, -0.71, 0.5][-0.71, -0.71, 0.5][-1.00, 0.00, 0.5]
                [0.00, 0.00, 0][-1.00, 0.00, 0.5][-1.00, 0.00, 0.5][-0.71, 0.71, 0.5]
                [0.00, 0.00, 0][-0.71, 0.71, 0.5][-0.71, 0.71, 0.5][0.00, 1.00, 0.5]
        }
        const {
            color : float4
            binormal : float4
            tangent : float4
            positions[2] : float4
        }
        inout {
            color : float4
        }
        vssrc {
            float3 inrm = _cross(const_binormal.xyz, const_tangent.xyz) * fixed_offset[vertex_ID].z;
            float  headScale = 0.25 * _len(const_positions[1].xyz - const_positions[0].xyz);
            float3 headDir = 0.15 * (fixed_offset[vertex_ID].x * const_binormal.xyz + fixed_offset[vertex_ID].y * const_tangent.xyz) + inrm;
            float3 position = const_positions[_min(vertex_ID, 1)].xyz + headScale * headDir;
            
            output_position = _transform(float4(position, 1.0), frame_plmVPMatrix);
            output_color = const_color;
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
    const char *g_voxelMeshShaderSrc = R"(
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
            float3 cubeCenter = float3(vertex_position_color_mask.xyz);
            float3 worldCubePos = _transform(float4(cubeCenter, 1.0), const_modelTransform).xyz;
            float3 toCamSign = _sign(_transform(const_modelTransform, float4(frame_cameraPosition.xyz - worldCubePos, 0.0)).xyz);
            
            uint faceIndex = uint(repeat_ID / 4) + uint((toCamSign.zxy[repeat_ID / 4] * 1.5 + 1.5));
            uint colorIndex = uint(vertex_position_color_mask.w & 0xff);
            uint mask = uint((vertex_position_color_mask.w >> (uint(8) + faceIndex)) & 1);
            
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
            quad[4] : float4 = [-0.5, 0.5, 0, 0][0.5, 0.5, 1, 0][-0.5, -0.5, 0, 1][0.5, -0.5, 1, 1]
        }
        const {
            t0_t1_mask_cap : float4
            position_alpha : float4
            normal_msw : float4
            right_msh : float4
            minXYZ_hpix : float4
            maxXYZ_vpix : float4
        }
        inout {
            uv : float2
        }
        vssrc {
            float  ptcv0 = 3.0 * const_maxXYZ_vpix.w * float(vertex_ID) + 0.25 * const_maxXYZ_vpix.w;
            float  ptcv1 = ptcv0 + const_maxXYZ_vpix.w;
            float  ptcv2 = ptcv1 + const_maxXYZ_vpix.w;

            float  t0 = _floor(const_t0_t1_mask_cap.x / const_minXYZ_hpix.w) * const_minXYZ_hpix.w;
            float  t1 = const_t0_t1_mask_cap.y;
            float  tf = (const_t0_t1_mask_cap.x - t0) / const_minXYZ_hpix.w;
            
            float4 map0 = _lerp(_tex2d(0, float2(t0, ptcv0)), _tex2d(0, float2(t1, ptcv0)), tf);    // x, y
            float4 map1 = _lerp(_tex2d(0, float2(t0, ptcv1)), _tex2d(0, float2(t1, ptcv1)), tf);    // z, angle

            float4 m2t0 = _tex2d(0, float2(t0, ptcv2));
            float4 m2t1 = _tex2d(0, float2(t1, ptcv2));
            float3 map2 = _lerp(m2t0.xyz, m2t1.xyz, tf);    // w, h, old

            float  fading = float(255.0f * map2.z > const_t0_t1_mask_cap.w);
            float  isAlive = _step(0.5, float(uint(m2t0.w * 255.0f) & uint(const_t0_t1_mask_cap.z))) * float(m2t1.w < 0.5) * fading;

            float3 posKoeff = float3(map0.x + map0.y / 255.0f, map0.z + map0.w / 255.0f, map1.x + map1.y / 255.0f);
            float3 ptcpos = const_minXYZ_hpix.xyz + (const_maxXYZ_vpix.xyz - const_minXYZ_hpix.xyz) * posKoeff;

            float2 wh = float2(const_normal_msw.w, const_right_msh.w) * map2.xy * isAlive;
            float3 up = _cross(const_normal_msw.xyz, const_right_msh.xyz);
            float3 relVertexPos = const_right_msh.xyz * fixed_quad[repeat_ID].x * wh.x + up * fixed_quad[repeat_ID].y * wh.y;
            output_position = _transform(float4(const_position_alpha.xyz + ptcpos + relVertexPos, 1.0), frame_plmVPMatrix);
            output_uv = fixed_quad[repeat_ID].zw;
        }
        fssrc {
            output_color[0] = _tex2d(1, input_uv);
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
            quad[4] : float4 = [-1, 1, 0, 0][1, 1, 1, 0][-1, -1, 0, 1][1, -1, 1, 1]
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
            output_position = float4(fixed_quad[repeat_ID].xy, 0.1, 1.0);
            output_uv = fixed_quad[repeat_ID].zw;
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
        _lineShader = rendering->createShader("scene_static_lineset", g_lineShaderSrc, foundation::InputLayout {});
        _boundingBoxShader = rendering->createShader("scene_static_bounding_box", g_boundingBoxShaderSrc, foundation::InputLayout {
            .repeat = 24
        });
        _voxelMeshShader = rendering->createShader("scene_dynamic_voxel_mesh", g_voxelMeshShaderSrc, layouts::VTXMVOX);
        _texturedMeshShader = rendering->createShader("scene_static_textured_mesh", g_texturedMeshShaderSrc, layouts::VTXNRMUV);
        _particlesShader = rendering->createShader("scene_particles", g_particlesShaderSrc, foundation::InputLayout {
            .repeat = 4
        });
        _gbufferToScreenShader = rendering->createShader("scene_gbuffer_to_screen", g_gbufferToScreenShaderSrc, foundation::InputLayout {
            .repeat = 4
        });
        
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
    
    SceneInterface::LineSetPtr SceneInterfaceImpl::addLineSet() {
        return _lineSets.emplace_back(std::make_shared<LineSetImpl>());
    }
    
    SceneInterface::BoundingBoxPtr SceneInterfaceImpl::addBoundingBox(const math::bound3f &bbox) {
        return _boundingBoxes.emplace_back(std::make_shared<BoundingBoxImpl>(bbox));
    }
    
    SceneInterface::VoxelMeshPtr SceneInterfaceImpl::addVoxelMesh(const std::vector<foundation::RenderDataPtr> &frames, const util::IntegerOffset3D &originOffset) {
        std::shared_ptr<VoxelMeshImpl> result = std::make_shared<VoxelMeshImpl>(frames.data(), std::uint32_t(frames.size()), originOffset);
        return _voxelMeshes.emplace_back(result);
    }
    
    SceneInterface::TexturedMeshPtr SceneInterfaceImpl::addTexturedMesh(const foundation::RenderDataPtr &mesh, const foundation::RenderTexturePtr &texture) {
        std::shared_ptr<TexturedMeshImpl> result = std::make_shared<TexturedMeshImpl>(mesh, texture);
        return _texturedMeshes.emplace_back(result);
    }
    
    SceneInterface::ParticlesPtr SceneInterfaceImpl::addParticles(const foundation::RenderTexturePtr &tx, const foundation::RenderTexturePtr &map, const ParticlesParams &params) {
        std::shared_ptr<ParticleEmitterImpl> result = std::make_shared<ParticleEmitterImpl>(tx, map, params);
        return _particleEmitters.emplace_back(result);
    }
    
    SceneInterface::LightSourcePtr SceneInterfaceImpl::addLightSource(float r, float g, float b, float radius) {
        return nullptr;
    }
    
    math::vector3f SceneInterfaceImpl::getCameraPosition() const {
        return _camera.position;
    }
    
    math::vector2f SceneInterfaceImpl::getScreenCoordinates(const math::vector3f &worldPosition) const {
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
    
    math::vector3f SceneInterfaceImpl::getWorldDirection(const math::vector2f &screenPosition, math::vector3f *outCamPosition) const {
        if (outCamPosition) {
            *outCamPosition = _camera.position;
        }
        
        float relScreenPosX = 2.0f * screenPosition.x / _platform->getScreenWidth() - 1.0f;
        float relScreenPosY = 1.0f - 2.0f * screenPosition.y / _platform->getScreenHeight();
        
        const math::vector4f worldPos = math::vector4f(relScreenPosX, relScreenPosY, 0.0f, 1.0f).transformed(_camera.invVPMatrix);
        return (math::vector3f(worldPos.x / worldPos.w, worldPos.y / worldPos.w, worldPos.z / worldPos.w) - _camera.position).normalized();
    }
    
    // Next:
    // + Particles
    // + Skybox + texture types
    //
    //
    void SceneInterfaceImpl::updateAndDraw(float dtSec) {
        _cleanupUnused(_lineSets);
        _cleanupUnused(_boundingBoxes);
        _cleanupUnused(_voxelMeshes);
        _cleanupUnused(_texturedMeshes);
        _cleanupUnused(_particleEmitters);
        _rendering->updateFrameConstants(_camera.plmVPMatrix, _camera.stdVPMatrix, _camera.invVPMatrix, _camera.position, _camera.forward);
        
        _rendering->forTarget(_gbuffer, nullptr, math::color{0.0, 0.0, 0.0, 1.0}, [&](foundation::RenderingInterface &rendering) {
            rendering.applyShader(_voxelMeshShader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::DISABLED, foundation::DepthBehavior::TEST_AND_WRITE);
            for (const auto &voxelMesh : _voxelMeshes) {
                const math::transform3f transform = voxelMesh->getFinalTransform();
                rendering.applyShaderConstants(&transform);
                rendering.draw(voxelMesh->frames[voxelMesh->frameIndex]);
            }
            
            rendering.applyShader(_texturedMeshShader, foundation::RenderTopology::TRIANGLES, foundation::BlendType::DISABLED, foundation::DepthBehavior::TEST_AND_WRITE);
            for (const auto &texturedMesh : _texturedMeshes) {
                rendering.applyShaderConstants(&texturedMesh->position);
                rendering.applyTextures({
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
            rendering.applyShader(_particlesShader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::MIXING, foundation::DepthBehavior::TEST_ONLY);
            for (const auto &emitter : _particleEmitters) {
                rendering.applyShaderConstants(emitter->getUpdatedConstants(_camera.forward, _camera.right));
                rendering.applyTextures({
                    {emitter->map, foundation::SamplerType::NEAREST},
                    {emitter->texture, foundation::SamplerType::NEAREST}
                });
                rendering.draw(emitter->particleCount);
            }
            rendering.applyShader(_lineShader, foundation::RenderTopology::LINES, foundation::BlendType::MIXING, foundation::DepthBehavior::DISABLED);
            for (const auto &set : _lineSets) {
                for (const auto &line : set->lines) {
                    set->fillShaderConstants(line);
                    rendering.applyShaderConstants(&set->shaderConstants);
                    rendering.draw(2 + (line.arrowHead ? 32 : 0));
                }
            }
            rendering.applyShader(_boundingBoxShader, foundation::RenderTopology::LINES, foundation::BlendType::MIXING, foundation::DepthBehavior::DISABLED);
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

