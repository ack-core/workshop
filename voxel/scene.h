
#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/math.h"

//#include "providers/mesh_provider.h"
#include "providers/texture_provider.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace voxel {
    class SceneInterface {
    public:
        static std::shared_ptr<SceneInterface> instance(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            const resource::TextureProviderPtr &textureProvider,
            const char *palette
        );
        
    public:
        struct VTXSVOX {
            std::int16_t positionX, positionY, positionZ;
            std::uint8_t colorIndex, mask;
            //std::uint8_t scaleX, scaleY, scaleZ;
        };
        struct VTXDVOX {
            float positionX, positionY, positionZ;
            std::uint8_t colorIndex, mask, r1, r2;
        };
        struct VTXNRMUV {
            float x, y, z, u;
            float nx, ny, nz, v;
        };
        
    public:
        struct BoundingBox {
            virtual ~BoundingBox() = default;
        };
        struct StaticModel {
            virtual ~StaticModel() = default;
        };
        struct TexturedModel {
            virtual ~TexturedModel() = default;
        };
        struct DynamicModel {
            virtual void setFrame(std::uint16_t index) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual ~DynamicModel() = default;
        };
        struct LightSource {
            virtual void setPosition(const math::vector3f &position) = 0;
            virtual ~LightSource() = default;
        };
        
        using BoundingBoxPtr = std::shared_ptr<BoundingBox>;
        using StaticModelPtr = std::shared_ptr<StaticModel>;
        using TexturedModelPtr = std::shared_ptr<TexturedModel>;
        using DynamicModelPtr = std::shared_ptr<DynamicModel>;
        using LightSourcePtr = std::shared_ptr<LightSource>;
        
    public:
        virtual void setCameraLookAt(const math::vector3f &position, const math::vector3f &sceneCenter) = 0;
        virtual void setSun(const math::vector3f &directionToSun, const math::color &rgba) = 0;
        
        virtual auto addBoundingBox(const math::bound3f &bbox) -> BoundingBoxPtr = 0;
        virtual auto addStaticModel(const std::vector<VTXSVOX> &voxels) -> StaticModelPtr = 0;
        virtual auto addTexturedModel(const std::vector<VTXNRMUV> &vtx, const std::vector<std::uint32_t> &idx, const foundation::RenderTexturePtr &tx) -> TexturedModelPtr = 0;
        virtual auto addDynamicModel(const std::vector<VTXDVOX> *frames, std::size_t frameCount, const math::transform3f &transform) -> DynamicModelPtr = 0;
        virtual auto addLightSource(const math::vector3f &position, float r, float g, float b, float radius) -> LightSourcePtr = 0;
        
        virtual void updateAndDraw(float dtSec) = 0;
        
    public:
        virtual ~SceneInterface() = default;
    };
    
    using SceneInterfacePtr = std::shared_ptr<SceneInterface>;
}

