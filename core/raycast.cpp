
#include "raycast.h"
#include <list>
#include <limits>

namespace core {
    class BaseShapeImpl;
};

namespace {
    struct IntersectIntermediateInfo {
        const core::BaseShapeImpl *shape = nullptr;
        float t = std::numeric_limits<float>::max();
        math::vector3f spherePosition;
        int boxAxis;
        float boxSign;
        const math::transform3f *boxTransform;
    };
}

namespace core {
    class BaseShapeImpl : public RaycastInterface::Shape {
    public:
        const std::uint64_t uniqueId;
        const std::uint64_t mask;
        
    public:
        BaseShapeImpl(std::uint64_t id, std::uint64_t m) : uniqueId(id), mask(m) {}
        ~BaseShapeImpl() override {}
        
        const math::bound3f &getBounds() const {
            return _bounds;
        }
        virtual void preCast(const math::vector3f &start, const math::vector3f &dir, float length, IntersectIntermediateInfo &im) const = 0;
        virtual RaycastInterface::RaycastResult completeCast(const math::vector3f &start, const math::vector3f &dir, float length, const IntersectIntermediateInfo &im) const = 0;
        
    protected:
        math::bound3f _bounds;
    };

    class SphereShapeImpl : public BaseShapeImpl {
    public:
        SphereShapeImpl(const core::SceneInterfacePtr &scene, const std::vector<math::vector3f> &points, const std::vector<double> &radiuses, std::uint64_t id, std::uint64_t m)
        : BaseShapeImpl(id, m)
        {
            for (std::size_t i = 0; i < points.size(); i++) {
                Sphere &element = _spheres.emplace_back(Sphere {});
                element.positionAndRadius = math::vector4f(points[i], radiuses[i]);
                element.finalTransform = math::transform3f::identity().translated(element.positionAndRadius.xyz);
                element.visual = scene->addBoundingSphere(points[i], radiuses[i], math::color(1.0f, 0.0f, 1.0f, 0.7f));
            }
        }
        ~SphereShapeImpl() override {
            
        }
        void setTransform(const math::transform3f &trfm) override {
            for (auto &sphere : _spheres) {
                sphere.finalTransform = math::transform3f::identity().translated(sphere.positionAndRadius.xyz) * trfm;
                sphere.visual->setTransform(sphere.finalTransform);
            }
        }
        void preCast(const math::vector3f &start, const math::vector3f &dir, float length, IntersectIntermediateInfo &im) const override {
            for (auto &sphere : _spheres) {
                const math::vector3f spherePosition(sphere.finalTransform.m41, sphere.finalTransform.m42, sphere.finalTransform.m43);
                const math::vector3f L = start - spherePosition;
                const float halfB = L.dot(dir);
                const float c = L.lengthSq() - sphere.positionAndRadius.w * sphere.positionAndRadius.w;
                const float d = halfB * halfB - c;
                if (d < 0.0f) {
                    continue;
                }
                const float sqrtD = std::sqrt(d);
                float t = -halfB - sqrtD;
                if (t < 0.0f) {
                    t = -halfB + sqrtD;
                }
                if (t >= im.t || t > length || t < 0.0f) {
                    continue;
                }
                im.t = t;
                im.shape = this;
                im.spherePosition = spherePosition;
            }
        }
        RaycastInterface::RaycastResult completeCast(const math::vector3f &start, const math::vector3f &dir, float length, const IntersectIntermediateInfo &im) const override {
            RaycastInterface::RaycastResult result;
            result.uniqueId = uniqueId;
            result.point = start + dir * im.t;
            result.normal = (result.point - im.spherePosition).normalized(1.0f);
            return result;
        }
        
    private:
        struct Sphere {
            math::vector4f positionAndRadius;
            math::transform3f finalTransform;
            core::SceneInterface::BoundingSpherePtr visual;
        };
        std::vector<Sphere> _spheres;
    };
}

