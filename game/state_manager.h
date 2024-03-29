
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "voxel/scene.h"
#include "voxel/yard.h"
#include "ui/stage.h"
#include "datahub/datahub.h"

namespace game {
    class StateManager {
    public:
        static std::shared_ptr<StateManager> instance(
            const foundation::PlatformInterfacePtr &platform,
            const voxel::SceneInterfacePtr &scene,
            const voxel::YardInterfacePtr &yard,
            const ui::StageInterfacePtr &ui,
            const dh::DataHubPtr &dh
        );
        
    public:
        virtual void switchToState(const char *name) = 0;
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~StateManager() = default;
    };
    
    using StateManagerPtr = std::shared_ptr<StateManager>;
}
