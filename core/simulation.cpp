
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
        SimulationInterface::ShapeType getType() const override {
            return SimulationInterface::ShapeType::CircleXZ;
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
            _lastDt = 1.0f;
        }
        const math::vector3f getVelocity() const override {
            return (transform.v3.xyz - _prevpos) / _lastDt;
        }
        void setVelocity(const math::vector3f &v) override {
            _prevpos = transform.v3.xyz - math::vector3f(v.x, 0.0f, v.z);
            _lastDt = 1.0f;
        }
        void addImpulse(const math::vector3f &impulse, float timeSec) override {
            _impulse = _impulse + math::vector3f(impulse.x, 0.0f, impulse.z);
            _impulseFade = std::log(100.0f) / std::max(timeSec, std::numeric_limits<float>::epsilon());
        }
        void update(float dtSec, const math::vector3f &gravity) {
            const math::vector3f v = getVelocity() + gravity * dtSec;
            const math::vector3f impulseOffset = _impulse * dtSec;
            _prevpos = transform.v3.xyz + impulseOffset;
            _lastDt = std::max(dtSec, std::numeric_limits<float>::epsilon());
            transform.v3.xyz = transform.v3.xyz + math::vector3f(v.x, 0.0f, v.z) * dtSec + impulseOffset;
            _impulse = _impulse.lengthSq() > std::numeric_limits<float>::epsilon() ? _impulse * std::exp(-_impulseFade * dtSec) : math::vector3f(0.0f, 0.0f, 0.0f);
        }
        void updateVisual() {
            _visual->setPosition(transform.v3.xyz);
        }

    private:
        float _lastDt = 1.0f;
        float _impulseFade = 0.0f;
        math::vector3f _impulse = {0.0f, 0.0f, 0.0f};
        math::vector3f _prevpos = {0.0f, 0.0f, 0.0f};
        core::SceneInterface::LineSetPtr _visual;
    };
}

namespace core {
    class ObstaclePolygonXZImpl : public CollisionBodyBase {
    public:
        std::vector<math::vector3f> points;
        
    public:
        ObstaclePolygonXZImpl(const core::SceneInterfacePtr &scene, std::vector<math::vector3f> &&src) : points(src), _src(std::move(src)) {
            _visual = scene->addLineSet();
            _visual->fillAsClosedPolygon(_src, {0.0f, 1.0f, 1.0f, 0.7f});
        }
        ~ObstaclePolygonXZImpl() override {}
        
        void setEnabled(bool value) override {
            enabled = value;
        }
        void setMovable(bool value) override {}
        SimulationInterface::ShapeType getType() const override {
            return SimulationInterface::ShapeType::ObstaclePolygonXZ;
        }
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
            points.clear();
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
        void addImpulse(const math::vector3f &impulse, float timeSec) override {}

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
        const math::vector3f push = info.normal * info.penetration;
        if (a.movable && b.movable) {
            const float invMassSumm = a.invMass + b.invMass;
            const float aShare = invMassSumm > 0.0f ? a.invMass / invMassSumm : 0.5f;
            a.transform.v3.xyz = a.transform.v3.xyz - push * aShare;
            b.transform.v3.xyz = b.transform.v3.xyz + push * (1.0f - aShare);
        }
        else if (a.movable) {
            a.transform.v3.xyz = a.transform.v3.xyz - push;
        }
        else if (b.movable) {
            b.transform.v3.xyz = b.transform.v3.xyz + push;
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
        if (obj.movable) {
            obj.transform.v3.xyz = obj.transform.v3.xyz + info.normal * info.penetration;
        }
    }

}

namespace core {
    class DistanceConstraintImpl : public SimulationInterface::Constraint {
    public:
        const std::weak_ptr<CircleXZImpl> a;
        const std::weak_ptr<CollisionBodyBase> b;
        const float distance;

    public:
        DistanceConstraintImpl(const std::shared_ptr<CircleXZImpl> &a, const std::shared_ptr<CollisionBodyBase> &b, float distance) : a(a), b(b), distance(distance) {}
        ~DistanceConstraintImpl() override {}

        bool isExpired() const {
            return a.expired() || b.expired();
        }
        void resolve() {
            const std::shared_ptr<CircleXZImpl> circle = a.lock();
            const std::shared_ptr<CollisionBodyBase> other = b.lock();

            if (circle->enabled && other->enabled) {
                const math::vector3f d = other->getTransform().v3.xyz - circle->transform.v3.xyz;
                const float current = d.xz.length();

                if (current > std::numeric_limits<float>::epsilon()) {
                    CollisionInfo info;
                    info.normal = math::vector3f(d.x / current, 0.0f, d.z / current);
                    info.penetration = distance - current;

                    if (other->getType() == SimulationInterface::ShapeType::CircleXZ) {
                        resolveCollisionCircleCircleXZ(info, *circle, static_cast<CircleXZImpl &>(*other));
                    }
                    else if (circle->movable) {
                        circle->transform.v3.xyz -= info.normal * info.penetration;
                    }
                }
            }
        }
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
                const float mass = float(desc.getNumber("mass", 0.0));
                const float radius = float(desc.getNumber("radius", 1.0));
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
        auto addConstraint(const BodyPtr &a, const BodyPtr &b, float distance) -> ConstraintPtr override {
            if (a == nullptr || b == nullptr || a == b || (a->getType() != ShapeType::CircleXZ && b->getType() != ShapeType::CircleXZ)) {
                _platform->logError("[SimulationInterfaceImpl::addConstraint] Invalid bodies");
                return nullptr;
            }
            if (a->getType() != ShapeType::CircleXZ) {
                return addConstraint(b, a, distance);
            }
            return _constraints.emplace_back(std::make_shared<DistanceConstraintImpl>(std::static_pointer_cast<CircleXZImpl>(a), std::static_pointer_cast<CollisionBodyBase>(b), distance));
        }
        void setGravity(const math::vector3f &gravity) override {
            _gravity = gravity;
        }
        void update(float dtSec) override {
            util::cleanupUnused(_circlesXZ);
            util::cleanupUnused(_obstaclesXZ);
            util::cleanupUnused(_constraints);
            std::erase_if(_constraints, [](const std::shared_ptr<DistanceConstraintImpl> &constraint) { return constraint->isExpired(); });

            for (auto &obj : _circlesXZ) {
                if (obj->enabled && obj->movable) {
                    obj->update(dtSec, _gravity);
                }
            }

            for (auto &constraint : _constraints) {
                constraint->resolve();
            }

            CollisionInfo info;
            for (std::size_t i = 0; i < _circlesXZ.size(); i++) {
                if (_circlesXZ[i]->enabled == false) {
                    continue;
                }
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

            for (auto &obj : _circlesXZ) {
                if (obj->enabled) {
                    obj->updateVisual();
                }
            }
        }

    private:
        const foundation::PlatformInterfacePtr _platform;
        const core::SceneInterfacePtr _scene;

        math::vector3f _gravity = {0.0f, 0.0f, 0.0f};
        std::vector<std::shared_ptr<CircleXZImpl>> _circlesXZ;
        std::vector<std::shared_ptr<ObstaclePolygonXZImpl>> _obstaclesXZ;
        std::vector<std::shared_ptr<DistanceConstraintImpl>> _constraints;
    };
}

namespace core {
    std::shared_ptr<SimulationInterface> SimulationInterface::instance(const foundation::PlatformInterfacePtr &platform, const core::SceneInterfacePtr &scene) {
        return std::make_shared<SimulationInterfaceImpl>(platform, scene);
    }
}

