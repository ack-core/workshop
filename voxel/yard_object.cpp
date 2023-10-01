
#include "yard.h"
#include "yard_base.h"
#include "yard_object.h"

namespace {
    static const resource::MeshOptimization MESH_OPT = resource::MeshOptimization::DISABLED;
}

namespace voxel {
    YardObjectImpl::YardObjectImpl(const YardFacility &facility, const YardCollision &collision, const YardObjectType &type, const math::vector3f &position, const math::vector3f &direction)
    : _facility(facility)
    , _collision(collision)
    , _type(type)
    , _currentPosition(position)
    , _currentDirection(direction)
    , _currentState(State::NONE)
    {
    }
    
    YardObjectImpl::~YardObjectImpl() {
    
    }
    
    void YardObjectImpl::attach(const char *helper, const char *type, const math::transform3f &trfm) {
    
    }
    
    void YardObjectImpl::detach(const char *helper) {
    
    }
    
    bool YardObjectImpl::isHelperOccupied(const char *helper) const {
        return false;
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
        if (_currentState == State::NONE) {
            _currentState = State::LOADING;
            
            _facility.getMeshProvider()->getOrLoadVoxelMesh(_type.model.data(), MESH_OPT, [weak = weak_from_this()](const std::unique_ptr<resource::VoxelMesh> &mesh) {
                if (std::shared_ptr<YardObjectImpl> self = weak.lock()) {
                    if (mesh) {
                        float angle = 0.0f; //(self->_currentDirection.x < 0.0f ? 1.0f : -1.0f) * math::vector3f(0.0f, 0.0f, 1.0f).angleTo(self->_currentDirection.normalized());
                        math::transform3f transform = math::transform3f({0, 1, 0}, angle).translated(self->_currentPosition);
                        
                        std::vector<SceneInterface::VTXDVOX> voxData;
                        for (std::uint16_t i = 0; i < mesh->frames[0].voxelCount; i++) {
                            voxData.emplace_back(SceneInterface::VTXDVOX{}); // TODO:
                        }
                        
                        self->_model = self->_facility.getScene()->addDynamicModel(&voxData, 1, transform);
                        self->_currentState = State::RENDERING;
                    }
                    else {
                        self->_facility.getPlatform()->logError("[YardObjectImpl::YardObjectImpl] unable to load mesh '%s'\n", self->_type.model.data());
                    }
                }
            });
        }
    }
}
