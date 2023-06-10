
#include "debug_context.h"

namespace game {
    template <> std::unique_ptr<Context> makeContext<DebugContext>(API &&api) {
        return std::make_unique<DebugContext>(std::move(api));
    }
    
    DebugContext::DebugContext(API &&api) : _api(std::move(api)) {
        _api.scene->setCameraLookAt(_center + _orbit, _center);
        _touchEventHandlerToken = _api.platform->addTouchEventHandler(
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

                        _api.scene->setCameraLookAt(_center + _orbit, _center);
                        _lockedMouseCoordinates = { args.coordinateX, args.coordinateY };
                    }
                }
//                if (args.type == foundation::PlatformTouchEventArgs::EventType::FINISH) {
//                    _mouseLocked = false;
//                    if (args.coordinateX < 50 && args.coordinateY < 50) {
//                        if (dbg_counter > 0) dbg_counter--;
//                    }
//                    if (args.coordinateX > _api.platform->getScreenWidth() - 50 && args.coordinateY < 50) {
//                        if (dbg_counter < 1000) dbg_counter++;
//                    }
//                }
            }
        );
        
        if (_api.yard->loadYard("debug")) {
            _api.yard->addObject("player", math::vector3f(16, 0, 16), math::vector3f(1.0, 0.0, 0.0));
        }
        
//        {
//            voxel::Mesh mesh;
//            int16_t offset[3] = {-22, -1, -8};
//
//            if (_api.factory->createMesh("1.vox", offset, mesh)) {
//                _api.scene->addStaticModel(mesh, {});
//            }
//        }
//
//        {
//            voxel::Mesh mesh;
//            int16_t offset[3] = {-2, 0, -2};
//
//            if (_api.factory->createMesh("knight.vox", offset, mesh)) {
//                _api.scene->addDynamicModel(mesh, {3, 0, 0}, M_PI / 4.0);
//            }
//        }

        _api.scene->setCameraLookAt(_center + _orbit, _center);
    }
    
    DebugContext::~DebugContext() {
        _api.platform->removeEventHandler(_touchEventHandlerToken);
    }
    
    void DebugContext::update(float dtSec) {
    
    }
}
