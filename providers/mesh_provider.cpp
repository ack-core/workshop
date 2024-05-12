
#include "mesh_provider.h"
#include "meshes_list.h"
#include "foundation/layouts.h"

#include <list>

namespace resource {
    namespace {
        struct AsyncContext {
            struct Voxel {
                std::int16_t positionX, positionY, positionZ;
                std::uint8_t colorIndex, mask, scaleX, scaleY, scaleZ, reserved;
            };
            struct Frame {
                std::unique_ptr<Voxel[]> voxels;
                std::uint16_t voxelCount = 0;
            };
        
            std::unique_ptr<Frame[]> frames;
            std::uint16_t frameCount = 0;
            std::size_t maxVoxelCount = 0;
        };
        
        bool readVox(AsyncContext &ctx, const std::uint8_t *data) {
            if (memcmp(data, "VOX ", 4) == 0) {
                if (*(std::int32_t *)(data + 4) == 0x7f) { // vox made by gen_meshes.py
                    data += 32;
                    ctx.frameCount = *(std::uint32_t *)data;
                    ctx.frames = std::make_unique<AsyncContext::Frame[]>(ctx.frameCount);
                    data += sizeof(std::uint32_t);
                    
                    for (std::uint32_t f = 0; f < ctx.frameCount; f++) {
                        AsyncContext::Frame &frame = ctx.frames[f];
                        frame.voxelCount = *(std::uint32_t *)data;
                        frame.voxels = std::make_unique<AsyncContext::Voxel[]>(frame.voxelCount);
                        
                        if (frame.voxelCount > ctx.maxVoxelCount) {
                            ctx.maxVoxelCount = frame.voxelCount;
                        }
                        
                        data += sizeof(std::uint32_t);
                        
                        for (std::uint32_t i = 0; i < frame.voxelCount; i++) {
                            frame.voxels[i] = *(AsyncContext::Voxel *)data;
                            data += sizeof(AsyncContext::Voxel);
                        }
                    }
                    
                    return true;
                }
            }
            
            return false;
        }
    }
    
    class MeshProviderImpl : public std::enable_shared_from_this<MeshProviderImpl>, public MeshProvider {
    public:
        MeshProviderImpl(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering);
        ~MeshProviderImpl() override;
        
        const MeshInfo *getMeshInfo(const char *voxPath) override;
        
        void getOrLoadVoxelStatic(const char *voxPath, util::callback<void(const foundation::RenderDataPtr &)> &&completion) override;
        void getOrLoadVoxelObject(const char *voxPath, util::callback<void(const std::vector<foundation::RenderDataPtr> &)> &&completion) override;
        
        void update(float dtSec) override;
        
    private:
        const foundation::PlatformInterfacePtr _platform;
        const foundation::RenderingInterfacePtr _rendering;
        
        std::unordered_map<std::string, foundation::RenderDataPtr> _statics;
        std::unordered_map<std::string, std::vector<foundation::RenderDataPtr>> _objects;
        
        struct QueueEntryStatic {
            std::string voxPath;
            util::callback<void(const foundation::RenderDataPtr &)> callback;
        };
        struct QueueEntryObject {
            std::string voxPath;
            util::callback<void(const std::vector<foundation::RenderDataPtr> &)> callback;
        };
        
        std::list<QueueEntryStatic> _callsQueueStatic;
        std::list<QueueEntryObject> _callsQueueObject;
        bool _asyncInProgress;
    };
    
    MeshProviderImpl::MeshProviderImpl(
        const std::shared_ptr<foundation::PlatformInterface> &platform,
        const foundation::RenderingInterfacePtr &rendering
    )
    : _platform(platform)
    , _rendering(rendering)
    , _asyncInProgress(false)
    {
    }
    
    MeshProviderImpl::~MeshProviderImpl() {
    
    }
    
    const MeshInfo *MeshProviderImpl::getMeshInfo(const char *voxPath) {
        auto index = MESHES_LIST.find(voxPath);
        return index != MESHES_LIST.end() ? &index->second : nullptr;
    }
    
