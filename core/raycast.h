
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
            BOXES,
            TRIANGLES,
            LANDSCAPE
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
        virtual bool rayCast(const math::vector3f &start, const math::vector3f &target, float length) const = 0;
        virtual bool sphereCast(const math::vector3f &start, const math::vector3f &target, float radius, float length) const = 0;
        
    public:
        virtual ~RaycastInterface() = default;
    };
    
    using RaycastInterfacePtr = std::shared_ptr<RaycastInterface>;
}

