
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
            virtual void setLine(std::uint32_t index, const math::vector3f &start, const math::vector3f &end, const math::color &rgba) = 0;
            virtual ~LineSet() = default;
        };
        struct BoundingBox {
            virtual void setBBoxData(const math::bound3f &bbox) = 0;
            virtual void setColor(const math::color &rgba) = 0;
            virtual ~BoundingBox() = default;
        };
        struct StaticMesh {
            virtual void setPosition(const math::vector3f &position) = 0;
            virtual void setFrame(std::uint32_t index) = 0;
            virtual ~StaticMesh() = default;
        };
        struct DynamicMesh {
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setFrame(std::uint32_t index) = 0;
            virtual ~DynamicMesh() = default;
        };
        struct TexturedMesh {
            virtual void setPosition(const math::vector3f &position) = 0;
            virtual ~TexturedMesh() = default;
        };
        struct LightSource {
            virtual void setPosition(const math::vector3f &position) = 0;
            virtual ~LightSource() = default;
        };
        
        using LineSetPtr = std::shared_ptr<LineSet>;
        using BoundingBoxPtr = std::shared_ptr<BoundingBox>;
        using StaticMeshPtr = std::shared_ptr<StaticMesh>;
        using DynamicMeshPtr = std::shared_ptr<DynamicMesh>;
        using TexturedMeshPtr = std::shared_ptr<TexturedMesh>;
        using LightSourcePtr = std::shared_ptr<LightSource>;
        
    public:
        virtual void setCameraLookAt(const math::vector3f &position, const math::vector3f &sceneCenter) = 0;
        virtual void setSun(const math::vector3f &directionToSun, const math::color &rgba) = 0;
        
        virtual auto addLineSet(std::uint32_t count) -> LineSetPtr = 0;
        virtual auto addBoundingBox(const math::bound3f &bbox) -> BoundingBoxPtr = 0;
        virtual auto addStaticMesh(const std::vector<VTXSVOX> *frames, std::size_t frameCount) -> StaticMeshPtr = 0;
        virtual auto addDynamicMesh(const std::vector<VTXDVOX> *frames, std::size_t frameCount) -> DynamicMeshPtr = 0;
        virtual auto addTexturedMesh(const std::vector<VTXNRMUV> &vtx, const std::vector<std::uint32_t> &idx, const foundation::RenderTexturePtr &tx) -> TexturedMeshPtr = 0;
        virtual auto addLightSource(const math::vector3f &position, float r, float g, float b, float radius) -> LightSourcePtr = 0;
        
        virtual auto getScreenCoordinates(const math::vector3f &worldPosition) -> math::vector2f = 0;
        virtual auto getWorldDirection(const math::vector2f &screenPosition) -> math::vector3f = 0;

        virtual void updateAndDraw(float dtSec) = 0;
        
    public:
        virtual ~SceneInterface() = default;
    };
    
    using SceneInterfacePtr = std::shared_ptr<SceneInterface>;
}