    void MeshProviderImpl::getOrLoadVoxelStatic(const char *voxPath, util::callback<void(const foundation::RenderDataPtr &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueueStatic.emplace_back(QueueEntryStatic {
                .voxPath = voxPath,
                .callback = std::move(completion)
            });
            
            return;
        }
        
        std::string path = std::string(voxPath);

        auto index = _statics.find(path);
        if (index != _statics.end()) {
            completion(index->second);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".vox").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<MeshProviderImpl> self = weak.lock()) {
                    if (len) {
                        self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak, path, bin = std::move(mem), len](AsyncContext &ctx) {
                            if (std::shared_ptr<MeshProviderImpl> self = weak.lock()) {
                                if (readVox(ctx, bin.get()) == false) {
                                    self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelStatic] '%s' is not a valid vox file", path.data());
                                }
                            }
                        },
                        [weak, path, completion = std::move(completion)](AsyncContext &ctx) {
                            if (std::shared_ptr<MeshProviderImpl> self = weak.lock()) {
                                self->_asyncInProgress = false;
                                
                                if (ctx.frameCount) {
                                    struct VTXSVOX {
                                        std::int16_t positionX, positionY, positionZ;
                                        std::uint8_t colorIndex, mask;
                                        std::uint8_t scaleX, scaleY, scaleZ, reserved;
                                    };
                                    
                                    std::vector<VTXSVOX> voxels (ctx.frames[0].voxelCount);
                                    
                                    for (std::size_t i = 0; i < voxels.size(); i++) {
                                        const AsyncContext::Voxel &src = ctx.frames[0].voxels[i];
                                        VTXSVOX &voxel = voxels[i];
                                        voxel.positionX = src.positionX;
                                        voxel.positionY = src.positionY;
                                        voxel.positionZ = src.positionZ;
                                        voxel.colorIndex = src.colorIndex;
                                        voxel.mask = src.mask;
                                        voxel.scaleX = src.scaleX;
                                        voxel.scaleY = src.scaleY;
                                        voxel.scaleZ = src.scaleZ;
                                        voxel.reserved = 0;
                                    }
                                    
                                    foundation::RenderDataPtr mesh = self->_rendering->createData(voxels.data(), layouts::VTXSVOX, ctx.frames[0].voxelCount);
                                    self->_statics.emplace(path, mesh);
                                    completion(mesh);
                                }
                                else {
                                    completion(nullptr);
                                }
                            }
                        }));
                    }
                    else {
                        self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelStatic] Unable to find file '%s'", path.data());
                        completion(nullptr);
                    }
                }
            });
        }
    }
    
    void MeshProviderImpl::getOrLoadVoxelObject(const char *voxPath, util::callback<void(const std::vector<foundation::RenderDataPtr> &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueueObject.emplace_back(QueueEntryObject {
                .voxPath = voxPath,
                .callback = std::move(completion)
            });
            
            return;
        }
        
        std::string path = std::string(voxPath);

        auto index = _objects.find(path);
        if (index != _objects.end()) {
            completion(index->second);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".vox").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<MeshProviderImpl> self = weak.lock()) {
                    if (len) {
                        self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak, path, bin = std::move(mem), len](AsyncContext &ctx) {
                            if (std::shared_ptr<MeshProviderImpl> self = weak.lock()) {
                                if (readVox(ctx, bin.get()) == false) {
                                    self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelObject] '%s' is not a valid vox file", path.data());
                                }
                            }
                        },
                        [weak, path, completion = std::move(completion)](AsyncContext &ctx) {
                            if (std::shared_ptr<MeshProviderImpl> self = weak.lock()) {
                                self->_asyncInProgress = false;
                                
                                if (ctx.frameCount) {
                                    struct VTXDVOX {
                                        float positionX, positionY, positionZ;
                                        std::uint8_t colorIndex, mask, r0, r1;
                                    };
                                    
                                    std::vector<foundation::RenderDataPtr> frames (ctx.frameCount);
                                    std::vector<VTXDVOX> voxels (ctx.maxVoxelCount);
                                    
                                    for (std::size_t f = 0; f < frames.size(); f++) {
                                        for (std::size_t i = 0; i < voxels.size(); i++) {
                                            const AsyncContext::Voxel &src = ctx.frames[f].voxels[i];
                                            VTXDVOX &voxel = voxels[i];
                                            voxel.positionX = float(src.positionX);
                                            voxel.positionY = float(src.positionY);
                                            voxel.positionZ = float(src.positionZ);
                                            voxel.colorIndex = src.colorIndex;
                                            voxel.mask = src.mask;
                                            voxel.r0 = 0;
                                            voxel.r1 = 0;
                                        }
                                        
                                        frames[f] = self->_rendering->createData(voxels.data(), layouts::VTXDVOX, ctx.frames[f].voxelCount);
                                    }
                                    
                                    const std::vector<foundation::RenderDataPtr> &result = self->_objects.emplace(path, std::move(frames)).first->second;
                                    completion(result);
                                }
                                else {
                                    completion({});
                                }
                            }
                        }));
                    }
                    else {
                        self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelObject] Unable to find file '%s'", path.data());
                        completion({});
                    }
                }
            });
        }
    }
    
    void MeshProviderImpl::update(float dtSec) {
        while (_asyncInProgress == false && _callsQueueStatic.size()) {
            QueueEntryStatic &entry = _callsQueueStatic.front();
            getOrLoadVoxelStatic(entry.voxPath.data(), std::move(entry.callback));
            _callsQueueStatic.pop_front();
        }
        while (_asyncInProgress == false && _callsQueueObject.size()) {
            QueueEntryObject &entry = _callsQueueObject.front();
            getOrLoadVoxelObject(entry.voxPath.data(), std::move(entry.callback));
            _callsQueueObject.pop_front();
        }
    }
}

namespace resource {
    std::shared_ptr<MeshProvider> MeshProvider::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const foundation::RenderingInterfacePtr &rendering) {
        return std::make_shared<MeshProviderImpl>(platform, rendering);
    }
}
