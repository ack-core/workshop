
#include "yard_base.h"
#include "yard_thing.h"

namespace voxel {
    YardThing::YardThing(const YardFacility &facility, const math::bound3f &bbox, std::string &&model)
    : YardStatic(facility, bbox)
    , _modelPath(std::move(model))
    {
    
    }
    
    YardThing::~YardThing() {
        _model = nullptr;
        _bboxmdl = nullptr;
    }

    void YardThing::updateState(State targetState) {
        if (_currentState != targetState) {
            if (targetState == YardStatic::State::NONE) { // unload
                _currentState = targetState;
                _model = nullptr;
                _bboxmdl = nullptr;
            }
            else {
                if (_currentState == YardStatic::State::NONE) { // load resources
                    _currentState = YardStatic::State::LOADING;
                    _facility.getMeshProvider()->getOrLoadVoxelMesh(_modelPath.data(), true, [weak = weak_from_this()](const std::unique_ptr<resource::VoxelMesh> &mesh) {
                        if (std::shared_ptr<YardThing> self = weak.lock()) {
                            struct AsyncContext {
                                std::vector<SceneInterface::VTXSVOX> voxels;
                            };
                            
                            self->_facility.getPlatform()->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak, &mesh](AsyncContext &ctx) {
                                ctx.voxels.reserve(mesh->frames[0].voxelCount);
                                if (std::shared_ptr<YardThing> self = weak.lock()) {
                                    for (std::uint16_t i = 0; i < mesh->frames[0].voxelCount; i++) {
                                        SceneInterface::VTXSVOX &voxel = ctx.voxels.emplace_back(SceneInterface::VTXSVOX{});
                                        voxel.positionX = mesh->frames[0].voxels[i].positionX;
                                        voxel.positionY = mesh->frames[0].voxels[i].positionY;
                                        voxel.positionZ = mesh->frames[0].voxels[i].positionZ;
                                        voxel.colorIndex = mesh->frames[0].voxels[i].colorIndex;
                                        voxel.mask = mesh->frames[0].voxels[i].mask;
                                    }
                                }
                            },
                            [weak](AsyncContext &ctx) {
                                if (std::shared_ptr<YardThing> self = weak.lock()) {
                                    if (ctx.voxels.size()) {
                                        self->_voxData = std::move(ctx.voxels);
                                        self->_bboxmdl = self->_facility.getScene()->addBoundingBox(self->_bbox);
                                        self->_currentState = YardStatic::State::LOADED;
                                    }
                                }
                            }));
                        }
                    });
                }
                if (targetState == YardStatic::State::RENDERING) { // add to scene
                    if (_currentState == YardStatic::State::LOADED) {
                        _currentState = targetState;
                        _model = _facility.getScene()->addStaticModel(_voxData);
                    }
                }
                if (targetState == YardStatic::State::LOADED) { // remove from scene
                    if (_currentState == YardStatic::State::RENDERING) {
                        _currentState = targetState;
                        _model = nullptr;
                    }
                }
            }
        }
    
    }
}
