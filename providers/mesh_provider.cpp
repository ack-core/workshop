
#include "mesh_provider.h"

#include <unordered_map>

namespace resource {
    class MeshProviderImpl : public std::enable_shared_from_this<MeshProviderImpl>, public MeshProvider {
    public:
        MeshProviderImpl(const foundation::PlatformInterfacePtr &platform, const char *resourceList);
        ~MeshProviderImpl() override;
        
        const MeshInfo *getMeshInfo(const char *voxPath) override;
        void getOrLoadVoxelMesh(const char *voxPath, std::function<void(const std::unique_ptr<VoxelMesh> &)> &&completion) override;
        void getOrLoadVoxelMesh(const char *voxPath, const std::int16_t(&offset)[3], std::function<void(const std::unique_ptr<VoxelMesh> &)> &&completion) override;
        
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

    void MeshProviderImpl::getOrLoadVoxelMesh(const char *voxPath, std::function<void(const std::unique_ptr<VoxelMesh> &)> &&completion) {
        std::int16_t zero[3] = {0, 0, 0};
        return getOrLoadVoxelMesh(voxPath, zero, std::move(completion));
    }

    void MeshProviderImpl::getOrLoadVoxelMesh(const char *voxPath, const std::int16_t(&offset)[3], std::function<void(const std::unique_ptr<VoxelMesh> &)> &&completion) {
        auto index = _meshes.find(voxPath);
        if (index != _meshes.end()) {
            completion(index->second);
        }
        else {
            struct Offset {
                std::int16_t x, y, z;
            }
            voxoff{ offset[0], offset[1], offset[2] };
            std::string path = std::string(voxPath) + ".vox";
            
            _platform->loadFile(path.data(), [weak = weak_from_this(), voxoff, path, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&mem, std::size_t size) {
                if (std::shared_ptr<MeshProviderImpl> self = weak.lock()) {
                    // TODO: parse in async
                    if (size) {
                        const std::int32_t version = 150;
                        const std::uint8_t *data = mem.get();
                        
                        if (memcmp(data, "VOX ", 4) == 0 && *(std::int32_t *)(data + 4) == version) {
                            // skip bytes of main chunk to start of the first child ('PACK')
                            data += 20;
                            std::int32_t frameCount = 1;
                            
                            if (memcmp(data, "PACK", 4) == 0) {
                                frameCount = *(std::int32_t *)(data + 12);
                                data += 16;
                            }
                            
                            std::unique_ptr<VoxelMesh> &mesh = self->_meshes.emplace(path, std::make_unique<VoxelMesh>()).first->second;
                            
                            mesh->frames = std::make_unique<VoxelMesh::Frame[]>(frameCount);
                            mesh->frameCount = frameCount;
                            
                            for (std::int32_t i = 0; i < frameCount; i++) {
                                if (memcmp(data, "SIZE", 4) == 0) {
                                    data += 24;
                                    
                                    if (memcmp(data, "XYZI", 4) == 0) {
                                        std::size_t voxelCount = *(std::uint32_t *)(data + 12);
                                        
                                        data += 16;
                                        
                                        mesh->frames[i].voxels = std::make_unique<VoxelMesh::Voxel[]>(voxelCount);
                                        mesh->frames[i].voxelCount = voxelCount;
                                        
                                        for (std::size_t c = 0, k = 0; c < voxelCount; c++) {
                                            std::uint8_t z = *(std::uint8_t *)(data + c * 4 + 0);
                                            std::uint8_t x = *(std::uint8_t *)(data + c * 4 + 1);
                                            std::uint8_t y = *(std::uint8_t *)(data + c * 4 + 2);
                                            
                                            VoxelMesh::Voxel &targetVoxel = mesh->frames[i].voxels[k++];
                                            
                                            targetVoxel.positionZ = std::int16_t(z) + voxoff.z;
                                            targetVoxel.positionX = std::int16_t(x) + voxoff.x;
                                            targetVoxel.positionY = std::int16_t(y) + voxoff.y;
                                            targetVoxel.colorIndex = *(std::uint8_t *)(data + c * 4 + 3);
                                        }
                                        
                                        data += voxelCount * 4;
                                    }
                                    else {
                                        self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelMesh] XYZI[%d] chunk is not found in '%s'", i, path.data());
                                        break;
                                    }
                                }
                                else {
                                    self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelMesh] SIZE[%d] chunk is not found in '%s'", i, path.data());
                                    break;
                                }
                            }
                            
                            completion(mesh);
                            return;
                        }
                        else {
                            self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelMesh] Incorrect vox-header in '%s'", path.data());
                        }
                    }
                    else {
                        self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelMesh] Unable to find file '%s'", path.data());
                    }
                    
                    completion(nullptr);
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
