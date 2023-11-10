
#include "yard.h"
#include "yard_base.h"
#include "yard_actor.h"

namespace voxel {
    YardActorImpl::YardActorImpl(const YardFacility &facility, const YardInterface::ActorTypeDesc &type, const math::vector3f &position, const math::vector3f &direction)
    : _facility(facility)
    , _type(type)
    , _position(position)
    , _direction(direction)
    , _currentAnimation(nullptr)
    , _currentState(YardLoadingState::NONE)
    {
    }
    
    YardActorImpl::~YardActorImpl() {
    
    }
    
    void YardActorImpl::playAnimation(const char *animation, bool cycled, util::callback<void(Actor &)> &&completion) {
        auto index = _type.animations.find(animation);
        if (index != _type.animations.end()) {
            _currentAnimation = &index->second;
            _animationFinished = std::move(completion);
            _animationCycled = cycled;
            _animationAccumulatedTime = 0.0f;
        }
        else {
            _facility.getPlatform()->logError("[YardActorImpl::playAnimation] animation '%s' is not found\n", animation);
        }
    }
    
    void YardActorImpl::rotate(const math::vector3f &targetDirection) {
        _direction = targetDirection;
    }
    
    float YardActorImpl::getRadius() const {
        return _type.radius;
    }
    
    const math::vector3f &YardActorImpl::getPosition() const {
        return _position;
    }
    
    const math::vector3f &YardActorImpl::getDirection() const {
        return _direction;
    }
    
    void YardActorImpl::update(float dtSec) {
        if (_currentState == YardLoadingState::RENDERING) {
            const float angle = (_direction.x < 0.0f ? 1.0f : -1.0f) * math::vector2f(0.0f, 1.0f).angleTo(_direction.xz.normalized());
            _model->setTransform(math::transform3f({0, 1, 0}, angle).translated(_position));
            _bound->setPosition(_position);

            if (_currentAnimation) {
                _animationAccumulatedTime += dtSec;

                const std::uint32_t frameIndex = std::uint32_t(_animationAccumulatedTime * _type.animFPS);
                const std::uint32_t frameCount = _currentAnimation->lastFrameIndex - _currentAnimation->firstFrameIndex + 1;
                
                if (frameIndex >= frameCount) {
                    if (_animationCycled) {
                        _animationAccumulatedTime -= float(frameCount) / _type.animFPS;
                        _model->setFrame(_currentAnimation->firstFrameIndex);
                    }
                    else {
                        _animationAccumulatedTime = float(frameCount - 1) / _type.animFPS;
                        _model->setFrame(_currentAnimation->lastFrameIndex);
                    }
                    if (_animationFinished) {
                        _animationFinished(*this);
                    }
                }
                else {
                    _model->setFrame(_currentAnimation->firstFrameIndex + frameIndex);
                }
            }
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
                        self->_bound = self->_facility.getScene()->addLineSet(true, 12);
                        self->_currentState = YardLoadingState::RENDERING;
                        
                        for (int i = 0; i < 12; i++) {
                            const float a0 = float(i) / 6.0f * M_PI;
                            const float a1 = float(i + 1) / 6.0f * M_PI;
                            const float radius = self->_type.radius;
                            const math::vector3f start = math::vector3f(radius * std::sin(a0), 0, radius * std::cos(a0));
                            const math::vector3f end = math::vector3f(radius * std::sin(a1), 0, radius * std::cos(a1));
                            self->_bound->addLine(start, end, math::color(1.0f, 1.0f, 1.0f, 1.0f));
                        }
                    }
                    else {
                        self->_facility.getPlatform()->logError("[YardActorImpl::update] unable to load mesh '%s'\n", self->_type.modelPath.data());
                    }
                }
            });
        }
    }
}
