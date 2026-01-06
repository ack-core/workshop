
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "camera_access_interface.h"

namespace game {
    class EditorCameraContext : public Context, public CameraAccessInterface {
    public:
        EditorCameraContext(API &&api);
        ~EditorCameraContext() override;
        
        auto getOrbitSize() const -> float override;
        auto getTarget() const -> math::vector3f override;
        void setTarget(const math::vector3f &position) override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        foundation::EventHandlerToken _pointerToken = foundation::INVALID_EVENT_TOKEN;
        foundation::EventHandlerToken _keyboardToken = foundation::INVALID_EVENT_TOKEN;
        std::size_t _pointerId = foundation::INVALID_POINTER_ID;
        math::vector2f _lockedCoordinates;
        math::vector3f _target = {0, 0, 0};
        math::vector3f _orbit = {100, 100, 100};
        int movLR = 0, movBF = 0;

        voxel::SceneInterface::LineSetPtr _axis;
    };
}
