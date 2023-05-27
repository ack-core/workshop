
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
            virtual void setPrimary() = 0;
            virtual bool instantMove(const math::vector3f &position) = 0;
            virtual void continuousMove(const math::vector3f &increment) = 0;
            virtual void rotate(const math::vector3f &targetDirection) = 0;
            virtual auto getPosition() const -> math::vector3f & = 0;
            virtual auto getRotation() const -> float = 0;
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
        // static <id> {
        //     model "name"
        //     visual 0
        //     collision 1
        //     link <N> <id_1> .. <id_N>
        // }
        // ...
        // square <id> {
        //     heightmap "name"
        //     texture "name"
        //     link <N> <id_1> .. <id_N>
        // }
        // ...
        // object "type" {
        //     model "name"
        // }
        // ...
        // s--------------------------------------
        //
        virtual bool loadYard(const char *src) = 0;
        virtual auto addObject(const char *type) -> std::shared_ptr<Object> = 0;
        
    public:
        virtual ~YardInterface() = default;
    };
    
    using YardInterfacePtr = std::shared_ptr<YardInterface>;
}

