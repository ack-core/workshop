
#include "simulation.h"
#include <vector>

namespace core {
    class CollisionBodyBase : public SimulationInterface::Body {
    public:
        bool enabled = true;
        bool movable = false;
        
    public:
        ~CollisionBodyBase() override {}
    };
}

namespace core {
    class CircleXZImpl : public CollisionBodyBase {
    public:
        const float invMass;
        const float radius;
        math::transform3f transform;

    public:
        CircleXZImpl(const core::SceneInterfacePtr &scene, float m, float r) : invMass(m >= 1.0f ? 1.0f / m : 0.0f), radius(r), transform(math::transform3f::identity()) {
            movable = true;
            _visual = scene->addLineSet();
            _visual->fillAsCircle(24, radius, {0.0f, 1.0f, 1.0f, 0.7f});
        }
        ~CircleXZImpl() override {}
        
        void setEnabled(bool value) override {
            enabled = value;
        }
        void setMovable(bool value) override {
            movable = value;
        }
        float getRadius() const override {
            return radius;
        }
        const math::transform3f getTransform() const override {
            return transform;
        }
        void setTransform(const math::transform3f &trfm) override {
            transform = trfm;
            _prevpos = transform.v3.xyz;
        }
        const math::vector3f getVelocity() const override {
            return transform.v3.xyz - _prevpos;
        }
        void setVelocity(const math::vector3f &v) override {
            _prevpos = transform.v3.xyz - math::vector3f(v.x, 0.0f, v.z);
        }
        void update(float dtSec) {
            const math::vector3f v = math::vector3f(transform.v3.x - _prevpos.x, 0.0f, transform.v3.z - _prevpos.z);
            _prevpos = transform.v3.xyz;
            transform.v3.xyz = transform.v3.xyz + v * (dtSec / _prevDt);
            _visual->setPosition(transform.v3.xyz);
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
            _visual->fillAsСlosedPolygon(_src, {0.0f, 1.0f, 1.0f, 0.7f});
        }
        ~ObstaclePolygonXZImpl() override {}
        
        void setEnabled(bool value) override {
            enabled = value;
        }
        void setMovable(bool value) override {}
        float getRadius() const override {
            return 0.0f;
        }
        const math::transform3f getTransform() const override {
            return _transform;
        }
        void setTransform(const math::transform3f &trfm) override {
            const math::vector3f translation = math::vector3f(trfm.m41, trfm.m42, trfm.m43);
            const float yaw = std::atan2(trfm.m31, trfm.m33);
            math::transform3f ctransform = math::transform3f({0, 1, 0}, -yaw).translated(translation);
            for (auto &point : _src) {
                points.emplace_back(point.transformed(ctransform, true));
            }
            _transform = trfm;
            _visual->setTransform(ctransform);
        }
        const math::vector3f getVelocity() const override {
            return {};
        }
        void setVelocity(const math::vector3f &v) override {}

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
        const math::vector3f d = b.transform.v3.xyz - a.transform.v3.xyz;
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
        if (a.movable) {
            a.transform.v3.xyz = a.transform.v3.xyz - info.normal * info.penetration * (a.invMass / invMassSumm);
        }
        if (b.movable) {
            b.transform.v3.xyz = b.transform.v3.xyz + info.normal * info.penetration * (b.invMass / invMassSumm);
        }
    }

    bool checkCollisionCircleObstacleXZ(const CircleXZImpl &obj, const ObstaclePolygonXZImpl &obstacle, CollisionInfo &info) {
        float minDistSq = std::numeric_limits<float>::max();
        bool isInside = false;
        math::vector3f closestPoint;
        const math::vector3f objpos = obj.transform.v3.xyz;

        for (std::size_t i = 0; i < obstacle.points.size(); i++) {
            const math::vector3f &a = obstacle.points[i];
            const math::vector3f &b = obstacle.points[(i + 1) % obstacle.points.size()];
            const math::vector3f edge = b - a;
            const math::vector3f toObj = objpos - a;
            const float t = std::max(0.0f, std::min(1.0f, (toObj.x * edge.x + toObj.z * edge.z) / edge.xz.lengthSq()));
            const math::vector3f pointOnEdge = math::vector3f(a.x + t * edge.x, 0.0f, a.z + t * edge.z);
            const float distSq = (objpos.xz - pointOnEdge.xz).lengthSq();
            if (minDistSq > distSq) {
                minDistSq = distSq;
                closestPoint = pointOnEdge;
            }
            if ((a.z > objpos.z) != (b.z > objpos.z) && (objpos.x < edge.x * (objpos.z - a.z) / edge.z + a.x)) {
                isInside = !isInside;
            }
        }
        
        if (minDistSq < std::numeric_limits<float>::max()) {
            const float distance = std::sqrtf(minDistSq);
            if (isInside || distance < obj.radius) {
                const float sign = isInside ? -1.0f : 1.0f;
                info.penetration = isInside ? distance + obj.radius : obj.radius - distance;
                info.normal.x = sign * (objpos.x - closestPoint.x) / distance;
                info.normal.z = sign * (objpos.z - closestPoint.z) / distance;
                return true;
            }
        }
        
        return false;
    }
    void resolveCollisionCircleObstacleXZ(const CollisionInfo &info, CircleXZImpl &obj, ObstaclePolygonXZImpl &obstacle) {
        obj.transform.v3.xyz = obj.transform.v3.xyz + info.normal * info.penetration;
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
                if (obj->enabled && obj->movable) {
                    obj->update(dtSec);
                }
            }
            
            CollisionInfo info;
            for (std::size_t i = 0; i < _circlesXZ.size(); i++) {
                for (std::size_t c = i + 1; c < _circlesXZ.size(); c++) {
                    if (_circlesXZ[c]->enabled) {
                        if (checkCollisionCircleCircleXZ(*_circlesXZ[i], *_circlesXZ[c], info)) {
                            resolveCollisionCircleCircleXZ(info, *_circlesXZ[i], *_circlesXZ[c]);
                        }
                    }
                }
                for (std::size_t c = 0; c < _obstaclesXZ.size(); c++) {
                    if (_obstaclesXZ[c]->enabled) {
                        if (checkCollisionCircleObstacleXZ(*_circlesXZ[i], *_obstaclesXZ[c], info)) {
                            resolveCollisionCircleObstacleXZ(info, *_circlesXZ[i], *_obstaclesXZ[c]);
                        }
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

