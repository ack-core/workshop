
#include "simulation.h"
#include <vector>

namespace core {
    class CollisionBodyBase : public SimulationInterface::Body {
    public:
        ~CollisionBodyBase() override {}
        virtual void update(float dtSec) = 0;
    };
}

namespace core {
    class CircleXZImpl : public CollisionBodyBase {
    public:
        const float invMass;
        const float radius;
        math::vector3f position;

    public:
        CircleXZImpl(const core::SceneInterfacePtr &scene, float m, float r) : invMass(m >= 1.0f ? 1.0f / m : 0.0f), radius(r), position(0, 0, 0) {
            _visual = scene->addLineSet();
            SceneInterface::fillLineSetAsCircle(_visual, 24, radius, {0.0f, 1.0f, 1.0f, 0.7f});
        }
        ~CircleXZImpl() override {}
        
        const math::transform3f getTransform() const override {
            math::transform3f result = math::transform3f::identity();
            result.m41 = position.x;
            result.m43 = position.z;
            return result;
        }
        void setTransform(const math::transform3f &trfm) override {
            _prevpos = position = math::vector3f(trfm.m41, trfm.m42, trfm.m43);
            _visual->setPosition(position);
        }
        void setVelocity(const math::vector3f &v) override {
            _prevpos = position - math::vector3f(v.x, 0.0f, v.z);
        }
        void update(float dtSec) override {
            const math::vector3f v = math::vector3f(position.x - _prevpos.x, 0.0f, position.z - _prevpos.z);
            _prevpos = position;
            position = position + v * (dtSec / _prevDt);
            _visual->setPosition(position);
        }
        
    private:
        float _prevDt = 0.033f;
        math::vector3f _prevpos;
        core::SceneInterface::LineSetPtr _visual;
    };
}
 
namespace core {
    class ObstaclePolygonXZImpl : public CollisionBodyBase {
    public:
        std::vector<math::vector3f> points;
        
    public:
        ObstaclePolygonXZImpl(const core::SceneInterfacePtr &scene, std::vector<math::vector3f> &&points) : _src(std::move(points)) {
            for (auto &point : _src) {
                points.emplace_back(point);
            }
            _visual = scene->addLineSet();
            SceneInterface::fillLineSetAsСlosedСircuit(_visual, _src, {0.0f, 1.0f, 1.0f, 0.7f});
        }
        ~ObstaclePolygonXZImpl() override {
            
        }
        
        const math::transform3f getTransform() const override {
            return _transform;
        }
        void setTransform(const math::transform3f &trfm) override {
            const math::vector3f translation = math::vector3f(trfm.m41, trfm.m42, trfm.m43);
            const float yaw = std::atan2(trfm.m31, trfm.m33);
            _transform = math::transform3f({0, 1, 0}, -yaw).translated(translation);
            for (auto &point : _src) {
                points.emplace_back(point.transformed(_transform, true));
            }
            _visual->setTransform(_transform);
        }
        void setVelocity(const math::vector3f &v) override {}
        void update(float dtSec) override {}

    private:
        math::transform3f _transform;
        std::vector<math::vector3f> _src;
        core::SceneInterface::LineSetPtr _visual;
    };
}

namespace core {
    struct CollisionInfo {
        math::vector3f normal = {0, 0, 0};
        float penetration = -1.0f;
    };
    
    bool checkCollisionCircleCircleXZ(const CircleXZImpl &a, const CircleXZImpl &b, CollisionInfo &info) {
        const math::vector3f d = b.position - a.position;
        const float distSq = d.x * d.x + d.z * d.z;
        const float minDist = a.radius + b.radius;
        const float minDistSq = minDist * minDist;
        
        if (distSq < minDistSq && distSq > std::numeric_limits<float>::epsilon()) {
            const float distance = std::sqrtf(distSq);
            info.penetration = minDist - distance;
            info.normal.x = d.x / distance;
            info.normal.z = d.z / distance;
            return true;
        }
        return false;
    }
    void resolveCollisionCircleCircleXZ(const CollisionInfo &info, CircleXZImpl &a, CircleXZImpl &b) {
        const float invMassSumm = a.invMass + b.invMass;
        a.position = a.position - info.normal * info.penetration * (a.invMass / invMassSumm);
        b.position = b.position + info.normal * info.penetration * (b.invMass / invMassSumm);
    }

    bool checkCollisionCircleObstacleXZ(const CircleXZImpl &obj, const ObstaclePolygonXZImpl &obstacle, CollisionInfo &info) {
        float minDistSq = std::numeric_limits<float>::max();
        bool isInside = false;
        math::vector3f closestPoint;
        
        for (std::size_t i = 0; i < obstacle.points.size(); i++) {
            const math::vector3f &a = obstacle.points[i];
            const math::vector3f &b = obstacle.points[(i + 1) % obstacle.points.size()];
            const math::vector3f edge = b - a;
            const math::vector3f toObj = obj.position - a;
            const float t = std::max(0.0f, std::min(1.0f, (toObj.x * edge.x + toObj.z * edge.z) / edge.xz.lengthSq()));
            const math::vector3f pointOnEdge = math::vector3f(a.x + t * edge.x, 0.0f, a.z + t * edge.z);
            const float distSq = (obj.position.xz - pointOnEdge.xz).lengthSq();
            if (minDistSq > distSq) {
                minDistSq = distSq;
                closestPoint = pointOnEdge;
            }
            if ((a.z > obj.position.z) != (b.z > obj.position.z) && (obj.position.x < edge.x * (obj.position.z - a.z) / edge.z + a.x)) {
                isInside = !isInside;
            }
        }
        
        if (minDistSq < std::numeric_limits<float>::max()) {
            const float distance = std::sqrtf(minDistSq);
            if (isInside || distance < obj.radius) {
                info.penetration = isInside ? distance + obj.radius : obj.radius - distance;
                info.normal.x = (obj.position.x - closestPoint.x) / distance;
                info.normal.z = (obj.position.z - closestPoint.z) / distance;
                return true;
            }
        }
        
        return false;
    }
    void resolveCollisionCircleObstacleXZ(const CollisionInfo &info, CircleXZImpl &obj, ObstaclePolygonXZImpl &obstacle) {
        obj.position = obj.position + info.normal * info.penetration;
    }

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
            
            for (auto &obj : _circlesXZ) {
                obj->update(dtSec);
            }
            
            CollisionInfo info;
            for (std::size_t i = 0; i < _circlesXZ.size(); i++) {
                for (std::size_t c = i + 1; c < _circlesXZ.size(); c++) {
                    if (checkCollisionCircleCircleXZ(*_circlesXZ[i], *_circlesXZ[c], info)) {
                        resolveCollisionCircleCircleXZ(info, *_circlesXZ[i], *_circlesXZ[c]);
                    }
                }
                for (std::size_t c = 0; c < _obstaclesXZ.size(); c++) {
                    if (checkCollisionCircleObstacleXZ(*_circlesXZ[i], *_obstaclesXZ[c], info)) {
                        resolveCollisionCircleObstacleXZ(info, *_circlesXZ[i], *_obstaclesXZ[c]);
                    }
                }
            }
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

