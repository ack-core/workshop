
#pragma once
#include "context.h"

namespace game {
    class DebugContext : public Context {
    public:
        DebugContext(const voxel::SceneInterfacePtr &scene, const ui::StageInterfacePtr &ui);
        ~DebugContext() override;
        
        void update(float dtSec) override;
        
    private:
        const foundation::PlatformInterfacePtr _platform;
        const voxel::SceneInterfacePtr _scene;
        const ui::StageInterfacePtr _ui;
    
        bool _mouseLocked = false;
        math::vector2f _lockedMouseCoordinates;

        math::vector3f _center = { 0, 0, 0 };
        math::vector3f _orbit = { 45, 45, 45 };

        foundation::EventHandlerToken _touchEventHandlerToken;
    };
}
