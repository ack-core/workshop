#include "raycheck.h"

namespace game {
    RaycheckContext::RaycheckContext(API &&api) : _api(std::move(api)) {
        util::Description spheres;
        spheres.setInteger("type", int(core::RaycastInterface::ShapeType::SPHERES));
        spheres.setVector3f("points", {0, 0, 0}, false);
        spheres.setNumber("radiuses", 10.0, false);
        spheres.setVector3f("points", {15, 5, 0}, false);
        spheres.setNumber("radiuses", 5.0, false);

        util::Description boxes;
        boxes.setInteger("type", int(core::RaycastInterface::ShapeType::BOXES));
        boxes.setVector3f("points", {0, 0, 0}, false);
        boxes.setVector3f("sizes", {20, 10, 10}, false);
        boxes.setVector3f("points", {0, 10, 0}, false);
        boxes.setVector3f("sizes", {5, 10, 5}, false);

        const math::vector3f tilted = math::vector3f(1, 1, 0).normalized();
        const std::pair<const util::Description &, math::transform3f> shapes[] = {
            {spheres, math::transform3f::identity().translated({-40, 0, -30})},
            {spheres, math::transform3f::identity().rotated({0, 1, 0}, 1.0f).translated({40, 10, -30})},
            {boxes, math::transform3f::identity().translated({-40, 0, 30})},
            {boxes, math::transform3f::identity().rotated({0, 1, 0}, -0.7f)},
            {boxes, math::transform3f::identity().rotated(tilted, 0.9f).translated({40, 10, 30})},
        };
        for (const auto &[desc, transform] : shapes) {
            _shapes.emplace_back(_api.raycast->addShape(desc, _shapes.size()));
            _shapes.back()->setTransform(transform);
        }

        _normal = _api.scene->addLineSet();
        _normal->setEnabled(false);

        _pointerToken = _api.platform->addPointerEventHandler([this](const foundation::PlatformPointerEventArgs &args) -> bool {
            if (args.type == foundation::PlatformPointerEventArgs::EventType::HOVER) {
                math::vector3f camPosition;
                const math::vector3f dir = _api.scene->getWorldDirection({args.coordinateX, args.coordinateY}, &camPosition);
                _result = _api.raycast->rayCast(camPosition, dir, 1000.0f);
            }
            return false;
        }, true);
    }

    RaycheckContext::~RaycheckContext() {
        _api.platform->removeEventHandler(_pointerToken);
    }

    void RaycheckContext::update(float dtSec) {
        if (_result.uniqueId != core::RaycastInterface::INVALID_UNIQUE_ID) {
            _normal->setLine(0, _result.point, _result.point + 4.0f * _result.normal, {1, 1, 0, 1});
            _normal->setEnabled(true);
        }
        else {
            _normal->setEnabled(false);
        }
    }
}
