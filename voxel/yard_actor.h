
#pragma once
#include "foundation/math.h"

#include <memory>

namespace voxel {
    class YardActorImpl : public std::enable_shared_from_this<YardActorImpl>, public YardInterface::Actor {
    public:
        YardActorImpl(const YardFacility &facility, const YardActorType &type, const math::vector3f &position, const math::vector3f &direction);
        ~YardActorImpl() override;
        
    public:
        void rotate(const math::vector3f &targetDirection) override;
        
        auto getRadius() const -> float override;
        auto getPosition() const -> const math::vector3f & override;
        auto getDirection() const -> const math::vector3f & override;
        
        void update(float dtSec);
        
    private:
        const YardFacility &_facility;
        const YardActorType &_type;
        
        math::vector3f _currentPosition;
        math::vector3f _currentDirection;
        math::vector3f _targetDirection;
        
        YardLoadingState _currentState;
        SceneInterface::DynamicModelPtr _model;
    };
}

