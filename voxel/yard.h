
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform.h"
#include "foundation/math.h"

#include "voxel/mesh_provider.h"
#include "voxel/texture_provider.h"
#include "voxel/scene.h"

namespace voxel {
    // + collision
    // + object list must be separated from statics
    // + 
    class YardInterface {
    public:
        static std::shared_ptr<YardInterface> instance(
            const foundation::PlatformInterfacePtr &platform,
            const MeshProviderPtr &meshProvider,
            const TextureProviderPtr &textureProvider,
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
        virtual bool loadYard(const char *sourcepath) = 0;
        virtual auto addObject(const char *type, const math::vector3f &position, const math::vector3f &direction) -> std::shared_ptr<Object> = 0;
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~YardInterface() = default;
    };
    
    using YardInterfacePtr = std::shared_ptr<YardInterface>;
}

