
#pragma once
#include <memory>

#include "core/scene.h"
#include "core/world.h"
#include "core/raycast.h"
#include "core/simulation.h"
#include "ui/stage.h"
#include "datahub/datahub.h"

namespace game {
    class StateManager {
    public:
        static std::shared_ptr<StateManager> instance(
            const foundation::PlatformInterfacePtr &platform,
            const resource::ResourceProviderPtr &resourceProvider,
            const core::SceneInterfacePtr &scene,
            const core::WorldInterfacePtr &world,
            const core::RaycastInterfacePtr &raycast,
            const core::SimulationInterfacePtr &simulation,
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
