
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
                _mouseEventHandlersToken = _platform->addMouseEventHandlers(
                    [this](const foundation::PlatformMouseEventArgs &args) {
                        _mouseLocked = true;
                        _lockedMouseCoordinates = { args.coordinateX, args.coordinateY };
                        return true;
                    },
                    [this](const foundation::PlatformMouseEventArgs &args) {
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

                            return true;
                        }
                        else if (args.wheel != 0) {
                            float multiplier = args.wheel > 0 ? 0.9f : 1.1f;

                            math::vector3f right = math::vector3f(0, 1, 0).cross(_orbit);
                            _orbit = _orbit.normalized(_orbit.length() * multiplier);
                            _camera->setLookAtByRight(_center + _orbit, _center, right);
                            return true;
                        }

                        return false;
                    },
                    [this](const foundation::PlatformMouseEventArgs &args) {
                        _mouseLocked = false;
                        return true;
                    }
                );
            }
            else {
                _platform->removeEventHandlers(_mouseEventHandlersToken);
                _mouseEventHandlersToken = {};
            }
        }

    protected:
        std::shared_ptr<foundation::PlatformInterface> _platform;
        std::shared_ptr<Camera> _camera;

        //std::size_t _lockedTouchID;
        bool _mouseLocked = false;
        math::vector2f _lockedMouseCoordinates;

        math::vector3f _center = { 0, 0, 0 };
        math::vector3f _orbit = { 50, 20, 50 };

        foundation::EventHandlersToken _mouseEventHandlersToken;
    };
}