
#pragma once

#include "camera.h"

namespace gears {
    class OrbitCameraController {
    public:
        OrbitCameraController(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<Camera> &camera) 
            : _platform(platform)
            , _camera(camera) 
        {
            setEnabled(true);
            _camera->setLookAtByRight(_center + _orbit, _center, math::vector3f(0, 1, 0).cross(_orbit));
        }

        ~OrbitCameraController() {
            setEnabled(false);
        }

        void setEnabled(bool enabled) {
            if (enabled) {
                _mouseEventHandlerToken = _platform->addTouchEventHandler(
                    [this](const foundation::PlatformTouchEventArgs &args) {
                        if (args.type == foundation::PlatformTouchEventArgs::EventType::START) {
                            _mouseLocked = true;
                            _lockedMouseCoordinates = { args.coordinateX, args.coordinateY };
                        }
                        if (args.type == foundation::PlatformTouchEventArgs::EventType::MOVE) {
                            if (_mouseLocked) {
                                float dx = args.coordinateX - _lockedMouseCoordinates.x;
                                float dy = args.coordinateY - _lockedMouseCoordinates.y;

                                _orbit.xz = _orbit.xz.rotated(dx / 100.0f);

                                math::vector3f right = math::vector3f(0, 1, 0).cross(_orbit);
                                math::vector3f rotatedOrbit = _orbit.rotated(right, dy / 100.0f);

                                if (fabs(math::vector3f(0, 1, 0).dot(rotatedOrbit.normalized())) < 0.96f) {
                                    _orbit = rotatedOrbit;
                                }

                                _camera->setLookAtByRight(_center + _orbit, _center, right);
                                _lockedMouseCoordinates = { args.coordinateX, args.coordinateY };
                            }
                        }
                        if (args.type == foundation::PlatformTouchEventArgs::EventType::FINISH) {
                            _mouseLocked = false;
                        }
                    }
                );
            }
            else {
                _platform->removeEventHandler(_mouseEventHandlerToken);
                _mouseEventHandlerToken = {};
            }
        }

    protected:
        std::shared_ptr<foundation::PlatformInterface> _platform;
        std::shared_ptr<Camera> _camera;

        bool _mouseLocked = false;
        math::vector2f _lockedMouseCoordinates;

        math::vector3f _center = { 8, 4, 8 };
        math::vector3f _orbit = { 5, 5, 5 };

        foundation::EventHandlerToken _mouseEventHandlerToken;
    };
}
