
#pragma once
#include "foundation/math.h"

#include <memory>

namespace voxel {
    class YardActorImpl : public std::enable_shared_from_this<YardActorImpl>, public YardInterface::Actor {
    public:
        YardActorImpl(const YardFacility &facility, const YardInterface::ActorTypeDesc &type, const math::vector3f &position, const math::vector3f &direction);
        ~YardActorImpl() override;
        
    public:
        void playAnimation(const char *animation, bool cycled, util::callback<void(Actor &)> &&completion) override;
        void rotate(const math::vector3f &targetDirection) override;
        
        auto getRadius() const -> float override;
        auto getPosition() const -> const math::vector3f & override;
        auto getDirection() const -> const math::vector3f & override;
        
        void update(float dtSec);
        
    private:
        const YardFacility &_facility;
        const YardInterface::ActorTypeDesc &_type;
        const YardInterface::ActorTypeDesc::Animation *_currentAnimation;
        
        util::callback<void(Actor &)> _animationFinished;
        float _animationAccumulatedTime = 0.0f;
        bool  _animationCycled = false;
        
        math::vector3f _position;
        math::vector3f _direction;
        
        YardLoadingState _currentState;
        SceneInterface::DynamicModelPtr _model;
        SceneInterface::LineSetPtr _bound;
    };
}

