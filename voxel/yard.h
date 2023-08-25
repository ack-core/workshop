
#pragma once
#include "foundation/util.h"
#include "foundation/math.h"
#include "foundation/platform.h"

#include "providers/mesh_provider.h"
#include "providers/texture_provider.h"

#include "voxel/scene.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace voxel {
    // TODO:
    // + collision
    // + object list must be separated from statics
    // + 
    class YardInterface {
    public:
        static std::shared_ptr<YardInterface> instance(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            const resource::MeshProviderPtr &meshProvider,
            const resource::TextureProviderPtr &textureProvider,
            const SceneInterfacePtr &scene
        );
        
    public:
        struct Object {
            virtual void attach(const char *helper, const char *type, const math::transform3f &trfm) = 0;
            virtual void detach(const char *helper) = 0;
            virtual void instantMove(const math::vector3f &position) = 0;
            virtual void continuousMove(const math::vector3f &increment) = 0;
            virtual void rotate(const math::vector3f &targetDirection) = 0;
            virtual auto getPosition() const -> const math::vector3f & = 0;
            virtual auto getDirection() const -> const math::vector3f & = 0;
            virtual void update(float dtSec) = 0;
            virtual ~Object() = default;
        };
        
    public:
        // Yard source format:
        // s--------------------------------------
        // options {
        //     nearby 1
        //     skybox "name"
        // }
        // ...
        // square <id> {
        //     heightmap "name"
        //     texture "name"
        //     link <N> <id_1> .. <id_N>
        // }
        // ...
        // thing <id> {
        //     model "name"
        //     visual 0
        //     collision 1
        //     direction -1.0 1.0 1.0
        //     link <N> <id_1> .. <id_N>
        // }
        // ...
        // object "type" {
        //     model "name"
        //     center 3 0 3
        // }
        // ...
        // s--------------------------------------
        //
        virtual void loadYard(const char *sourcepath, util::callback<void(bool loaded)> &&completion) = 0;
        virtual auto addObject(const char *type, const math::vector3f &position, const math::vector3f &direction) -> std::shared_ptr<Object> = 0;
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~YardInterface() = default;
    };
    
    using YardInterfacePtr = std::shared_ptr<YardInterface>;
}

