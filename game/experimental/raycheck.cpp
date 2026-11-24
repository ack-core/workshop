#include "raycheck.h"

namespace game {
    RaycheckContext::RaycheckContext(API &&api) : _api(std::move(api)) {
        util::Description spheres;
        spheres.setInteger("type", int(core::RaycastInterface::ShapeType::SPHERES));
        spheres.setVector3f("points", {0, 0, 0}, false);
        spheres.setNumber("radiuses", 1.0, false);
        spheres.setVector3f("points", {1.5f, 0.5f, 0}, false);
        spheres.setNumber("radiuses", 0.5, false);

        util::Description boxes;
        boxes.setInteger("type", int(core::RaycastInterface::ShapeType::BOXES));
        boxes.setVector3f("points", {0, 0, 0}, false);
        boxes.setVector3f("sizes", {2, 1, 1}, false);
        boxes.setVector3f("points", {0, 1, 0}, false);
        boxes.setVector3f("sizes", {0.5f, 1, 0.5f}, false);

        const math::vector3f tilted = math::vector3f(1, 1, 0).normalized();
        const std::pair<const util::Description &, math::transform3f> shapes[] = {
            {spheres, math::transform3f::identity().translated({-4, 0, -3})},
            {spheres, math::transform3f::identity().rotated({0, 1, 0}, 1.0f).translated({4, 1, -3})},
            {boxes, math::transform3f::identity().translated({-4, 0, 3})},
            {boxes, math::transform3f::identity().rotated({0, 1, 0}, 0.7f)},
            {boxes, math::transform3f::identity().rotated(tilted, 0.9f).translated({4, 1, 3})},
        };
        for (const auto &[desc, transform] : shapes) {
            _shapes.emplace_back(_api.raycast->addShape(desc, _shapes.size()));
            _shapes.back()->setTransform(transform);
        }

        _normal = _api.scene->addLineSet();
        _normal->setEnabled(false);

        _pointerToken = _api.platform->addPointerEventHandler([this](const foundation::PlatformPointerEventArgs &args) -> bool {
            if (args.type != foundation::PlatformPointerEventArgs::EventType::START &&
                args.type != foundation::PlatformPointerEventArgs::EventType::MOVE &&
                args.type != foundation::PlatformPointerEventArgs::EventType::HOVER) {
                return false;
            }
            math::vector3f camPosition;
            const math::vector3f dir = _api.scene->getWorldDirection({args.coordinateX, args.coordinateY}, &camPosition);
            _result = _api.raycast->rayCast(camPosition, dir, 100.0f);
            return true;
        });
    }

    RaycheckContext::~RaycheckContext() {
        _api.platform->removeEventHandler(_pointerToken);
    }

    void RaycheckContext::update(float dtSec) {
        if (_result.uniqueId != core::RaycastInterface::INVALID_UNIQUE_ID) {
            _normal->setLine(0, _result.point, _result.point + _result.normal, {1, 1, 0, 1});
            _normal->setEnabled(true);
        }
        else {
            _normal->setEnabled(false);
        }
        _api.scene->setCameraLookAt({8, 12, 12}, {0, 0, 0});
    }
}
