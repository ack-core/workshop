
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "voxel/scene.h"
#include "ui/stage.h"

namespace game {
    class StateManager {
    public:
        static std::shared_ptr<StateManager> instance(const voxel::SceneInterfacePtr &scene, const ui::StageInterfacePtr &ui);
        
    public:
        virtual void switchToState(const char *name) = 0;
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~StateManager() = default;
    };
    
    using StateManagerPtr = std::shared_ptr<StateManager>;
}
