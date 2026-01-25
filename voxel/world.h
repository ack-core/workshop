
#pragma once
#include "foundation/util.h"
#include "foundation/math.h"
#include "foundation/platform.h"
#include "providers/resource_provider.h"
#include "voxel/scene.h"

#include <memory>
#include <forward_list>

namespace voxel {
    class WorldInterface {
    public:
        enum class NodeType : std::size_t {
            PREFAB = 0,
            VOXELMESH,
            PARTICLES,
            GROUND,
            LOWPOLYMESH,
            RAYCAST,
            COLLISION,
            ANIMATION,
            _count
        };
        
    public:
        static std::shared_ptr<WorldInterface> instance(
            const foundation::PlatformInterfacePtr &platform,
            const resource::ResourceProviderPtr &resources,
            const voxel::SceneInterfacePtr &scene
        );
        
    public:
        struct Object;
        using ObjectPtr = std::shared_ptr<Object>;
        
        struct Object {
            virtual auto getId() const -> std::size_t = 0;
            
            virtual void loadResources(util::callback<void(voxel::WorldInterface::Object &)> &&completion) = 0;
            virtual void unloadResources() = 0;
            
            virtual void setPosition(const math::vector3f &pos) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setLocalTransform(const char *nodeName, const math::transform3f &trfm) = 0;
            virtual auto getLocalTransform(const char *nodeName) -> const math::transform3f & = 0;
            virtual auto getWorldTransform(const char *nodeName) -> const math::transform3f & = 0;
            virtual void setVelocity(const math::vector3f &v) = 0;
            
            // Play animation
            // @animationName - name of the animation node
            //     or name of the particles node
            //     or name of the mesh animation concatenated with the node name. Example: 'root...voxelMeshNode.animationName'
            // @completion    - callback is called when animation ends or instantly if animation wasn't found.
            // callback is called at the end of every cycle of looped particles or looped animations
            //
            virtual void play(const char *animationName, util::callback<void(voxel::WorldInterface::Object &)> &&completion) = 0;
            
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

