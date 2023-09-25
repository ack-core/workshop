
#pragma once
#include "foundation/math.h"

#include <memory>

namespace voxel {
    class YardObjectImpl : public std::enable_shared_from_this<YardObjectImpl>, public YardInterface::Object {
    public:
        YardObjectImpl(const YardFacility &facility, const YardCollision &collision, const YardObjectType &type, const math::vector3f &position, const math::vector3f &direction);
        ~YardObjectImpl();
    
    public:
        void attach(const char *helper, const char *type, const math::transform3f &trfm) override;
        void detach(const char *helper) override;
        bool isHelperOccupied(const char *helper) const override;
                
        void instantMove(const math::vector3f &position) override;
        void continuousMove(const math::vector3f &increment) override;
        void rotate(const math::vector3f &targetDirection) override;
        
        const math::vector3f &getPosition() const override;
        const math::vector3f &getDirection() const override;
        
        void update(float dtSec);
        
    private:
        enum class State {
            NONE = 0,
            LOADING,
            RENDERING,
        };
        
        const YardFacility &_facility;
        const YardCollision &_collision;
        const YardObjectType &_type;
        
        SceneInterface::DynamicModelPtr _model;
        math::vector3f _currentPosition;
        math::vector3f _currentDirection;
        math::vector3f _targetDirection;
        
        State _currentState;
    };
}

