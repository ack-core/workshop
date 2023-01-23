
#include "debug_context.h"

namespace game {
    template <> std::unique_ptr<Context> makeContext<DebugContext>(const voxel::SceneInterfacePtr &scene, const ui::StageInterfacePtr &ui) {
        return std::make_unique<DebugContext>(scene, ui);
    }
    
    DebugContext::DebugContext(const voxel::SceneInterfacePtr &scene, const ui::StageInterfacePtr &ui) : _platform(scene->getPlatformInterface()), _scene(scene), _ui(ui) {
        _scene->setCameraLookAt(_center + _orbit, _center);
        _touchEventHandlerToken = _platform->addTouchEventHandler(
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

                        _scene->setCameraLookAt(_center + _orbit, _center);
                        _lockedMouseCoordinates = { args.coordinateX, args.coordinateY };
                    }
                }
                if (args.type == foundation::PlatformTouchEventArgs::EventType::FINISH) {
                    _mouseLocked = false;
                }
            }
        );
        
        _scene->addStaticModel("1.vox", {-16, 0, -16});
        _scene->addStaticModel("1.vox", {23, 0, -16});
        _scene->addStaticModel("knight.vox", {-12, 1, 6});
        _scene->setCameraLookAt(math::vector3f(45.0f, 45.0f, 45.0f), math::vector3f(0, 0, 0));
    }
    
    DebugContext::~DebugContext() {
        _platform->removeEventHandler(_touchEventHandlerToken);
        _touchEventHandlerToken = {};
    }
    
    void DebugContext::update(float dtSec) {
    
    }
}
