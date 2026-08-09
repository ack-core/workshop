
#pragma once
#include <memory>
#include "foundation/math.h"
#include "foundation/platform.h"
#include "scene.h"

namespace core {
    class SimulationInterface {
    public:
        enum class ShapeType {
            CircleXZ = 1,
            ObstaclePolygonXZ,
        };

    public:
        static std::shared_ptr<SimulationInterface> instance(const foundation::PlatformInterfacePtr &platform, const core::SceneInterfacePtr &scene);
        
    public:
        struct Body {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setMovable(bool movable) = 0;
            virtual auto getRadius() const -> float = 0;
            virtual auto getTransform() const -> const math::transform3f = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual auto getVelocity() const -> const math::vector3f = 0;
            virtual void setVelocity(const math::vector3f &v) = 0;
            virtual ~Body() = default;
        };
        
        using BodyPtr = std::shared_ptr<Body>;

    public:
        virtual auto addBody(const util::Description &desc) -> BodyPtr = 0;
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~SimulationInterface() = default;
    };
    
    using SimulationInterfacePtr = std::shared_ptr<SimulationInterface>;
}

