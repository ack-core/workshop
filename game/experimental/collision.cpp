#include "collision.h"

namespace game {
    CollisionContext::CollisionContext(API &&api) : _api(std::move(api)) {
        util::Description playerDesc;
        playerDesc.setInteger("type", int(core::SimulationInterface::ShapeType::CircleXZ));
        playerDesc.setNumber("mass", 3.0);
        playerDesc.setNumber("radius", 0.5);
        _player = _api.simulation->addBody(playerDesc);

        util::Description smallDesc = playerDesc;
        smallDesc.setNumber("mass", 1.0);
        smallDesc.setNumber("radius", 0.2);
        _tied = _api.simulation->addBody(smallDesc);
        _tied->setTransform(math::transform3f::identity().translated({1, 0, 0}));
        _tie = _api.simulation->addConstraint(_player, _tied, 1.0f);

        _circles.emplace_back(_api.simulation->addBody(playerDesc));
        _circles.back()->setTransform(math::transform3f::identity().translated({-2, 0, 1}));

        util::Description bigDesc = playerDesc;
        bigDesc.setNumber("mass", 12.0);
        bigDesc.setNumber("radius", 1.0);
        _circles.emplace_back(_api.simulation->addBody(bigDesc));
        _circles.back()->setTransform(math::transform3f::identity().translated({1, 0, -3}));

        const std::vector<std::vector<math::vector3f>> obstacles = {
            {{3, 0, -2}, {3, 0, 2}, {4, 0, 2}, {4, 0, -2}},
            {{-5, 0, -4}, {-4, 0, -2}, {-2, 0, -4}},
            {{-2, 0, 4}, {-2, 0, 5}, {2, 0, 5}, {2, 0, 4}},
        };
        for (const auto &points : obstacles) {
            util::Description desc;
            desc.setInteger("type", int(core::SimulationInterface::ShapeType::ObstaclePolygonXZ));
            for (const auto &point : points) {
                desc.setVector3f("points", point, false);
            }
            _obstacles.emplace_back(_api.simulation->addBody(desc));
        }

        _pointerToken = _api.platform->addPointerEventHandler([this](const foundation::PlatformPointerEventArgs &args) -> bool {
            if (args.type != foundation::PlatformPointerEventArgs::EventType::START) {
                return false;
            }
            math::vector3f camPosition;
            const math::vector3f dir = _api.scene->getWorldDirection({args.coordinateX, args.coordinateY}, &camPosition);
            if (dir.y > -std::numeric_limits<float>::epsilon()) {
                return false;
            }
            const math::vector3f click = camPosition + dir * (-camPosition.y / dir.y);
            const math::vector3f offset = _player->getTransform().v3.xyz - click;
            const float strength = 10.0f * std::max(0.0f, 1.0f - offset.length() / 10.0f);
            _player->addImpulse(offset.normalized(strength), 0.5f);
            return true;
        });
        _keyboardToken = _api.platform->addKeyboardEventHandler([this](const foundation::PlatformKeyboardEventArgs &args) -> bool {
            const float sign = args.type == foundation::PlatformKeyboardEventArgs::EventType::PRESS ? 1.0f : -1.0f;
            switch (args.key) {
                case foundation::Key::W: _direction.z -= sign; break;
                case foundation::Key::S: _direction.z += sign; break;
                case foundation::Key::A: _direction.x -= sign; break;
                case foundation::Key::D: _direction.x += sign; break;
                default: return false;
            }
            return true;
        });
    }

    CollisionContext::~CollisionContext() {
        _api.platform->removeEventHandler(_keyboardToken);
        _api.platform->removeEventHandler(_pointerToken);
    }

    void CollisionContext::update(float dtSec) {
        _player->setVelocity(_direction.normalized(4.0f));
        for (auto &circle : _circles) {
            circle->setVelocity({0, 0, 0});
        }
        _api.scene->setCameraLookAt({0, 20, 0.01f}, {0, 0, 0});
    }
}
