
#include "mesh_provider.h"

#include <list>
#include <unordered_map>
#include <sstream>

namespace resource {
    class MeshProviderImpl : public std::enable_shared_from_this<MeshProviderImpl>, public MeshProvider {
    public:
        MeshProviderImpl(const foundation::PlatformInterfacePtr &platform, const char *resourceList);
        ~MeshProviderImpl() override;
        
        const MeshInfo *getMeshInfo(const char *voxPath) override;
        void getOrLoadVoxelMesh(const char *voxPath, bool scaledVoxels, util::callback<void(const std::unique_ptr<VoxelMesh> &)> &&completion) override;
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        std::unordered_map<std::string, MeshInfo> _meshInfos;
        std::unordered_map<std::string, std::unique_ptr<VoxelMesh>> _meshes;
        std::unique_ptr<VoxelMesh> _empty;
        
        struct QueueEntry {
            bool scaled;
            std::string voxPath;
            util::callback<void(const std::unique_ptr<VoxelMesh> &)> callback;
        };
        
        std::list<QueueEntry> _callsQueue;
        bool _asyncInProgress;
    };
    
    MeshProviderImpl::MeshProviderImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const char *resourceList) : _platform(platform), _asyncInProgress(false) {
        std::istringstream source = std::istringstream(resourceList);
        std::string line, path;
        MeshInfo info;
        
        while (std::getline(source, line)) {
            printf("-->> %s\n", line.data());
            std::istringstream input = std::istringstream(line);
            
            if (line.length()) {
                if (input >> path >> info.sizeX >> info.sizeY >> info.sizeZ) {
                    _meshInfos.emplace(path, info);
                }
                else {
                    _platform->logError("[MeshProviderImpl::MeshProviderImpl] Bad mesh info in '%s'\n", line.data());
                }
            }
        }
    }
    
    MeshProviderImpl::~MeshProviderImpl() {
    
    }
    
    const MeshInfo *MeshProviderImpl::getMeshInfo(const char *voxPath) {
        auto index = _meshInfos.find(voxPath);
        return index != _meshInfos.end() ? &index->second : nullptr;
    }

    void MeshProviderImpl::getOrLoadVoxelMesh(const char *voxPath, bool scaledVoxels, util::callback<void(const std::unique_ptr<VoxelMesh> &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueue.emplace_back(QueueEntry {
                .scaled = scaledVoxels,
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
                    const foundation::PlatformInterfacePtr &platform = self->_platform;
                    if (len) {
                        struct AsyncContext {
                            std::unique_ptr<VoxelMesh> mesh;
                        };
                        
                        self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak, path, bin = std::move(mem), len](AsyncContext &ctx) {
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
                                            data += 12;
                                            int sizeZ = *(int *)(data + 0) + 2;
                                            int sizeX = *(int *)(data + 4) + 2;
                                            int sizeY = *(int *)(data + 8) + 2;
                                            data += 12;
                                            
                                            if (memcmp(data, "XYZI", 4) == 0) {
                                                std::unique_ptr<std::uint8_t[]> voxMap = std::make_unique<std::uint8_t[]>(sizeX * sizeY * sizeZ);
                                                std::size_t voxelCount = *(std::uint32_t *)(data + 12);
                                                
                                                data += 16;
                                                
                                                for (std::size_t c = 0; c < voxelCount; c++) {
                                                    std::uint8_t z = *(std::uint8_t *)(data + c * 4 + 0);
                                                    std::uint8_t x = *(std::uint8_t *)(data + c * 4 + 1);
                                                    std::uint8_t y = *(std::uint8_t *)(data + c * 4 + 2);
                                                    voxMap[(z + 1) + (x + 1) * sizeZ + (y + 1) * sizeX * sizeZ] = 0x80;
                                                }
                                                
                                                std::size_t optimizedCount = 0;
                                                for (int x = 1; x < sizeX - 1; x++) {
                                                    for (int y = 1; y < sizeY - 1; y++) {
                                                        for (int z = 1; z < sizeZ - 1; z++) {
                                                            if (voxMap[z + x * sizeZ + y * sizeX * sizeZ] & 0x80) {
                                                                std::uint8_t mask = 0b1111110;
                                                                
                                                                if (voxMap[(z - 1) + (x + 0) * sizeZ + (y + 0) * sizeX * sizeZ] & 0x80) {
                                                                    mask &= 0b1111100;
                                                                }
                                                                if (voxMap[(z + 0) + (x - 1) * sizeZ + (y + 0) * sizeX * sizeZ] & 0x80) {
                                                                    mask &= 0b1111010;
                                                                }
                                                                if (voxMap[(z + 0) + (x + 0) * sizeZ + (y - 1) * sizeX * sizeZ] & 0x80) {
                                                                    mask &= 0b1110110;
                                                                }
                                                                if (voxMap[(z + 1) + (x + 0) * sizeZ + (y + 0) * sizeX * sizeZ] & 0x80) {
                                                                    mask &= 0b1101110;
                                                                }
                                                                if (voxMap[(z + 0) + (x + 1) * sizeZ + (y + 0) * sizeX * sizeZ] & 0x80) {
                                                                    mask &= 0b1011110;
                                                                }
                                                                if (voxMap[(z + 0) + (x + 0) * sizeZ + (y + 1) * sizeX * sizeZ] & 0x80) {
                                                                    mask &= 0b0111110;
                                                                }
                                                                
                                                                voxMap[z + x * sizeZ + y * sizeX * sizeZ] = mask | 0x80;
                                                                
                                                                if (mask) {
                                                                    optimizedCount++;
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                                
                                                ctx.mesh->frames[i].voxels = std::make_unique<VoxelMesh::Voxel[]>(optimizedCount);
                                                ctx.mesh->frames[i].voxelCount = optimizedCount;
                                                
                                                std::size_t voxelIndex = 0;
                                                for (int x = 1; x < sizeX - 1; x++) {
                                                    for (int y = 1; y < sizeY - 1; y++) {
                                                        for (int z = 1; z < sizeZ - 1; z++) {
                                                            if (voxMap[z + x * sizeZ + y * sizeX * sizeZ] & 0x7f) {
                                                                VoxelMesh::Voxel &targetVoxel = ctx.mesh->frames[i].voxels[voxelIndex++];
                                                                targetVoxel.positionX = std::int16_t(x - 1);
                                                                targetVoxel.positionY = std::int16_t(y - 1);
                                                                targetVoxel.positionZ = std::int16_t(z - 1);
                                                                targetVoxel.scaleX = 1;
                                                                targetVoxel.scaleY = 1;
                                                                targetVoxel.scaleZ = 1;
                                                                targetVoxel.mask = voxMap[z + x * sizeZ + y * sizeX * sizeZ];
                                                            }
                                                        }
                                                    }
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
                                self->_asyncInProgress = false;

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
                                
                                while (self->_asyncInProgress == false && self->_callsQueue.size()) {
                                    QueueEntry &entry = self->_callsQueue.front();                                    
                                    self->getOrLoadVoxelMesh(entry.voxPath.data(), entry.scaled, std::move(entry.callback));
                                    self->_callsQueue.pop_front();
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
