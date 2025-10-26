
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
            virtual void setActive(bool active) = 0;
            virtual void loadResources(util::callback<void(WorldInterface::ObjectPtr)> &&completion);
            virtual void unloadResources();
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void release() = 0;
            virtual ~Object() = default;
        };
        
    public:
        virtual void loadWorld(const char *descPath, util::callback<void(bool, float)> &&progress) = 0;
        virtual auto getObject(const char *name) -> WorldInterface::ObjectPtr = 0;
        virtual auto createObject(const char *name, const char *prefabPath, util::callback<void(WorldInterface::ObjectPtr)> &&completion) -> ObjectPtr = 0;
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~WorldInterface() = default;
    };
    
    using WorldInterfacePtr = std::shared_ptr<WorldInterface>;
}

