
#pragma once
#include <game/context.h>
#include <unordered_map>

namespace game {
    class EditorCameraContext : public Context {
    public:
        EditorCameraContext(API &&api);
        ~EditorCameraContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        foundation::EventHandlerToken _token = foundation::INVALID_EVENT_TOKEN;
        std::size_t _pointerId = foundation::INVALID_POINTER_ID;
        math::vector2f _lockedCoordinates;
        math::vector3f _orbit = {135, 0, 0};
    };
}
