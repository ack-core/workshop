
#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/layouts.h"
#include "foundation/math.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace core {
    const std::uint32_t VERTICAL_PIXELS_PER_PARTICLE = 4;

    class SceneInterface {
    public:
        static std::shared_ptr<SceneInterface> instance(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering
        );
        
    public:
        struct Arrows {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setPosition(const math::vector3f &pos) = 0;
            virtual void setArrow(std::uint32_t index, const math::vector3f &start, const math::vector3f &end, const math::color &rgba) = 0;
            virtual void clear() = 0;
            virtual ~Arrows() = default;
        };
        struct LineSet {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setPosition(const math::vector3f &pos) = 0;
            virtual void setLine(std::uint32_t index, const math::vector3f &start, const math::vector3f &end, const math::color &rgba) = 0;
            virtual void fillAsCircle(std::uint32_t segCount, float radius, const math::color &rgba) = 0;
            virtual void fillAsСlosedPolygon(const std::vector<math::vector3f> &points, const math::color &rgba) = 0;
            virtual void clear() = 0;
            virtual ~LineSet() = default;
        };
        struct BoundingSphere {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setPosition(const math::vector3f &pos) = 0;
            virtual void setRadius(float radius) = 0;
            virtual void setColor(const math::color &rgba) = 0;
            virtual ~BoundingSphere() = default;
        };
        struct BoundingBox {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setPosition(const math::vector3f &pos) = 0;
            virtual void setBBox(const math::bound3f &bbox) = 0;
            virtual void setColor(const math::color &rgba) = 0;
            virtual ~BoundingBox() = default;
        };
        struct VoxelMesh {
            virtual void resetOffset() = 0;
            virtual auto getCenterOffset() const -> math::vector3f = 0;
            virtual void setCenterOffset(const math::vector3f& offset) = 0;
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setPosition(const math::vector3f &pos) = 0;
            virtual void setFrame(std::uint32_t index) = 0;
            virtual auto getFrameCount() const -> std::uint32_t = 0;
            virtual auto getDescription() const -> const util::Description & = 0;
            virtual ~VoxelMesh() = default;
        };
        struct GroundMesh {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setPosition(const math::vector3f &pos) = 0;
            virtual ~GroundMesh() = default;
        };
        struct Vegetation {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual ~Vegetation() = default;
        };
        struct Particles {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setTime(float totalTimeSec, float fadingTimeSec) = 0;
            virtual ~Particles() = default;
        };
        struct LightSource {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setPosition(const math::vector3f &position) = 0;
            virtual ~LightSource() = default;
        };
        
        using ArrowsPtr = std::shared_ptr<Arrows>;
        using LineSetPtr = std::shared_ptr<LineSet>;
        using BoundingSpherePtr = std::shared_ptr<BoundingSphere>;
        using BoundingBoxPtr = std::shared_ptr<BoundingBox>;
        using VoxelMeshPtr = std::shared_ptr<VoxelMesh>;
        using GroundMeshPtr = std::shared_ptr<GroundMesh>;
        using LightSourcePtr = std::shared_ptr<LightSource>;
        using VegetationPtr = std::shared_ptr<Vegetation>;
        using ParticlesPtr = std::shared_ptr<Particles>;
        
    public:
        virtual void setCameraLookAt(const math::vector3f &position, const math::vector3f &sceneCenter, const math::vector3f &shift = {0, 0, 0}) = 0;
        virtual void setSun(const math::vector3f &directionToSun, const math::color &rgba) = 0;
        
        virtual auto addArrows() -> ArrowsPtr = 0;
        virtual auto addLineSet() -> LineSetPtr = 0;
        virtual auto addBoundingSphere(const math::vector3f &position, float radius, const math::color &rgba) -> BoundingSpherePtr = 0;
        virtual auto addBoundingBox(const math::vector3f &position, const math::bound3f &bbox, const math::color &rgba) -> BoundingBoxPtr = 0;
        virtual auto addVoxelMesh(const std::vector<foundation::RenderDataPtr> &frames, const util::Description &desc) -> VoxelMeshPtr = 0;
        virtual auto addGroundMesh(const foundation::RenderDataPtr &mesh, const foundation::RenderTexturePtr &texture) -> GroundMeshPtr = 0;
        virtual auto addVegetation(const ByteDataPtr &map, const math::vector3i &margs, const util::Description &desc, const foundation::RenderTexturePtr &tx) -> VegetationPtr = 0;
        virtual auto addVegetation(const ByteDataPtr &map, const math::vector3i &margs, const util::Description &desc, const std::vector<std::pair<std::uint32_t, ByteDataPtr>> &vxm) -> VegetationPtr = 0;
        virtual auto addParticles(const foundation::RenderTexturePtr &tx, const foundation::RenderTexturePtr &map, const util::Description &desc) -> ParticlesPtr = 0;
        virtual auto addLightSource(float r, float g, float b, float radius) -> LightSourcePtr = 0;
        
        virtual auto getCameraPosition() const -> math::vector3f = 0;
        virtual auto getScreenCoordinates(const math::vector3f &worldPosition) const -> math::vector2f = 0;
        virtual auto getWorldDirection(const math::vector2f &screenPosition, math::vector3f *outCamPosition = nullptr) const -> math::vector3f = 0;
        
        virtual void updateAndDraw(float dtSec) = 0;
        
        virtual void setLinesDrawingEnabled(bool enabled) = 0;
        
    public:
        virtual ~SceneInterface() = default;
    };
    
    using SceneInterfacePtr = std::shared_ptr<SceneInterface>;
}

