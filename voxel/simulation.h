
#pragma once
#include "foundation/util.h"
#include "foundation/math.h"

#include <memory>

namespace voxel {
    class SimulationInterface {
    public:
        static std::shared_ptr<SimulationInterface> instance();
        
    public:
        enum class EntityType {
            ACTOR,
            FENCE
        };
        struct Actor {
            virtual void setImpulse(const math::vector3f &impulse) = 0;
            virtual void setImpactHandler(util::callback<void(const math::vector3f &point, const math::vector3f &impulse, EntityType type)> &&handler) = 0;
            virtual auto getPosition() const -> const math::vector3f & = 0;
            virtual ~Actor() = default;
        };
        struct Fence {
            virtual void addSegment(const math::vector3f &pStart, const math::vector3f &pEnd) = 0;
            virtual void removeAllSegments() = 0;
            virtual ~Fence() = default;
        };
        
        using ActorPtr = std::shared_ptr<Actor>;
        using FencePtr = std::shared_ptr<Fence>;
        
    public:
        virtual auto addActor(const math::vector3f &position, float radius) -> ActorPtr = 0;
        virtual auto addFence() -> FencePtr = 0;
        
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~SimulationInterface() = default;
    };
    
    using SimulationInterfacePtr = std::shared_ptr<SimulationInterface>;
}

