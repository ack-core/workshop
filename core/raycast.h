
#pragma once
#include <memory>
#include "foundation/math.h"
#include "foundation/platform.h"
#include "foundation/util.h"
#include "scene.h"

namespace core {
    class RaycastInterface {
    public:
        enum class ShapeType {
            SPHERES = 1,
            BOXES = 2
        };
        struct RaycastResult {
            bool intersected;
            math::vector3f point;
            math::vector3f normal;
        };
        
    public:
        static std::shared_ptr<RaycastInterface> instance(const foundation::PlatformInterfacePtr &platform, const core::SceneInterfacePtr &scene);

    public:
        struct Shape {
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual ~Shape() = default;
        };
        
        using ShapePtr = std::shared_ptr<Shape>;
        
    public:
        virtual auto addShape(const util::Description &desc) -> ShapePtr = 0;
        virtual auto rayCast(const math::vector3f &start, const math::vector3f &target, float length) const -> RaycastResult = 0;
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~RaycastInterface() = default;
    };
    
    using RaycastInterfacePtr = std::shared_ptr<RaycastInterface>;
}

