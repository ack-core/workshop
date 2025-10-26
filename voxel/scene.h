
#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/layouts.h"
#include "foundation/math.h"
#include "foundation/util.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace voxel {
    const std::uint32_t VERTICAL_PIXELS_PER_PARTICLE = 3;
    
    struct ParticlesParams {
        ParticlesParams() = default;
        ParticlesParams(const util::Description &emitterDesc);
        
        enum class ParticlesOrientation {
            CAMERA = 1, AXIS, WORLD
        };
        bool additiveBlend = false;
        float bakingTimeSec = 0.0f;
        ParticlesOrientation orientation = ParticlesOrientation::CAMERA;
        math::vector3f minXYZ = {0, 0, 0};
        math::vector3f maxXYZ = {0, 0, 0};
        math::vector2f maxSize = {0, 0};
    };

    class SceneInterface {
    public:
        static std::shared_ptr<SceneInterface> instance(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering
        );
        
    public:
        struct LineSet {
            virtual void setPosition(const math::vector3f &position) = 0;
            virtual void setLine(std::uint32_t index, const math::vector3f &start, const math::vector3f &end, const math::color &rgba, bool isArrow = false) = 0;
            virtual void capLineCount(std::uint32_t limit) = 0;
            virtual ~LineSet() = default;
        };
        struct BoundingBox {
            virtual void setBBoxData(const math::bound3f &bbox) = 0;
            virtual void setColor(const math::color &rgba) = 0;
            virtual ~BoundingBox() = default;
        };
        struct VoxelMesh {
            virtual void resetOffset() = 0;
            virtual auto getCenterOffset() const -> util::IntegerOffset3D = 0;
            virtual void setCenterOffset(const util::IntegerOffset3D& offset) = 0;
            virtual void setPosition(const math::vector3f &position) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setFrame(std::uint32_t index) = 0;
            virtual ~VoxelMesh() = default;
        };
        struct TexturedMesh {
            virtual void setPosition(const math::vector3f &position) = 0;
            virtual ~TexturedMesh() = default;
        };
        struct Particles {
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setTime(float totalTimeSec, float fadingTimeSec) = 0;
            virtual ~Particles() = default;
        };
        struct LightSource {
            virtual void setPosition(const math::vector3f &position) = 0;
            virtual ~LightSource() = default;
        };
        
        using LineSetPtr = std::shared_ptr<LineSet>;
        using BoundingBoxPtr = std::shared_ptr<BoundingBox>;
        using VoxelMeshPtr = std::shared_ptr<VoxelMesh>;
        using TexturedMeshPtr = std::shared_ptr<TexturedMesh>;
        using LightSourcePtr = std::shared_ptr<LightSource>;
        using ParticlesPtr = std::shared_ptr<Particles>;
        
    public:
        virtual void setCameraLookAt(const math::vector3f &position, const math::vector3f &sceneCenter) = 0;
        virtual void setSun(const math::vector3f &directionToSun, const math::color &rgba) = 0;
        
        virtual auto addLineSet() -> LineSetPtr = 0;
        virtual auto addBoundingBox(const math::bound3f &bbox) -> BoundingBoxPtr = 0;
        virtual auto addVoxelMesh(const std::vector<foundation::RenderDataPtr> &frames, const util::IntegerOffset3D &originOffset) -> VoxelMeshPtr = 0;
        virtual auto addTexturedMesh(const foundation::RenderDataPtr &mesh, const foundation::RenderTexturePtr &texture) -> TexturedMeshPtr = 0;
        virtual auto addParticles(const foundation::RenderTexturePtr &tx, const foundation::RenderTexturePtr &map, const ParticlesParams &params) -> ParticlesPtr = 0;
        virtual auto addLightSource(float r, float g, float b, float radius) -> LightSourcePtr = 0;
        
        virtual auto getCameraPosition() const -> math::vector3f = 0;
        virtual auto getScreenCoordinates(const math::vector3f &worldPosition) const -> math::vector2f = 0;
        virtual auto getWorldDirection(const math::vector2f &screenPosition, math::vector3f *outCamPosition = nullptr) const -> math::vector3f = 0;
        
        virtual void updateAndDraw(float dtSec) = 0;
        
    public:
        virtual ~SceneInterface() = default;
    };
    
    using SceneInterfacePtr = std::shared_ptr<SceneInterface>;
}

