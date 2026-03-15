
#pragma once
#include <memory>
#include "foundation/math.h"
#include "foundation/platform.h"
#include "foundation/util.h"
#include "scene.h"

namespace core {
    class RaycastInterface {
    public:
        static const std::uint64_t INVALID_UNIQUE_ID = std::uint64_t(-1);
        static const std::uint64_t MASK_ALL = std::uint64_t(-1);
        
        enum class ShapeType {
            SPHERES = 1,
            BOXES = 2
        };
        struct RaycastResult {
            std::uint64_t uniqueId = INVALID_UNIQUE_ID;
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
        virtual auto addShape(const util::Description &desc, std::uint64_t uniqueId, std::uint64_t mask = MASK_ALL) -> ShapePtr = 0;
        
        // Check the ray for intersections
        // @start   - ray start position
        // @dir     - ray direction. Must be normalized
        // @length  - max length
        //
        virtual auto rayCast(const math::vector3f &start, const math::vector3f &dir, float length, std::uint64_t mask = MASK_ALL) const -> RaycastResult = 0;
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~RaycastInterface() = default;
    };
    
    using RaycastInterfacePtr = std::shared_ptr<RaycastInterface>;
}

