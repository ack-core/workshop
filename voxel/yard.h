
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
        struct Actor {
            virtual void rotate(const math::vector3f &targetDirection) = 0;
            virtual auto getRadius() const -> float = 0;
            virtual auto getPosition() const -> const math::vector3f & = 0;
            virtual auto getDirection() const -> const math::vector3f & = 0;
            virtual ~Actor() = default;
        };
        struct Stead {
            virtual void setPosition(const math::vector3f &position) = 0;
            virtual auto getPosition() const -> const math::vector3f & = 0;
            virtual ~Stead() = default;
        };
        struct Thing {
            virtual void setPosition(const math::vector3f &position) = 0;
            virtual auto getPosition() const -> const math::vector3f & = 0;
            virtual ~Thing() = default;
        };
        
        using SteadPtr = std::shared_ptr<YardInterface::Stead>;
        using ThingPtr = std::shared_ptr<YardInterface::Thing>;
        using ActorPtr = std::shared_ptr<YardInterface::Actor>;
        
    public:
        virtual void setCameraLookAt(const math::vector3f &position, const math::vector3f &target) = 0;
        
        // Yard source format:
        // s--------------------------------------
        // options {
        //     nearby 1
        //     skybox "name"
        // }
        // ...
        // stead <id> {
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
        // actor "type" {
        //     model "name"
        //     center 3 0 3
        //     radius 2.5
        // }
        // ...
        // s--------------------------------------
        //
        virtual void loadYard(const char *sourcePath, util::callback<void(bool loaded)> &&completion) = 0;
        virtual void saveYard(const char *outputPath, util::callback<void(bool saved)> &&completion) = 0;
        virtual void addActorType(const char *type, const char *model, const math::vector3f &offset, float radius) = 0;
        
        virtual auto addThing(const char *model, const math::vector3f &position) -> std::shared_ptr<Thing> = 0;
        virtual auto addStead(const char *heightmap, const char *texture, const math::vector3f &position) -> std::shared_ptr<Stead> = 0;
        virtual auto addActor(const char *type, const math::vector3f &position, const math::vector3f &direction) -> std::shared_ptr<Actor> = 0;
        
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~YardInterface() = default;
    };
    
    using YardInterfacePtr = std::shared_ptr<YardInterface>;
}

