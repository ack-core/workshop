#pragma once
#include <game/context.h>

namespace game {
    class CollisionContext : public Context {
    public:
        CollisionContext(API &&api);
        ~CollisionContext() override;

        void init() override {}
        void update(float dtSec) override;

    private:
        const API _api;

        foundation::EventHandlerToken _keyboardToken = foundation::INVALID_EVENT_TOKEN;
        foundation::EventHandlerToken _pointerToken = foundation::INVALID_EVENT_TOKEN;
        math::vector3f _direction = {0, 0, 0};
        core::SimulationInterface::BodyPtr _player;
        std::vector<core::SimulationInterface::BodyPtr> _obstacles;
    };
}
