
#include "simulation.h"
#include <vector>

namespace core {
    class CircleXZImpl : public SimulationInterface::Body {
    public:
        CircleXZImpl(const core::SceneInterfacePtr &scene, float mass, float radius) : _mass(mass), _radius(radius) {
            _position = {0, 0, 0};
            _visual = scene->addLineSet();
            SceneInterface::fillLineSetAsCircle(_visual, 24, radius, {0.0f, 1.0f, 1.0f, 0.7f});
        }
        ~CircleXZImpl() override {
            
        }
        void setTransform(const math::transform3f &trfm) override {
            _position = math::vector3f(trfm.m41, 0.0f, trfm.m43);
            _visual->setPosition(_position);
        }
        void setVelocity(const math::vector3f &v) override {
            
        }
        
    private:
        const float _mass;
        const float _radius;
        math::vector3f _position;
        core::SceneInterface::LineSetPtr _visual;
    };
}
 
namespace core {
    class ObstaclePolygonXZImpl : public SimulationInterface::Body {
    public:
        ObstaclePolygonXZImpl(const core::SceneInterfacePtr &scene, std::vector<math::vector3f> &&points) : _src(std::move(points)) {
            _visual = scene->addLineSet();
            SceneInterface::fillLineSetAsСlosedСircuit(_visual, _src, {0.0f, 1.0f, 1.0f, 0.7f});
        }
        ~ObstaclePolygonXZImpl() override {
            
        }
        void setTransform(const math::transform3f &trfm) override {
            _transform = trfm;
            _visual->setTransform(trfm);
        }
        void setVelocity(const math::vector3f &v) override {
            
        }

    private:
        math::transform3f _transform;
        std::vector<math::vector3f> _src;
        std::vector<math::vector3f> _points;
        core::SceneInterface::LineSetPtr _visual;
    };
}

namespace core {
    class SimulationInterfaceImpl : public SimulationInterface {
    public:
        SimulationInterfaceImpl(const foundation::PlatformInterfacePtr &platform, const core::SceneInterfacePtr &scene) : _platform(platform), _scene(scene) {}
        ~SimulationInterfaceImpl() override {}
        
    public:
        auto addBody(const util::Description &desc) -> BodyPtr override {
            const core::SimulationInterface::ShapeType shapeType = static_cast<core::SimulationInterface::ShapeType>(desc.getInteger("type", 0));
            BodyPtr result;
            
            if (shapeType == core::SimulationInterface::ShapeType::CircleXZ) {
                const float mass = desc.getNumber("mass", 0.0f);
                const float radius = desc.getNumber("radius", 1.0f);
                result = _circlesXZ.emplace_back(std::make_shared<CircleXZImpl>(_scene, mass, radius));
            }
            else if (shapeType == core::SimulationInterface::ShapeType::ObstaclePolygonXZ) {
                std::vector<math::vector3f> points = desc.getVector3fs("points");
                result = _obstaclesXZ.emplace_back(std::make_shared<ObstaclePolygonXZImpl>(_scene, std::move(points)));
            }
            else {
                _platform->logError("[SimulationInterfaceImpl::addBody] Unknown collision shape type");
            }
            return result;
        }
        void update(float dtSec) override {
            util::cleanupUnused(_circlesXZ);
            util::cleanupUnused(_obstaclesXZ);
        }

    private:
        const foundation::PlatformInterfacePtr _platform;
        const core::SceneInterfacePtr _scene;
        
        std::vector<std::shared_ptr<CircleXZImpl>> _circlesXZ;
        std::vector<std::shared_ptr<ObstaclePolygonXZImpl>> _obstaclesXZ;
    };
}

namespace core {
    std::shared_ptr<SimulationInterface> SimulationInterface::instance(const foundation::PlatformInterfacePtr &platform, const core::SceneInterfacePtr &scene) {
        return std::make_shared<SimulationInterfaceImpl>(platform, scene);
    }
}

