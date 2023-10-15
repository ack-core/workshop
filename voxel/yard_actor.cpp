
#include "yard.h"
#include "yard_base.h"
#include "yard_actor.h"

namespace voxel {
    YardActorImpl::YardActorImpl(const YardFacility &facility, const YardActorType &type, const math::vector3f &position, const math::vector3f &direction)
    : _facility(facility)
    , _type(type)
    , _currentPosition(position)
    , _currentDirection(direction)
    , _currentState(YardLoadingState::NONE)
    {
    }
    
    YardActorImpl::~YardActorImpl() {
    
    }
    
    void YardActorImpl::rotate(const math::vector3f &targetDirection) {
        _targetDirection = targetDirection;
    }
    
    float YardActorImpl::getRadius() const {
        return _type.radius;
    }
    
    const math::vector3f &YardActorImpl::getPosition() const {
        return _currentPosition;
    }
    
    const math::vector3f &YardActorImpl::getDirection() const {
        return _currentDirection;
    }
    
    void YardActorImpl::update(float dtSec) {
        if (_currentState == YardLoadingState::NONE) {
            _currentState = YardLoadingState::LOADING;
            _facility.getMeshProvider()->getOrLoadVoxelMesh(_type.modelPath.data(), resource::MeshOptimization::DISABLED, [weak = weak_from_this()](const std::unique_ptr<resource::VoxelMesh> &mesh) {
                if (std::shared_ptr<YardActorImpl> self = weak.lock()) {
                    if (mesh) {
                        float angle = 0.0f; //(self->_currentDirection.x < 0.0f ? 1.0f : -1.0f) * math::vector3f(0.0f, 0.0f, 1.0f).angleTo(self->_currentDirection.normalized());
                        math::transform3f transform = math::transform3f({0, 1, 0}, angle).translated(self->_currentPosition);
                        
                        std::vector<SceneInterface::VTXDVOX> voxData;
                        for (std::uint16_t i = 0; i < mesh->frames[0].voxelCount; i++) {
                            voxData.emplace_back(SceneInterface::VTXDVOX{}); // TODO:
                        }
                        
                        self->_model = self->_facility.getScene()->addDynamicModel(&voxData, 1, transform);
                        self->_currentState = YardLoadingState::RENDERING;
                    }
                    else {
                        self->_facility.getPlatform()->logError("[YardActorImpl::YardActorImpl] unable to load mesh '%s'\n", self->_type.modelPath.data());
                    }
                }
            });
        }
    }
}
