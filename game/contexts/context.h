
#pragma once
#include <memory>

#include "voxel/scene.h"
#include "ui/stage.h"

namespace game {
    class Context {
    public:
        virtual void update(float dtSec) = 0;
    
    public:
        virtual ~Context() = default;
    };

    template <typename Ctx> std::unique_ptr<Context> makeContext(const voxel::SceneInterfacePtr &scene, const ui::StageInterfacePtr &ui);
}
