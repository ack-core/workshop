
#pragma once
#include <memory>

#include "providers/resource_provider.h"
#include "voxel/scene.h"
#include "voxel/simulation.h"
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
        const voxel::SimulationInterfacePtr &simulation;
        const voxel::RaycastInterfacePtr &raycast;
        const ui::StageInterfacePtr &ui;
        const dh::DataHubPtr &dh;
        const game::StateManagerPtr &stateManager;
    };
    
    class Context {
    public:
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~Context() = default;
    };
    
    template <typename Ctx> std::unique_ptr<Context> makeContext(API &&api);
    using MakeContextFunc = std::unique_ptr<Context>(*)(API &&api);
}
