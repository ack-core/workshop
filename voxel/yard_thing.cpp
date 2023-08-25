
#include "yard_base.h"
#include "yard_thing.h"

namespace {
    static const resource::MeshOptimization MESH_OPT = resource::MeshOptimization::OPTIMIZED;
}

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
                    _facility.getMeshProvider()->getOrLoadVoxelMesh(_modelPath.data(), MESH_OPT, [weak = weak_from_this()](const std::unique_ptr<resource::VoxelMesh> &mesh) {
                        if (std::shared_ptr<YardThing> self = weak.lock()) {
                            if (mesh) {
                                struct AsyncContext {
                                    std::vector<SceneInterface::VTXSVOX> voxels;
                                };
                                
                                self->_facility.getPlatform()->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak, &mesh](AsyncContext &ctx) {
                                    if (std::shared_ptr<YardThing> self = weak.lock()) {
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
                                    if (std::shared_ptr<YardThing> self = weak.lock()) {
                                        if (ctx.voxels.size()) {
                                            self->_voxData = std::move(ctx.voxels);
                                            self->_bboxmdl = self->_facility.getScene()->addBoundingBox(self->_bbox);
                                            self->_currentState = YardStatic::State::LOADED;
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
