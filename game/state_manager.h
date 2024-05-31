
#pragma once
#include <memory>

#include "voxel/scene.h"
#include "voxel/simulation.h"
#include "voxel/raycast.h"
#include "ui/stage.h"
#include "datahub/datahub.h"

namespace game {
    class StateManager {
    public:
        static std::shared_ptr<StateManager> instance(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            const resource::ResourceProviderPtr &resourceProvider,
            const voxel::SceneInterfacePtr &scene,
            const voxel::SimulationInterfacePtr &simulation,
            const voxel::RaycastInterfacePtr &raycast,
            const ui::StageInterfacePtr &ui,
            const dh::DataHubPtr &dh
        );
        
    public:
        // TODO: notifyContexts(...)
        //
        virtual void switchToState(const char *name) = 0;
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~StateManager() = default;
    };
    
    using StateManagerPtr = std::shared_ptr<StateManager>;
}
