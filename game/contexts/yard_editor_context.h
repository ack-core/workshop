
#pragma once
#include "context.h"

namespace game {
    class YardEditorContext : public Context {
    public:
        class MovingTool {
        public:
            MovingTool(const API &_api, float distance);
            ~MovingTool();
            
            void initialize();
            void update(const math::vector3f &camPos, float distance);

        public:
            const API &_api;
            float _axisLength;
            math::vector3f _camPos;
            math::vector3f _position = {0, 0, 0};
            
            math::vector3f _capturedPosition;
            math::vector3f _capturedIntersectPos;
            math::vector3f _capturedPlaneNormal;

            voxel::SceneInterface::LineSetPtr _lineset;
            ui::PivotPtr _pivotX, _pivotY, _pivotZ;
            ui::ImagePtr _tipX, _tipY, _tipZ;
        };
        
    public:
        YardEditorContext(API &&api);
        ~YardEditorContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        
        std::size_t _pointerId = foundation::INVALID_POINTER_ID;
        math::vector2f _lockedCoordinates;
        math::vector3f _center = { 8, 0, 8 };
        math::vector3f _orbit = { 0, 58, 42 };
        float _distanceMultiplier = 0.0f;

        foundation::EventHandlerToken _touchEventHandlerToken;
        
        MovingTool _movingTool;
        voxel::SceneInterface::LineSetPtr _quad;
        ui::ElementPtr _joystickElement;
        math::vector2f _joystickMovement = {0, 0};
        ui::ImagePtr _zoomIn;
        ui::ImagePtr _zoomOut;
    };
}
