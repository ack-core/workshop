
#pragma once
#include <game/context.h>

namespace game {
    class VegetationContext : public Context {
    public:
        VegetationContext(API &&api);
        ~VegetationContext() override {}

        void init() override {}
        void update(float dtSec) override {}

    private:
        const API _api;

        core::SceneInterface::GroundMeshPtr _ground;
        std::vector<core::SceneInterface::VegetationPtr> _vg;
    };
}
