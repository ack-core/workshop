
#pragma once
#include "foundation/util.h"
#include "foundation/math.h"
#include "foundation/platform.h"
#include "providers/resource_provider.h"
#include "core/scene.h"
#include "raycast.h"
#include "simulation.h"

#include <memory>
#include <forward_list>

namespace core {
    class WorldInterface {
    public:
        enum class NodeType : std::size_t {
            PREFAB    = 0,
            ANIMATION = 1,
            VOXEL     = 10, // TODO: 1/3 subdetail
            LOWPOLY   = 11,
            GROUND    = 12, // TODO: update minimal
            PARTICLES = 20, // TODO: angle, rendering
            TRAILS    = 21,
            LIGHT     = 22,
            DECALS    = 23,
            RAYCAST   = 30, // TODO: impl, triangles, extended moving tool
            COLLISION = 31, // TODO: impl
            HELPER    = 32,
            SOUND     = 40,
            HAPTIC    = 41,
            _count
        };
        
    public:
        static std::shared_ptr<WorldInterface> instance(
            const foundation::PlatformInterfacePtr &platform,
            const resource::ResourceProviderPtr &resources,
            const core::SceneInterfacePtr &scene,
            const core::RaycastInterfacePtr &raycast,
            const core::SimulationInterfacePtr &simulation
        );
        
    public:
        struct Object;
        using ObjectPtr = std::shared_ptr<Object>;
        
        struct Object {
            virtual auto getId() const -> std::size_t = 0;
            
            virtual void loadResources(util::callback<void(core::WorldInterface::Object &)> &&completion) = 0;
            virtual void unloadResources() = 0;
            
            virtual void setPosition(const math::vector3f &pos) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setLocalTransform(const char *nodeName, const math::transform3f &trfm) = 0;
            virtual auto getLocalTransform(const char *nodeName) -> const math::transform3f & = 0;
            virtual auto getWorldTransform(const char *nodeName) -> const math::transform3f & = 0;
            virtual void setVelocity(const math::vector3f &v) = 0;
            
            // Play animation
            // @animationName - name of the animation node
            //     or name of the mesh animation concatenated with the node name. Example: 'root...voxelMeshNode.animationName'
            //     or name of the particles node
            //
            // @completion    - callback is called when animation ends or instantly if animation wasn't found.
            // callback is called at the end of every cycle of looped particles or looped animations
            //
            virtual void play(const char *animationName, util::callback<void(core::WorldInterface::Object &)> &&completion) = 0;
            
            virtual ~Object() = default;
        };
        
    public:
        virtual auto getObject(const char *name) -> ObjectPtr = 0;
        virtual auto createObject(const char *name, const char *prefabPath) -> ObjectPtr = 0;
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~WorldInterface() = default;
    };
    
    using WorldInterfacePtr = std::shared_ptr<WorldInterface>;
}

