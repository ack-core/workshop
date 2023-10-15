
#include "yard.h"
#include "yard_base.h"
#include "yard_thing.h"

namespace voxel {
    YardThingImpl::YardThingImpl(const YardFacility &facility, const math::vector3f &position, const math::bound3f &bbox, std::string &&model)
    : YardStatic(facility, bbox)
    , _position(position)
    , _modelPath(std::move(model))
    {
    
    }
    
    YardThingImpl::~YardThingImpl() {
        _model = nullptr;
        _bboxmdl = nullptr;
    }
    
    void YardThingImpl::setPosition(const math::vector3f &position) {
        _position = position;
        if (_bboxmdl) {
            _bboxmdl->setPosition(position);
        }
        if (_model) {
            _model->setPosition(position);
        }
    }
    
    const math::vector3f &YardThingImpl::getPosition() const {
        return _position;
    }
    
    void YardThingImpl::updateState(YardLoadingState targetState) {
        if (_currentState != targetState) {
            if (targetState == YardLoadingState::NONE) { // unload
                _currentState = targetState;
                _model = nullptr;
                _bboxmdl = nullptr;
            }
            else {
                const resource::MeshOptimization opt = resource::MeshOptimization::OPTIMIZED;
                if (_currentState == YardLoadingState::NONE) { // load resources
                    _currentState = YardLoadingState::LOADING;
                    _facility.getMeshProvider()->getOrLoadVoxelMesh(_modelPath.data(), opt, [weak = weak_from_this()](const std::unique_ptr<resource::VoxelMesh> &mesh) {
                        if (std::shared_ptr<YardThingImpl> self = weak.lock()) {
                            if (mesh) {
                                struct AsyncContext {
                                    std::vector<SceneInterface::VTXSVOX> voxels;
                                };
                                
                                self->_facility.getPlatform()->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak, &mesh](AsyncContext &ctx) {
                                    if (std::shared_ptr<YardThingImpl> self = weak.lock()) {
                                        ctx.voxels.reserve(mesh->frames[0].voxelCount);
                                        
                                        for (std::uint16_t i = 0; i < mesh->frames[0].voxelCount; i++) {
                                            SceneInterface::VTXSVOX &voxel = ctx.voxels.emplace_back(SceneInterface::VTXSVOX{});
                                            voxel.positionX = mesh->frames[0].voxels[i].positionX + std::uint16_t(self->_bbox.xmin + 0.5f + std::numeric_limits<float>::epsilon());
                                            voxel.positionY = mesh->frames[0].voxels[i].positionY + std::uint16_t(self->_bbox.ymin + 0.5f + std::numeric_limits<float>::epsilon());
                                            voxel.positionZ = mesh->frames[0].voxels[i].positionZ + std::uint16_t(self->_bbox.zmin + 0.5f + std::numeric_limits<float>::epsilon());
                                            voxel.colorIndex = mesh->frames[0].voxels[i].colorIndex;
                                            voxel.mask = mesh->frames[0].voxels[i].mask;
                                            voxel.scaleX = mesh->frames[0].voxels[i].scaleX;
                                            voxel.scaleY = mesh->frames[0].voxels[i].scaleY;
                                            voxel.scaleZ = mesh->frames[0].voxels[i].scaleZ;
                                            voxel.reserved = 0;
                                        }
                                    }
                                },
                                [weak](AsyncContext &ctx) {
                                    if (std::shared_ptr<YardThingImpl> self = weak.lock()) {
                                        if (ctx.voxels.size()) {
                                            self->_voxData = std::move(ctx.voxels);
                                            self->_bboxmdl = self->_facility.getScene()->addBoundingBox(self->_bbox);
                                            self->_bboxmdl->setPosition(self->_position);
                                            self->_currentState = YardLoadingState::LOADED;
                                        }
                                    }
                                }));
                            }
                            else {
                                self->_facility.getPlatform()->logError("[YardThing::updateState] unable to load mesh '%s'\n", self->_modelPath.data());
                            }
                        }
                    });
                }
                if (targetState == YardLoadingState::RENDERING) { // add to scene
                    if (_currentState == YardLoadingState::LOADED) {
                        _currentState = targetState;
                        _model = _facility.getScene()->addStaticModel(_voxData);
                    }
                }
                if (targetState == YardLoadingState::LOADED) { // remove from scene
                    if (_currentState == YardLoadingState::RENDERING) {
                        _currentState = targetState;
                        _model = nullptr;
                    }
                }
            }
        }
    
    }
}
