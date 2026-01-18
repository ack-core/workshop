
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
        static std::shared_ptr<WorldInterface> instance(
            const foundation::PlatformInterfacePtr &platform,
            const resource::ResourceProviderPtr &resources,
            const voxel::SceneInterfacePtr &scene
        );
        
    public:
        struct Object;
        using ObjectPtr = std::shared_ptr<Object>;
        
        struct Object {
            virtual void loadResources(util::callback<void(WorldInterface::ObjectPtr)> &&completion);
            virtual void unloadResources();
            virtual void setTransform(const char *nodeName, const math::transform3f &trfm) = 0;
            
            // Play animation
            // @animationName - name of the animation node
            //     or name of the particles node
            //     or name of the mesh animation concatenated with the node name. Example: 'root...voxelMeshNode.animationName'
            // @completion    - callback is called when animation ends or instantly if animation wasn't found
            //
            virtual void play(const char *animationName, util::callback<void(WorldInterface::ObjectPtr)> &&completion);
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