namespace core {
    class BoxShapeImpl : public BaseShapeImpl {
    public:
        BoxShapeImpl(const core::SceneInterfacePtr &scene, const std::vector<math::vector3f> &points, const std::vector<math::vector3f> &sizes, std::uint64_t id, std::uint64_t m)
        : BaseShapeImpl(id, m)
        {
            for (std::size_t i = 0; i < points.size(); i++) {
                Box &element = _boxes.emplace_back(Box {});
                element.position = points[i];
                const math::vector3f bmin = math::vector3f(-0.5f);
                const math::vector3f bmax = sizes[i] - math::vector3f(0.5f);
                element.bounds = {bmin.x, bmin.y, bmin.z, bmax.x, bmax.y, bmax.z};
                element.finalTransform = math::transform3f::identity().translated(element.position);
                element.visual = scene->addBoundingBox(points[i], element.bounds, math::color(1.0f, 0.0f, 1.0f, 0.7f));
            }
        }
        ~BoxShapeImpl() override {
            
        }
        void setTransform(const math::transform3f &trfm) override {
            for (auto &box : _boxes) {
                box.finalTransform = math::transform3f::identity().translated(box.position) * trfm;
                box.visual->setTransform(box.finalTransform);
            }
        }
        void preCast(const math::vector3f &start, const math::vector3f &dir, float length, IntersectIntermediateInfo &im) const override {
            auto preCastSingle = [](const math::vector3f &localOrigin, const math::vector3f &localDir, float length, const math::bound3f &bb, IntersectIntermediateInfo &im) {
                float tEnter = -std::numeric_limits<float>::max();
                float tExit = std::numeric_limits<float>::max();
                int enterAxis = -1;
                int exitAxis = -1;

                const float minVal[3] = { bb.xmin, bb.ymin, bb.zmin };
                const float maxVal[3] = { bb.xmax, bb.ymax, bb.zmax };

                for (int i = 0; i < 3; ++i) {
                    const float origin = localOrigin.flat[i];
                    const float dir = localDir.flat[i];

                    if (std::fabs(dir) < std::numeric_limits<float>::epsilon()) {
                        if (origin < minVal[i] || origin > maxVal[i]) {
                            return false;
                        }
                        continue;
                    }
                    const float invDir = 1.0f / dir;
                    float t0 = (minVal[i] - origin) * invDir;
                    float t1 = (maxVal[i] - origin) * invDir;
                    if (t0 > t1) {
                        std::swap(t0, t1);
                    }
                    if (t0 > tEnter) {
                        tEnter = t0;
                        enterAxis = i;
                    }
                    if (t1 < tExit) {
                        tExit = t1;
                        exitAxis = i;
                    }
                }

                if (tEnter > tExit || tExit < 0.0f || tEnter > length) {
                    return false;
                }
                const float t = (tEnter >= 0.0f) ? tEnter : tExit;
                if (t >= im.t || t < 0.0f || t > length) {
                    return false;
                }
                im.t = t;
                im.boxAxis = (tEnter >= 0.0f) ? enterAxis : exitAxis;
                im.boxSign = (tEnter >= 0.0f ? (localDir.flat[im.boxAxis] < 0.0f) : (localDir.flat[im.boxAxis] > 0.0f)) ? 1.0f : -1.0f;
                return true;
            };
            
            for (auto &box : _boxes) {
                const math::vector3f delta(start.x - box.finalTransform.m41, start.y - box.finalTransform.m42, start.z - box.finalTransform.m43);
                const math::vector3f localOrigin(
                    delta.x * box.finalTransform.m11 + delta.y * box.finalTransform.m12 + delta.z * box.finalTransform.m13,
                    delta.x * box.finalTransform.m21 + delta.y * box.finalTransform.m22 + delta.z * box.finalTransform.m23,
                    delta.x * box.finalTransform.m31 + delta.y * box.finalTransform.m32 + delta.z * box.finalTransform.m33
                );
                const math::vector3f localDir(
                    dir.x * box.finalTransform.m11 + dir.y * box.finalTransform.m12 + dir.z * box.finalTransform.m13,
                    dir.x * box.finalTransform.m21 + dir.y * box.finalTransform.m22 + dir.z * box.finalTransform.m23,
                    dir.x * box.finalTransform.m31 + dir.y * box.finalTransform.m32 + dir.z * box.finalTransform.m33
                );
                if (preCastSingle(localOrigin, localDir, length, box.bounds, im)) {
                    im.shape = this;
                    im.boxTransform = &box.finalTransform;
                }
            }
        }
        RaycastInterface::RaycastResult completeCast(const math::vector3f &start, const math::vector3f &dir, float length, const IntersectIntermediateInfo &im) const override {
            RaycastInterface::RaycastResult result;
            const float nx = im.boxSign * (im.boxAxis == 0 ? im.boxTransform->m11 : (im.boxAxis == 1 ? im.boxTransform->m21 : im.boxTransform->m31));
            const float ny = im.boxSign * (im.boxAxis == 0 ? im.boxTransform->m12 : (im.boxAxis == 1 ? im.boxTransform->m22 : im.boxTransform->m32));
            const float nz = im.boxSign * (im.boxAxis == 0 ? im.boxTransform->m13 : (im.boxAxis == 1 ? im.boxTransform->m23 : im.boxTransform->m33));
            result.normal = math::vector3f(nx, ny, nz).normalized(1.0f);
            result.point = start + dir * im.t;
            result.uniqueId = uniqueId;
            return result;
        }
        
