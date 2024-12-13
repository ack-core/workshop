
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "camera_access_interface.h"

namespace game {
    class EditorCameraContext : public Context, public CameraAccessInterface {
    public:
        EditorCameraContext(API &&api);
        ~EditorCameraContext() override;
        
        float getOrbitSize() const override;
        void update(float dtSec) override;
        
    private:
        const API _api;
        foundation::EventHandlerToken _token = foundation::INVALID_EVENT_TOKEN;
        std::size_t _pointerId = foundation::INVALID_POINTER_ID;
        math::vector2f _lockedCoordinates;
        math::vector3f _orbit = {100, 100, 100};
    };
}
