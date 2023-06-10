
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/math.h"

namespace voxel {
    class YardObjectImpl : public YardInterface::Object {
    public:
        YardObjectImpl(const YardFacility &facility, const YardCollision &collision, const YardObjectType &type, const math::vector3f &position, const math::vector3f &direction);
        ~YardObjectImpl();
    
    public:
        void attach(const char *helper, const char *type, const math::transform3f &trfm) override;
        void detach(const char *helper) override;
    
        void instantMove(const math::vector3f &position) override;
        void continuousMove(const math::vector3f &increment) override;
        void rotate(const math::vector3f &targetDirection) override;
        
        const math::vector3f &getPosition() const override;
        const math::vector3f &getDirection() const override;
        
        void update(float dtSec) override;
        
    private:
        const YardFacility &_facility;
        const YardCollision &_collision;
        
        SceneInterface::DynamicModelPtr _model;
        math::vector3f _currentPosition;
        math::vector3f _currentDirection;
        math::vector3f _targetDirection;
    };
}