    private:
        struct Box {
            math::vector3f position;
            math::bound3f bounds;
            math::transform3f finalTransform;
            core::SceneInterface::BoundingBoxPtr visual;
        };
        std::vector<Box> _boxes;
    };
}

namespace core {
    class RaycastInterfaceImpl : public RaycastInterface {
    public:
        RaycastInterfaceImpl(const foundation::PlatformInterfacePtr &platform, const core::SceneInterfacePtr &scene) : _platform(platform), _scene(scene) {}
        ~RaycastInterfaceImpl() override {}
        
    public:
        auto addShape(const util::Description &desc, std::uint64_t uniqueId, std::uint64_t mask) -> ShapePtr override {
            const core::RaycastInterface::ShapeType shapeType = static_cast<core::RaycastInterface::ShapeType>(desc.getInteger("type", 0));
            const std::vector<math::vector3f> points = desc.getVector3fs("points");
            ShapePtr result;
            
            if (shapeType == core::RaycastInterface::ShapeType::SPHERES) {
                const std::vector<double> radiuses = desc.getNumbers("radiuses");
                result = _shapes.emplace_back(std::make_shared<SphereShapeImpl>(_scene, points, radiuses, uniqueId, mask));
            }
            else if (shapeType == core::RaycastInterface::ShapeType::BOXES) {
                const std::vector<math::vector3f> sizes = desc.getVector3fs("sizes");
                result = _shapes.emplace_back(std::make_shared<BoxShapeImpl>(_scene, points, sizes, uniqueId, mask));
            }
            else {
                _platform->logError("[RaycastInterfaceImpl::addShape] Unknown raycast shape type");
            }
            return result;
        }
        auto rayCast(const math::vector3f &start, const math::vector3f &dir, float length, std::uint64_t mask) const -> RaycastResult override {
            RaycastResult result;
            IntersectIntermediateInfo intermediate;
            for (auto &shape : _shapes) {
                if (shape->mask & mask) {
                    shape->preCast(start, dir, length, intermediate);
                }
            }
            if (intermediate.shape) {
                result = intermediate.shape->completeCast(start, dir, length, intermediate);
            }
            return result;
        }
        void update(float dtSec) override {
            util::cleanupUnused(_shapes);
        }
        
    private:
        const foundation::PlatformInterfacePtr _platform;
        const core::SceneInterfacePtr _scene;
        
        std::vector<std::shared_ptr<BaseShapeImpl>> _shapes;
    };
}

namespace core {
    std::shared_ptr<RaycastInterface> RaycastInterface::instance(const foundation::PlatformInterfacePtr &platform, const core::SceneInterfacePtr &scene) {
        return std::make_shared<RaycastInterfaceImpl>(platform, scene);
    }
}

