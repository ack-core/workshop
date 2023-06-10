
#include "mesh_provider.h"

#include <unordered_map>

namespace voxel {
    class MeshProviderImpl : public MeshProvider {
    public:
        MeshProviderImpl(const foundation::PlatformInterfacePtr &platform);
        ~MeshProviderImpl() override;
        
        const std::unique_ptr<Mesh> &getOrLoadVoxelMesh(const char *voxPath) override;
        const std::unique_ptr<Mesh> &getOrLoadVoxelMesh(const char *voxPath, const std::int16_t(&offset)[3]) override;
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        std::unordered_map<std::string, std::unique_ptr<Mesh>> _meshes;
        std::unique_ptr<Mesh> _empty;
    };
    
    MeshProviderImpl::MeshProviderImpl(const std::shared_ptr<foundation::PlatformInterface> &platform) : _platform(platform) {
    
    }
    
    MeshProviderImpl::~MeshProviderImpl() {
    
    }

    const std::unique_ptr<Mesh> &MeshProviderImpl::getOrLoadVoxelMesh(const char *voxPath) {
        std::int16_t zero[3] = {0, 0, 0};
        return getOrLoadVoxelMesh(voxPath, zero);
    }

    const std::unique_ptr<Mesh> &MeshProviderImpl::getOrLoadVoxelMesh(const char *name, const std::int16_t(&offset)[3]) {
        auto index = _meshes.find(name);
        if (index != _meshes.end()) {
            return index->second;
        }
        else {
            const std::int32_t version = 150;
            
            std::unique_ptr<std::uint8_t[]> voxData;
            std::size_t voxSize = 0;

            if (_platform->loadFile((std::string(name) + ".vox").data(), voxData, voxSize)) {
                std::uint8_t *data = voxData.get();
                
                if (memcmp(data, "VOX ", 4) == 0 && *(std::int32_t *)(data + 4) == version) {
                    // skip bytes of main chunk to start of the first child ('PACK')
                    data += 20;
                    std::int32_t frameCount = 1;
                    
                    if (memcmp(data, "PACK", 4) == 0) {
                        frameCount = *(std::int32_t *)(data + 12);
                        data += 16;
                    }
                    
                    std::unique_ptr<Mesh> &mesh = _meshes.emplace(std::string(name), std::make_unique<Mesh>()).first->second;
                    
                    mesh->frames = std::make_unique<Mesh::Frame[]>(frameCount);
                    mesh->frameCount = frameCount;
                    
                    for (std::int32_t i = 0; i < frameCount; i++) {
                        if (memcmp(data, "SIZE", 4) == 0) {
                            data += 24;
                            
                            if (memcmp(data, "XYZI", 4) == 0) {
                                std::size_t voxelCount = *(std::uint32_t *)(data + 12);
                                
                                data += 16;
                                
                                mesh->frames[i].voxels = std::make_unique<VoxelInfo[]>(voxelCount);
                                mesh->frames[i].voxelCount = voxelCount;
                                
                                for (std::size_t c = 0, k = 0; c < voxelCount; c++) {
                                    std::uint8_t z = *(std::uint8_t *)(data + c * 4 + 0);
                                    std::uint8_t x = *(std::uint8_t *)(data + c * 4 + 1);
                                    std::uint8_t y = *(std::uint8_t *)(data + c * 4 + 2);
                                    
                                    VoxelInfo &targetVoxel = mesh->frames[i].voxels[k++];
                                    
                                    targetVoxel.positionZ = std::int16_t(z) + offset[2];
                                    targetVoxel.positionX = std::int16_t(x) + offset[0];
                                    targetVoxel.positionY = std::int16_t(y) + offset[1];
                                    targetVoxel.colorIndex = *(std::uint8_t *)(data + c * 4 + 3);
                                }
                                
                                data += voxelCount * 4;
                            }
                            else {
                                _platform->logError("[voxel::createMesh] XYZI[%d] chunk is not found in '%s'", i, name);
                                break;
                            }
                        }
                        else {
                            _platform->logError("[voxel::createMesh] SIZE[%d] chunk is not found in '%s'", i, name);
                            break;
                        }
                    }
                    
                    return mesh;
                }
                else {
                    _platform->logError("[voxel::createMesh] Incorrect vox-header in '%s'", name);
                }
            }
            else {
                _platform->logError("[voxel::createMesh] Unable to find file '%s'", name);
            }
        }
        
        return _empty;
    }
}

namespace voxel {
    std::shared_ptr<MeshProvider> MeshProvider::instance(const std::shared_ptr<foundation::PlatformInterface> &platform) {
        return std::make_shared<MeshProviderImpl>(platform);
    }
}
