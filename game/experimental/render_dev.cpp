
#include "render_dev.h"
#include "ui/extensions.h"
#include <list>

namespace game {
    RenderDevContext::RenderDevContext(API &&api) : _api(std::move(api)) {
        _token = _api.platform->addPointerEventHandler(
            [this](const foundation::PlatformPointerEventArgs &args) -> bool {
                if (args.type == foundation::PlatformPointerEventArgs::EventType::START) {
                    _pointerId = args.pointerID;
                    _lockedCoordinates = { args.coordinateX, args.coordinateY };
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::MOVE) {
                    if (_pointerId != foundation::INVALID_POINTER_ID) {
                        float dx = args.coordinateX - _lockedCoordinates.x;
                        float dy = args.coordinateY - _lockedCoordinates.y;

                        _orbit.xz = _orbit.xz.rotated(dx / 200.0f);

                        math::vector3f right = math::vector3f(0, 1, 0).cross(_orbit).normalized();
                        math::vector3f rotatedOrbit = _orbit.rotated(right, dy / 200.0f);

                        if (fabs(math::vector3f(0, 1, 0).dot(rotatedOrbit.normalized())) < 0.96f) {
                            _orbit = rotatedOrbit;
                        }

                        _lockedCoordinates = { args.coordinateX, args.coordinateY };
                    }
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::FINISH) {
                    _pointerId = foundation::INVALID_POINTER_ID;
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::CANCEL) {
                    _pointerId = foundation::INVALID_POINTER_ID;
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::WHEEL) {
                    const float percent = std::max(1000.0f + float(args.flags.wheel), 900.0f) / 1000.0f;
                    const float orbitLength = _orbit.length();

                    if ((orbitLength >= 50.0f || percent > 1.0f) && (orbitLength <= 1000.0f || percent < 1.0f)) {
                        _orbit = _orbit * percent;
                    }
                }

                return true;
            }
        );

        _axis = _api.scene->addLineSet();
        _axis->setLine(0, {0, 0, 0}, {1000, 0, 0}, {1, 0, 0, 0.9});
        _axis->setLine(1, {0, 0, 0}, {0, 1000, 0}, {0, 1, 0, 0.9});
        _axis->setLine(2, {0, 0, 0}, {0, 0, 1000}, {0, 0, 1, 0.9});
        _axis->setLine(3, {0, 0, 0}, {-1000, 0, 0}, {0.5, 0.5, 0.5, 0.9});
        _axis->setLine(4, {0, 0, 0}, {0, 0, -1000}, {0.5, 0.5, 0.5, 0.9});

        api.stateManager->switchToState("uielements");
    }
    
    RenderDevContext::~RenderDevContext() {
        _api.platform->removeEventHandler(_token);
    }
    
    void RenderDevContext::update(float dtSec) {
        _api.scene->setCameraLookAt(_orbit, math::vector3f{0, 0, 0});
    }
}
