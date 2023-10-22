
#include "yard.h"
#include "yard_base.h"
#include "yard_actor.h"

namespace voxel {
    YardActorImpl::YardActorImpl(const YardFacility &facility, const YardInterface::ActorTypeDesc &type, const math::vector3f &position, const math::vector3f &direction)
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
        if (_currentState == YardLoadingState::RENDERING) {
            const float angle = (_currentDirection.x < 0.0f ? 1.0f : -1.0f) * math::vector2f(0.0f, 1.0f).angleTo(_currentDirection.xz.normalized());
            _model->setTransform(math::transform3f({0, 1, 0}, angle).translated(_currentPosition));
        }
        if (_currentState == YardLoadingState::NONE) {
            _currentState = YardLoadingState::LOADING;
            _facility.getMeshProvider()->getOrLoadVoxelMesh(_type.modelPath.data(), resource::MeshOptimization::VISIBLE, [weak = weak_from_this()](const std::unique_ptr<resource::VoxelMesh> &mesh) {
                if (std::shared_ptr<YardActorImpl> self = weak.lock()) {
                    if (mesh) {
                        std::vector<std::vector<SceneInterface::VTXDVOX>> voxData (mesh->frameCount);
                        
                        for (std::uint16_t f = 0; f < mesh->frameCount; f++) {
                            for (std::uint16_t i = 0; i < mesh->frames[f].voxelCount; i++) {
                                const resource::VoxelMesh::Voxel &src = mesh->frames[f].voxels[i];
                                SceneInterface::VTXDVOX &voxel = voxData[f].emplace_back(SceneInterface::VTXDVOX{});
                                voxel.positionX = float(src.positionX) - self->_type.centerPoint.x;
                                voxel.positionY = float(src.positionY) - self->_type.centerPoint.y;
                                voxel.positionZ = float(src.positionZ) - self->_type.centerPoint.z;
                                voxel.colorIndex = src.colorIndex;
                                voxel.mask = src.mask;
                            }
                        }
                        
                        self->_model = self->_facility.getScene()->addDynamicModel(voxData.data(), mesh->frameCount, math::transform3f({0, 1, 0}, 0.0f));
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
