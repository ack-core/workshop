
#pragma once
#include "foundation/util.h"
#include "foundation/math.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace voxel {
    class SimulationInterface {
    public:
        static std::shared_ptr<SimulationInterface> instance();
        
    public:
        enum class EntityType {
            HOOP,
            WALL
        };
        
        struct Hoop {
            virtual void setImpulse(const math::vector3f &impulse) = 0;
            virtual void setImpactHandler(util::callback<void(const math::vector3f &point, const math::vector3f &impulse, EntityType type)> &&handler) = 0;
            virtual auto getPosition() const -> const math::vector3f & = 0;
            virtual ~Hoop() = default;
        };
        struct Wall {
            virtual void addSegment(const math::vector3f &pStart, const math::vector3f &pEnd) = 0;
            virtual void removeAllSegments() = 0;
            virtual ~Wall() = default;
        };
        
        using HoopPtr = std::shared_ptr<Hoop>;
        using WallPtr = std::shared_ptr<Wall>;
        
    public:
        virtual auto addHoop(const math::vector3f &position, float radius) -> HoopPtr = 0;
        virtual auto addWall() -> WallPtr = 0;
        
        virtual void updateAndDraw(float dtSec) = 0;
        
    public:
        virtual ~SimulationInterface() = default;
    };
    
    using SimulationInterfacePtr = std::shared_ptr<SimulationInterface>;
}

