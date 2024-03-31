
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
                                    if (*(std::int32_t *)(data + 4) == 0x96) { // vox made by Magica Voxel 0.98
                                        std::int32_t frameCount = 1;
                                        data += 20;
                                        
                                        if (memcmp(data, "PACK", 4) == 0) {
                                            frameCount = *(std::int32_t *)(data + 12);
                                            data += 16;
                                        }
                                        
                                        ctx.mesh = std::make_unique<VoxelMesh>();
                                        ctx.mesh->frames = std::make_unique<VoxelMesh::Frame[]>(frameCount);
                                        ctx.mesh->frameCount = frameCount;
                                        
                                        for (std::uint32_t f = 0; f < frameCount; f++) {
                                            if (memcmp(data, "SIZE", 4) == 0) {
                                                data += 12;
                                                int sizeZ = *(int *)(data + 0) + 2;
                                                int sizeX = *(int *)(data + 4) + 2;
                                                int sizeY = *(int *)(data + 8) + 2;
                                                data += 12;
                                                
                                                if (memcmp(data, "XYZI", 4) == 0) {
                                                    struct Cell {
                                                        std::uint8_t exist : 1;
                                                        std::uint8_t covered : 1;
                                                        std::uint8_t mask : 6;
                                                        std::uint8_t colorIndex;
                                                        std::uint8_t sx, sy, sz;
                                                    };
                                                    
                                                    std::unique_ptr<Cell[]> voxMap = std::make_unique<Cell[]>(sizeX * sizeY * sizeZ);
                                                    std::size_t voxelCount = *(std::uint32_t *)(data + 12);
                                                    
                                                    data += 16;
                                                    
                                                    for (std::size_t c = 0; c < voxelCount; c++) {
                                                        std::uint8_t z = *(std::uint8_t *)(data + c * 4 + 0);
                                                        std::uint8_t x = *(std::uint8_t *)(data + c * 4 + 1);
                                                        std::uint8_t y = *(std::uint8_t *)(data + c * 4 + 2);
                                                        std::uint8_t r = *(std::uint8_t *)(data + c * 4 + 3) - 1;
                                                        
                                                        int off = (z + 1) + (x + 1) * sizeZ + (y + 1) * sizeX * sizeZ;
                                                        voxMap[off].colorIndex = (31 - r / 8) * 8 + r % 8;
                                                        voxMap[off].exist = 1;
                                                        voxMap[off].mask = 0b111111;
                                                    }
                                                    
                                                    std::size_t visibleVoxelCount = 0;
                                                    
                                                    for (int y = 1; y <= sizeY - 2; y++) {
                                                        for (int x = 1; x <= sizeX - 2; x++) {
                                                            for (int z = 1; z <= sizeZ - 2; z++) {
                                                                int off = z + x * sizeZ + y * sizeX * sizeZ;
                                                                if (voxMap[off].exist) {
                                                                    std::uint8_t mask = 0b111111;
                                                                    
                                                                    if (voxMap[(z - 1) + (x + 0) * sizeZ + (y + 0) * sizeX * sizeZ].exist) {
                                                                        mask &= 0b111110;
                                                                    }
                                                                    if (voxMap[(z + 0) + (x - 1) * sizeZ + (y + 0) * sizeX * sizeZ].exist) {
                                                                        mask &= 0b111101;
                                                                    }
                                                                    if (voxMap[(z + 0) + (x + 0) * sizeZ + (y - 1) * sizeX * sizeZ].exist) {
                                                                        mask &= 0b111011;
                                                                    }
                                                                    if (voxMap[(z + 1) + (x + 0) * sizeZ + (y + 0) * sizeX * sizeZ].exist) {
                                                                        mask &= 0b110111;
                                                                    }
                                                                    if (voxMap[(z + 0) + (x + 1) * sizeZ + (y + 0) * sizeX * sizeZ].exist) {
                                                                        mask &= 0b101111;
                                                                    }
                                                                    if (voxMap[(z + 0) + (x + 0) * sizeZ + (y + 1) * sizeX * sizeZ].exist) {
                                                                        mask &= 0b011111;
                                                                    }
                                                                    
                                                                    voxMap[off].mask = mask;
                                                                    
                                                                    if (mask) {
                                                                        visibleVoxelCount++;
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                    
                                                    for (int y = 1; y <= sizeY - 2; y++) {
                                                        for (int x = 1; x <= sizeX - 2; x++) {
                                                            for (int z = 1; z <= sizeZ - 2; z++) {
                                                                int off = z + x * sizeZ + y * sizeX * sizeZ;
                                                                if (voxMap[off].mask == 0) {
                                                                    voxMap[off].exist = 0;
                                                                }
                                                            }
                                                        }
                                                    }
                                                    
                                                    ctx.mesh->frames[f].voxels = std::make_unique<VoxelMesh::Voxel[]>(visibleVoxelCount);
                                                    ctx.mesh->frames[f].voxelCount = 0; //areas.size();
                                                    
                                                    for (int x = 1; x < sizeX - 1; x++) {
                                                        for (int y = 1; y < sizeY - 1; y++) {
                                                            for (int z = 1; z < sizeZ - 1; z++) {
                                                                if (voxMap[z + x * sizeZ + y * sizeX * sizeZ].mask) {
                                                                    VoxelMesh::Voxel &targetVoxel = ctx.mesh->frames[f].voxels[ctx.mesh->frames[f].voxelCount++];
                                                                    int off = z + x * sizeZ + y * sizeX * sizeZ;
                                                                    targetVoxel.positionX = std::int16_t(x - 1);
                                                                    targetVoxel.positionY = std::int16_t(y - 1);
                                                                    targetVoxel.positionZ = std::int16_t(z - 1);
                                                                    targetVoxel.scaleX = 0;
                                                                    targetVoxel.scaleY = 0;
                                                                    targetVoxel.scaleZ = 0;
                                                                    targetVoxel.colorIndex = voxMap[off].colorIndex;
                                                                    targetVoxel.mask = std::uint8_t(voxMap[off].mask) << 0x1;
                                                                }
                                                            }
                                                        }
                                                    }
                                                    
                                                    data += voxelCount * 4;
                                                }
                                                else {
                                                    self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelMesh] XYZI[%d] chunk is not found in '%s'", f, path.data());
                                                    ctx.mesh = nullptr;
                                                    break;
                                                }
                                            }
                                            else {
                                                self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelMesh] SIZE[%d] chunk is not found in '%s'", f, path.data());
                                                ctx.mesh = nullptr;
                                                break;
                                            }
                                        }
                                    }
                                    else if (*(std::int32_t *)(data + 4) == 0x7f) { // vox made by gen_static_meshes.py
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
