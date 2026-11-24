#pragma once
#include <game/context.h>

namespace game {
    class RaycheckContext : public Context {
    public:
        RaycheckContext(API &&api);
        ~RaycheckContext() override;

        void init() override {}
        void update(float dtSec) override;

    private:
        const API _api;

        foundation::EventHandlerToken _pointerToken = foundation::INVALID_EVENT_TOKEN;
        std::vector<core::RaycastInterface::ShapePtr> _shapes;
        core::SceneInterface::LineSetPtr _normal;
        core::RaycastInterface::RaycastResult _result;
    };
}
