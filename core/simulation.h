
#pragma once
#include <memory>
#include "foundation/math.h"
#include "foundation/platform.h"
#include "scene.h"

namespace core {
    // Interface provides 2D physics simulation on XZ plane. Simulation keeps bodies alive while user holds their pointers.
    // Every body has a debug visual, drawn if scene lines drawing enabled
    //
    class SimulationInterface {
    public:
        enum class ShapeType {
            CircleXZ = 1,           // dynamic circle, collides with circles and obstacles
            ObstaclePolygonXZ,      // static closed polygon, pushes circles out
        };

    public:
        static std::shared_ptr<SimulationInterface> instance(const foundation::PlatformInterfacePtr &platform, const core::SceneInterfacePtr &scene);
        
    public:
        struct Body {
            // Disabled body doesn't move and doesn't collide
            //
            virtual void setEnabled(bool enabled) = 0;

            // Unmovable body stays in place but still pushes others. No effect on obstacles
            //
            virtual void setMovable(bool movable) = 0;

            virtual auto getRadius() const -> float = 0;
            virtual auto getTransform() const -> const math::transform3f = 0;

            // Teleport body and reset its velocity. Obstacles take translation and Y-rotation only
            //
            virtual void setTransform(const math::transform3f &trfm) = 0;

            // @return velocity in units per second, derived from the last position change. Zero for obstacles
            //
            virtual auto getVelocity() const -> const math::vector3f = 0;

            // Set velocity in units per second, Y is ignored. No effect on obstacles
            //
            virtual void setVelocity(const math::vector3f &v) = 0;

            // Add extra velocity that fades exponentially, Y is ignored. No effect on obstacles
            // @impulse     - initial velocity in units per second
            // @timeSec     - time to fade down to 1% of the initial value
            //
            virtual void addImpulse(const math::vector3f &impulse, float timeSec) = 0;

            virtual ~Body() = default;
        };
        
        using BodyPtr = std::shared_ptr<Body>;

    public:
        // Create body. Body starts with identity transform, enabled and with zero velocity
        // @desc - body description:
        //         type   : integer, ShapeType value
        //         CircleXZ:
        //           radius : number, default 1.0
        //           mass   : number, default 0.0. Defines share of collision response between circles.
        //                    Mass < 1.0 means infinite mass: the circle is not pushed by finite-mass circles.
        //                    Two infinite-mass circles push each other equally
        //         ObstaclePolygonXZ:
        //           points : vector3f, repeated, polygon vertices in local space (Y is ignored)
        // @return nullptr if type is unknown
        //
        virtual auto addBody(const util::Description &desc) -> BodyPtr = 0;

        // Set global gravity applied to enabled movable bodies regardless of mass. Default is {0, 0, 0}
        // @gravity - direction of gravity, length is velocity gained per second. Y is ignored by XZ bodies
        //
        virtual void setGravity(const math::vector3f &gravity) = 0;

        // Move bodies and resolve collisions
        //
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~SimulationInterface() = default;
    };
    
    using SimulationInterfacePtr = std::shared_ptr<SimulationInterface>;
}

