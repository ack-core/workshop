
#include "mesh_provider.h"
#include "meshes_list.h"

#include <list>

namespace resource {
    class MeshProviderImpl : public std::enable_shared_from_this<MeshProviderImpl>, public MeshProvider {
    public:
        MeshProviderImpl(const foundation::PlatformInterfacePtr &platform);
        ~MeshProviderImpl() override;
        
        const MeshInfo *getMeshInfo(const char *voxPath) override;
        void getOrLoadVoxelMesh(const char *voxPath, util::callback<void(const std::unique_ptr<resource::VoxelMesh> &)> &&completion) override;
        void update(float dtSec) override;
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        std::unordered_map<std::string, std::unique_ptr<VoxelMesh>> _meshes;
        
        struct QueueEntry {
            std::string voxPath;
            util::callback<void(const std::unique_ptr<VoxelMesh> &)> callback;
        };
        
        std::list<QueueEntry> _callsQueue;
        bool _asyncInProgress;
    };
    
    MeshProviderImpl::MeshProviderImpl(const std::shared_ptr<foundation::PlatformInterface> &platform) : _platform(platform), _asyncInProgress(false) {}
    MeshProviderImpl::~MeshProviderImpl() {
    
    }
    
    const MeshInfo *MeshProviderImpl::getMeshInfo(const char *voxPath) {
        auto index = MESHES_LIST.find(voxPath);
        return index != MESHES_LIST.end() ? &index->second : nullptr;
    }

    void MeshProviderImpl::getOrLoadVoxelMesh(const char *voxPath, util::callback<void(const std::unique_ptr<resource::VoxelMesh> &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueue.emplace_back(QueueEntry {
                .voxPath = voxPath,
                .callback = std::move(completion)
            });
            
            return;
        }
        
        std::string path = std::string(voxPath);

        auto index = _meshes.find(path);
        if (index != _meshes.end()) {
            completion(index->second);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".vox").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<MeshProviderImpl> self = weak.lock()) {
                    if (len) {
                        struct AsyncContext {
                            std::unique_ptr<VoxelMesh> mesh;
                        };
                        
                        self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak, path, bin = std::move(mem), len](AsyncContext &ctx) {
                            if (std::shared_ptr<MeshProviderImpl> self = weak.lock()) {
                                const std::int32_t version = 150;
                                const std::uint8_t *data = bin.get();
                                
                                if (memcmp(data, "VOX ", 4) == 0) {
                                    if (*(std::int32_t *)(data + 4) == 0x7f) { // vox made by gen_meshes.py
                                        data += 32;                                        
                                        ctx.mesh = std::make_unique<VoxelMesh>();
                                        ctx.mesh->frameCount = *(std::uint32_t *)data;
                                        ctx.mesh->frames = std::make_unique<VoxelMesh::Frame[]>(ctx.mesh->frameCount);
                                        data += sizeof(std::uint32_t);
                                        
                                        for (std::uint32_t f = 0; f < ctx.mesh->frameCount; f++) {
                                            ctx.mesh->frames[f].voxelCount = *(std::uint32_t *)data;
                                            ctx.mesh->frames[f].voxels = std::make_unique<VoxelMesh::Voxel[]>(ctx.mesh->frames[f].voxelCount);
                                            data += sizeof(std::uint32_t);
                                            
                                            for (std::uint32_t i = 0; i < ctx.mesh->frames[f].voxelCount; i++) {
                                                VoxelMesh::Voxel &targetVoxel = ctx.mesh->frames[f].voxels[i];
                                                targetVoxel = *(VoxelMesh::Voxel *)data;
                                                data += sizeof(VoxelMesh::Voxel);
                                            }
                                        }
                                    }
                                }
                                else {
                                    self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelMesh] '%s' is not a valid vox file", path.data());
                                }
                            }
                        },
                        [weak, path, completion = std::move(completion)](AsyncContext &ctx) {
                            if (std::shared_ptr<MeshProviderImpl> self = weak.lock()) {
                                self->_asyncInProgress = false;

                                if (ctx.mesh) {
                                    const std::unique_ptr<VoxelMesh> &mesh = self->_meshes.emplace(path, std::move(ctx.mesh)).first->second;
                                    completion(mesh);
                                }
                                else {
                                    completion(nullptr);
                                }
                            }
                        }));
                    }
                    else {
                        self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelMesh] Unable to find file '%s'", path.data());
                        completion(nullptr);
                    }
                }
            });
        }
    }
    
    void MeshProviderImpl::update(float dtSec) {
        while (_asyncInProgress == false && _callsQueue.size()) {
            QueueEntry &entry = _callsQueue.front();
            getOrLoadVoxelMesh(entry.voxPath.data(), std::move(entry.callback));
            _callsQueue.pop_front();
        }
    }
}

namespace resource {
    std::shared_ptr<MeshProvider> MeshProvider::instance(const std::shared_ptr<foundation::PlatformInterface> &platform) {
        return std::make_shared<MeshProviderImpl>(platform);
    }
}
