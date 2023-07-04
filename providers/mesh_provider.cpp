
#include "mesh_provider.h"

#include <unordered_map>

namespace resource {
    class MeshProviderImpl : public std::enable_shared_from_this<MeshProviderImpl>, public MeshProvider {
    public:
        MeshProviderImpl(const foundation::PlatformInterfacePtr &platform, const char *resourceList);
        ~MeshProviderImpl() override;
        
        const MeshInfo *getMeshInfo(const char *voxPath) override;
        void getOrLoadVoxelMesh(const char *voxPath, util::callback<void(const std::unique_ptr<VoxelMesh> &)> &&completion) override;
        void getOrLoadVoxelMesh(const char *voxPath, const std::int16_t(&offset)[3], util::callback<void(const std::unique_ptr<VoxelMesh> &)> &&completion) override;
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        std::unordered_map<std::string, std::unique_ptr<VoxelMesh>> _meshes;
        std::unique_ptr<VoxelMesh> _empty;
    };
    
    MeshProviderImpl::MeshProviderImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const char *resourceList) : _platform(platform) {
    
    }
    
    MeshProviderImpl::~MeshProviderImpl() {
    
    }
    
    const MeshInfo *MeshProviderImpl::getMeshInfo(const char *voxPath) {
        return nullptr;
    }

    void MeshProviderImpl::getOrLoadVoxelMesh(const char *voxPath, util::callback<void(const std::unique_ptr<VoxelMesh> &)> &&completion) {
        std::int16_t zero[3] = {0, 0, 0};
        return getOrLoadVoxelMesh(voxPath, zero, std::move(completion));
    }

    void MeshProviderImpl::getOrLoadVoxelMesh(const char *voxPath, const std::int16_t(&offset)[3], util::callback<void(const std::unique_ptr<VoxelMesh> &)> &&completion) {
        std::string path = std::string(voxPath);
        auto index = _meshes.find(path);
        if (index != _meshes.end()) {
            completion(index->second);
        }
        else {
            struct Offset {
                std::int16_t x, y, z;
            }
            voxoff{ offset[0], offset[1], offset[2] };
            
            _platform->loadFile((path + ".vox").data(), [weak = weak_from_this(), voxoff, path, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&mem, std::size_t size) mutable {
                if (std::shared_ptr<MeshProviderImpl> self = weak.lock()) {
                    const foundation::PlatformInterfacePtr &platform = self->_platform;
                    if (size) {
                        struct AsyncContext {
                            std::unique_ptr<VoxelMesh> mesh;
                        };
                        
                        self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak, path, voxoff, bin = std::move(mem), size](AsyncContext &ctx) {
                            if (std::shared_ptr<MeshProviderImpl> self = weak.lock()) {
                                const std::int32_t version = 150;
                                const std::uint8_t *data = bin.get();
                                
                                if (memcmp(data, "VOX ", 4) == 0 && *(std::int32_t *)(data + 4) == version) {
                                    // skip bytes of main chunk to start of the first child ('PACK')
                                    data += 20;
                                    std::int32_t frameCount = 1;
                                    
                                    if (memcmp(data, "PACK", 4) == 0) {
                                        frameCount = *(std::int32_t *)(data + 12);
                                        data += 16;
                                    }
                                    
                                    ctx.mesh = std::make_unique<VoxelMesh>();
                                    ctx.mesh->frames = std::make_unique<VoxelMesh::Frame[]>(frameCount);
                                    ctx.mesh->frameCount = frameCount;
                                    
                                    for (std::int32_t i = 0; i < frameCount; i++) {
                                        if (memcmp(data, "SIZE", 4) == 0) {
                                            data += 24;
                                            
                                            if (memcmp(data, "XYZI", 4) == 0) {
                                                std::size_t voxelCount = *(std::uint32_t *)(data + 12);
                                                
                                                data += 16;
                                                
                                                ctx.mesh->frames[i].voxels = std::make_unique<VoxelMesh::Voxel[]>(voxelCount);
                                                ctx.mesh->frames[i].voxelCount = voxelCount;
                                                
                                                for (std::size_t c = 0, k = 0; c < voxelCount; c++) {
                                                    std::uint8_t z = *(std::uint8_t *)(data + c * 4 + 0);
                                                    std::uint8_t x = *(std::uint8_t *)(data + c * 4 + 1);
                                                    std::uint8_t y = *(std::uint8_t *)(data + c * 4 + 2);
                                                    
                                                    VoxelMesh::Voxel &targetVoxel = ctx.mesh->frames[i].voxels[k++];
                                                    
                                                    targetVoxel.positionZ = std::int16_t(z) + voxoff.z;
                                                    targetVoxel.positionX = std::int16_t(x) + voxoff.x;
                                                    targetVoxel.positionY = std::int16_t(y) + voxoff.y;
                                                    targetVoxel.colorIndex = *(std::uint8_t *)(data + c * 4 + 3);
                                                }
                                                
                                                data += voxelCount * 4;
                                            }
                                            else {
                                                self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelMesh] XYZI[%d] chunk is not found in '%s'", i, path.data());
                                                ctx.mesh = nullptr;
                                                break;
                                            }
                                        }
                                        else {
                                            self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelMesh] SIZE[%d] chunk is not found in '%s'", i, path.data());
                                            ctx.mesh = nullptr;
                                            break;
                                        }
                                    }
                                }
                                else {
                                    self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelMesh] Incorrect vox-header in '%s'", path.data());
                                }
                            }
                        },
                        [weak, path, completion = std::move(completion)](AsyncContext &ctx) {
                            if (std::shared_ptr<MeshProviderImpl> self = weak.lock()) {
                                if (ctx.mesh) {
                                    auto index = self->_meshes.find(path);
                                    if (index != self->_meshes.end()) {
                                        completion(index->second);
                                    }
                                    else {
                                        const std::unique_ptr<VoxelMesh> &mesh = self->_meshes.emplace(path, std::move(ctx.mesh)).first->second;
                                        completion(mesh);
                                    }
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
}

namespace resource {
    std::shared_ptr<MeshProvider> MeshProvider::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const char *resourceList) {
        return std::make_shared<MeshProviderImpl>(platform, resourceList);
    }
}
