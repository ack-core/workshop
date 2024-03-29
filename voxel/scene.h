
#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/math.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace voxel {
    class SceneInterface {
    public:
        static std::shared_ptr<SceneInterface> instance(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering
        );
        
    public:
        struct VTXSVOX {
            std::int16_t positionX, positionY, positionZ;
            std::uint8_t colorIndex, mask;
            std::uint8_t scaleX, scaleY, scaleZ, reserved;
        };
        struct VTXDVOX {
            float positionX, positionY, positionZ;
            std::uint8_t colorIndex, mask, r0, r1;
        };
        struct VTXNRMUV {
            float x, y, z, u;
            float nx, ny, nz, v;
        };
        
    public:
        struct LineSet {
            virtual void setPosition(const math::vector3f &position) = 0;
            virtual auto addLine(const math::vector3f &start, const math::vector3f &end, const math::color &rgba) -> std::uint32_t = 0;
            virtual void setLine(std::uint32_t index, const math::vector3f &start, const math::vector3f &end, const math::color &rgba) = 0;
            virtual void removeAllLines() = 0;
            virtual ~LineSet() = default;
        };
        struct BoundingBox {
            virtual void setPosition(const math::vector3f &position) = 0;
            virtual ~BoundingBox() = default;
        };
        struct StaticModel {
            virtual void setPosition(const math::vector3f &position) = 0;
            virtual ~StaticModel() = default;
        };
        struct TexturedModel {
            virtual void setPosition(const math::vector3f &position) = 0;
            virtual ~TexturedModel() = default;
        };
        struct DynamicModel {
            virtual void setFrame(std::uint32_t index) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual ~DynamicModel() = default;
        };
        struct LightSource {
            virtual void setPosition(const math::vector3f &position) = 0;
            virtual ~LightSource() = default;
        };
        
        using LineSetPtr = std::shared_ptr<LineSet>;
        using BoundingBoxPtr = std::shared_ptr<BoundingBox>;
        using StaticModelPtr = std::shared_ptr<StaticModel>;
        using TexturedModelPtr = std::shared_ptr<TexturedModel>;
        using DynamicModelPtr = std::shared_ptr<DynamicModel>;
        using LightSourcePtr = std::shared_ptr<LightSource>;
        
    public:
        virtual void setCameraLookAt(const math::vector3f &position, const math::vector3f &sceneCenter) = 0;
        virtual void setSun(const math::vector3f &directionToSun, const math::color &rgba) = 0;
        
        virtual auto addLineSet(bool depthTested, std::uint32_t startCount) -> LineSetPtr = 0;
        virtual auto addBoundingBox(const math::bound3f &bbox) -> BoundingBoxPtr = 0;
        virtual auto addStaticModel(const std::vector<VTXSVOX> &voxels) -> StaticModelPtr = 0;
        virtual auto addTexturedModel(const std::vector<VTXNRMUV> &vtx, const std::vector<std::uint32_t> &idx, const foundation::RenderTexturePtr &tx) -> TexturedModelPtr = 0;
        virtual auto addDynamicModel(const std::vector<VTXDVOX> *frames, std::size_t frameCount, const math::transform3f &transform) -> DynamicModelPtr = 0;
        virtual auto addLightSource(const math::vector3f &position, float r, float g, float b, float radius) -> LightSourcePtr = 0;
        
        virtual auto getScreenCoordinates(const math::vector3f &worldPosition) -> math::vector2f = 0;
        virtual auto getWorldDirection(const math::vector2f &screenPosition) -> math::vector3f = 0;

        virtual void updateAndDraw(float dtSec) = 0;
        
    public:
        virtual ~SceneInterface() = default;
    };
    
    using SceneInterfacePtr = std::shared_ptr<SceneInterface>;
}

