
#pragma once
#include <memory>

#include "providers/resource_provider.h"
#include "voxel/scene.h"
#include "voxel/world.h"
#include "voxel/raycast.h"
#include "ui/stage.h"
#include "game/state_manager.h"
#include "datahub/datahub.h"

namespace game {
    struct API {
        const foundation::PlatformInterfacePtr &platform;
        const foundation::RenderingInterfacePtr &rendering;
        const resource::ResourceProviderPtr &resources;
        const voxel::SceneInterfacePtr &scene;
        const voxel::WorldInterfacePtr &world;
        const voxel::RaycastInterfacePtr &raycast;
        const ui::StageInterfacePtr &ui;
        const dh::DataHubPtr &dh;
        const game::StateManagerPtr &stateManager;
    };
    
    class Interface {
    public:
        virtual ~Interface() = default;
    };
    
    class Context {
    public:
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~Context() = default;
    };

    template <typename I> I& makeArg(Interface **existInterfaces, std::size_t count) {
        I *ptr = nullptr;
        
        for (std::size_t i = 0; i < count; i++) {
            if ((ptr = dynamic_cast<I *>(existInterfaces[i])) != nullptr) {
                break;
            }
        }
        
        return *ptr;
    }
    template <typename Ctx, typename... Interfaces> std::shared_ptr<Context> makeContext(API &&api, Interface **existInterfaces, std::size_t count) {
        return std::make_shared<Ctx>(std::move(api), makeArg<Interfaces>(existInterfaces, count)...);
    }
    
    using MakeContextFunc = std::shared_ptr<Context>(*)(API &&api, Interface **existInterfaces, std::size_t count);
}
