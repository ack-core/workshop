
#include "yard.h"
#include "yard_base.h"
#include "yard_object.h"

namespace voxel {
    YardObjectImpl::YardObjectImpl(const YardFacility &facility, const YardCollision &collision, const YardObjectType &type, const math::vector3f &position, const math::vector3f &direction)
    : _facility(facility)
    , _collision(collision)
    , _currentPosition(position)
    {
        if (const std::unique_ptr<Mesh> &mesh = _facility.getMeshProvider()->getOrLoadVoxelMesh(type.model.data())) {
            float angle = (direction.x < 0.0f ? 1.0f : -1.0f) * math::vector3f(0.0f, 0.0f, 1.0f).angleTo(direction.normalized());
            math::transform3f transform = math::transform3f::identity().rotated({0, 1, 0}, angle).translated(position);
            _model = _facility.getScene()->addDynamicModel(*mesh, type.center, transform);
        }
        else {
            _facility.getLogger()->logError("[YardObjectImpl::YardObjectImpl] unable to load mesh '%s'\n", type.model.data());
        }
    }
    
    YardObjectImpl::~YardObjectImpl() {
    
    }
    
    void YardObjectImpl::attach(const char *helper, const char *type, const math::transform3f &trfm) {
    
    }
    
    void YardObjectImpl::detach(const char *helper) {
    
    }
    
    void YardObjectImpl::instantMove(const math::vector3f &position) {
        _currentPosition = position;
    }
    
    void YardObjectImpl::continuousMove(const math::vector3f &increment) {
        math::vector3f movement = increment;
        _collision.correctMovement(_currentPosition, movement);
        _currentPosition = _currentPosition + movement;
    }
    
    void YardObjectImpl::rotate(const math::vector3f &targetDirection) {
        _targetDirection = targetDirection;
    }
    
    const math::vector3f &YardObjectImpl::getPosition() const {
        return _currentPosition;
    }
    
    const math::vector3f &YardObjectImpl::getDirection() const {
        return _currentDirection;
    }
    
    void YardObjectImpl::update(float dtSec) {
    
    }
}
