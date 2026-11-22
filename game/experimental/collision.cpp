#include "collision.h"

namespace game {
    CollisionContext::CollisionContext(API &&api) : _api(std::move(api)) {
        util::Description playerDesc;
        playerDesc.setInteger("type", int(core::SimulationInterface::ShapeType::CircleXZ));
        playerDesc.setNumber("mass", 1.0);
        playerDesc.setNumber("radius", 0.5);
        _player = _api.simulation->addBody(playerDesc);

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

        _token = _api.platform->addKeyboardEventHandler([this](const foundation::PlatformKeyboardEventArgs &args) -> bool {
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
        _api.platform->removeEventHandler(_token);
    }

    void CollisionContext::update(float dtSec) {
        _player->setVelocity(_direction.normalized(5.0f));
        _api.scene->setCameraLookAt({0, 20, 0.01f}, {0, 0, 0});
    }
}
