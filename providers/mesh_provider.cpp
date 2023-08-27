
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
        void getOrLoadVoxelMesh(const char *voxPath, MeshOptimization optimization, util::callback<void(const std::unique_ptr<VoxelMesh> &)> &&completion) override;
        void update(float dtSec) override;
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        std::unordered_map<std::string, MeshInfo> _meshInfos;
        std::unordered_map<std::string, std::unique_ptr<VoxelMesh>> _meshes;
        
        struct QueueEntry {
            MeshOptimization optimization;
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

    void MeshProviderImpl::getOrLoadVoxelMesh(const char *voxPath, MeshOptimization optimization, util::callback<void(const std::unique_ptr<VoxelMesh> &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueue.emplace_back(QueueEntry {
                .optimization = optimization,
                .voxPath = voxPath,
                .callback = std::move(completion)
            });
            
            return;
        }
        
        std::string path = std::string(voxPath);

        auto index = _meshes.find(path + "_" + std::to_string(int(optimization)));
        if (index != _meshes.end()) {
            completion(index->second);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".vox").data(), [weak = weak_from_this(), path, completion = std::move(completion), optimization](std::unique_ptr<uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<MeshProviderImpl> self = weak.lock()) {
                    const foundation::PlatformInterfacePtr &platform = self->_platform;
                    if (len) {
                        struct AsyncContext {
                            std::unique_ptr<VoxelMesh> mesh;
                        };
                        
                        self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak, path, bin = std::move(mem), len, optimization](AsyncContext &ctx) {
                            if (std::shared_ptr<MeshProviderImpl> self = weak.lock()) {
                                const std::int32_t version = 150;
                                const std::uint8_t *data = bin.get();
                                
                                if (memcmp(data, "VOX ", 4) == 0 && *(std::int32_t *)(data + 4) == version) {
                                    data += 20; // skip bytes of main chunk to start of the first child ('PACK')
                                    std::int32_t frameCount = 1;
                                    
                                    if (memcmp(data, "PACK", 4) == 0) {
                                        frameCount = *(std::int32_t *)(data + 12);
                                        data += 16;
                                    }
                                    
                                    ctx.mesh = std::make_unique<VoxelMesh>();
                                    ctx.mesh->frames = std::make_unique<VoxelMesh::Frame[]>(frameCount);
                                    ctx.mesh->frameCount = frameCount;
                                    
                                    for (std::int32_t f = 0; f < frameCount; f++) {
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
                                                    std::uint8_t r = *(std::uint8_t *)(data + c * 4 + 3);
                                                    
                                                    int off = (z + 1) + (x + 1) * sizeZ + (y + 1) * sizeX * sizeZ;
                                                    voxMap[off].colorIndex = r;
                                                    voxMap[off].exist = 1;
                                                    voxMap[off].mask = 0b111111;
                                                }
                                                
                                                std::size_t visibleVoxelCount = voxelCount;

                                                if (optimization == MeshOptimization::VISIBLE || optimization == MeshOptimization::OPTIMIZED) {
                                                    visibleVoxelCount = 0;
                                                    
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
                                                }
                                                
                                                if (optimization == MeshOptimization::OPTIMIZED) {
                                                    auto same = [](Cell &a, Cell &b) {
                                                        return a.mask == b.mask && a.colorIndex == b.colorIndex;
                                                    };
                                                    
                                                    for (int y = 1; y <= sizeY - 2; y++) {
                                                        for (int x = 1; x <= sizeX - 2; x++) {
                                                            for (int z = 1; z <= sizeZ - 2; z++) {
                                                                int off = z + x * sizeZ + y * sizeX * sizeZ;
                                                                int ppX = (z - 0) + (x - 1) * sizeZ + (y - 0) * sizeX * sizeZ;
                                                                int ppY = (z - 0) + (x - 0) * sizeZ + (y - 1) * sizeX * sizeZ;
                                                                int ppZ = (z - 1) + (x - 0) * sizeZ + (y - 0) * sizeX * sizeZ;
                                                                int pXY = (z - 0) + (x - 1) * sizeZ + (y - 1) * sizeX * sizeZ;
                                                                int pYZ = (z - 1) + (x - 0) * sizeZ + (y - 1) * sizeX * sizeZ;
                                                                int pXZ = (z - 1) + (x - 1) * sizeZ + (y - 0) * sizeX * sizeZ;

                                                                std::uint8_t sX0 = 1, sY0 = 1, sZ0 = 1;
                                                                std::uint8_t sX1 = 1, sY1 = 1, sZ1 = 1;

                                                                if (voxMap[off].exist) {
                                                                    // XY
                                                                    if (same(voxMap[pXY], voxMap[off])) {
                                                                        sX0 = std::min(voxMap[pXY].sx, same(voxMap[ppX], voxMap[off]) ? voxMap[ppX].sx : std::uint8_t(0)) + 1;
                                                                        sY0 = std::min(voxMap[pXY].sy, same(voxMap[ppY], voxMap[off]) ? voxMap[ppY].sy : std::uint8_t(0)) + 1;
                                                                    }
                                                                    else {
                                                                        if (same(voxMap[ppX], voxMap[off]) && same(voxMap[ppY], voxMap[off])) {
                                                                            if (voxMap[ppX].sx > voxMap[ppY].sy) {
                                                                                sX0 = voxMap[ppX].sx + 1;
                                                                            }
                                                                            else {
                                                                                sY0 = voxMap[ppY].sy + 1;
                                                                            }
                                                                        }
                                                                        else if (same(voxMap[ppX], voxMap[off])) {
                                                                            sX0 = voxMap[ppX].sx + 1;
                                                                        }
                                                                        else if (same(voxMap[ppY], voxMap[off])) {
                                                                            sY0 = voxMap[ppY].sy + 1;
                                                                        }
                                                                    }

                                                                    // YZ
                                                                    if (same(voxMap[pYZ], voxMap[off])) {
                                                                        sY1 = std::min(voxMap[pYZ].sy, same(voxMap[ppY], voxMap[off]) ? voxMap[ppY].sy : std::uint8_t(0)) + 1;
                                                                        sZ0 = std::min(voxMap[pYZ].sz, same(voxMap[ppZ], voxMap[off]) ? voxMap[ppZ].sz : std::uint8_t(0)) + 1;
                                                                    }
                                                                    else {
                                                                        if (same(voxMap[ppY], voxMap[off]) && same(voxMap[ppZ], voxMap[off])) {
                                                                            if (voxMap[ppY].sy > voxMap[ppZ].sz) {
                                                                                sY1 = voxMap[ppY].sy + 1;
                                                                            }
                                                                            else {
                                                                                sZ0 = voxMap[ppZ].sz + 1;
                                                                            }
                                                                        }
                                                                        else if (same(voxMap[ppY], voxMap[off])) {
                                                                            sY1 = voxMap[ppY].sy + 1;
                                                                        }
                                                                        else if (same(voxMap[ppZ], voxMap[off])) {
                                                                            sZ0 = voxMap[ppZ].sz + 1;
                                                                        }
                                                                    }
                                                                    
                                                                    // XZ
                                                                    if (same(voxMap[pXZ], voxMap[off])) {
                                                                        sX1 = std::min(voxMap[pXZ].sx, same(voxMap[ppX], voxMap[off]) ? voxMap[ppX].sx : std::uint8_t(0)) + 1;
                                                                        sZ1 = std::min(voxMap[pXZ].sz, same(voxMap[ppZ], voxMap[off]) ? voxMap[ppZ].sz : std::uint8_t(0)) + 1;
                                                                    }
                                                                    else {
                                                                        if (same(voxMap[ppX], voxMap[off]) && same(voxMap[ppZ], voxMap[off])) {
                                                                            if (voxMap[ppX].sx > voxMap[ppZ].sz) {
                                                                                sX1 = voxMap[ppX].sx + 1;
                                                                            }
                                                                            else {
                                                                                sZ1 = voxMap[ppZ].sz + 1;
                                                                            }
                                                                        }
                                                                        else if (same(voxMap[ppX], voxMap[off])) {
                                                                            sX1 = voxMap[ppX].sx + 1;
                                                                        }
                                                                        else if (same(voxMap[ppZ], voxMap[off])) {
                                                                            sZ1 = voxMap[ppZ].sz + 1;
                                                                        }
                                                                    }
                                                                    
                                                                    voxMap[off].sx = std::min(sX0, sX1);
                                                                    voxMap[off].sy = std::min(sY0, sY1);
                                                                    voxMap[off].sz = std::min(sZ0, sZ1);
                                                                }
                                                            }
                                                        }
                                                    }

                                                    struct Area {
                                                        int x, y, z;
                                                        int sx, sy, sz;
                                                    };
                                                    auto makeArea = [](std::vector<Area> &out, Cell *voxMap, int x, int y, int z, int sizeX, int sizeY, int sizeZ) {
                                                        int off = z + x * sizeZ + y * sizeX * sizeZ;
                                                        int sx = voxMap[off].sx;
                                                        int sy = voxMap[off].sy;
                                                        int sz = voxMap[off].sz;
                                                        Area result = {};
                                                        
                                                        for (int ay = y; ay > y - sy; ay--) {
                                                            for (int ax = x; ax > x - sx; ax--) {
                                                                for (int az = z; az > z - sz; az--) {
                                                                    int cur = az + ax * sizeZ + ay * sizeX * sizeZ;
                                                                    
                                                                    if (voxMap[cur].covered == 0) {
                                                                        voxMap[cur].covered = 0x1;
                                                                        result = Area{ax, ay, az, x - ax + 1, y - ay + 1, z - az + 1};
                                                                    }
                                                                }
                                                            }
                                                        }

                                                        if (result.sx && result.sy && result.sz) {
                                                            out.emplace_back(result);
                                                        }
                                                    };
                                                    
                                                    std::vector<Area> areas;
                                                    
                                                    for (int y = sizeY - 2; y >= 1; y--) {
                                                        for (int x = sizeX - 2; x >= 1; x--) {
                                                            for (int z = sizeZ - 2; z >= 1; z--) {
                                                                int off = z + x * sizeZ + y * sizeX * sizeZ;
                                                                if (voxMap[off].exist && voxMap[off].covered == 0) {
                                                                    makeArea(areas, voxMap.get(), x, y, z, sizeX, sizeY, sizeZ);
                                                                }
                                                            }
                                                        }
                                                    }

                                                    ctx.mesh->frames[f].voxels = std::make_unique<VoxelMesh::Voxel[]>(areas.size());
                                                    ctx.mesh->frames[f].voxelCount = 0; //areas.size();
                                                    
                                                    for (const auto &item : areas) {
                                                        VoxelMesh::Voxel &targetVoxel = ctx.mesh->frames[f].voxels[ctx.mesh->frames[f].voxelCount++];
                                                        int off = item.z + item.x * sizeZ + item.y * sizeX * sizeZ;
                                                        targetVoxel.positionX = std::int16_t(item.x - 1);
                                                        targetVoxel.positionY = std::int16_t(item.y - 1);
                                                        targetVoxel.positionZ = std::int16_t(item.z - 1);
                                                        targetVoxel.scaleX = (item.sx - 1);
                                                        targetVoxel.scaleY = (item.sy - 1);
                                                        targetVoxel.scaleZ = (item.sz - 1);
                                                        targetVoxel.colorIndex = voxMap[off].colorIndex;
                                                        targetVoxel.mask = std::uint8_t(voxMap[off].mask) << 0x1;
                                                    }
                                                }
                                                else {  // do not optimize
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
                                else {
                                    self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelMesh] Incorrect vox-header in '%s'", path.data());
                                }
                            }
                        },
                        [weak, path, optimization, completion = std::move(completion)](AsyncContext &ctx) {
                            if (std::shared_ptr<MeshProviderImpl> self = weak.lock()) {
                                self->_asyncInProgress = false;

                                if (ctx.mesh) {
                                    const std::unique_ptr<VoxelMesh> &mesh = self->_meshes.emplace(path + "_" + std::to_string(int(optimization)), std::move(ctx.mesh)).first->second;
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
            getOrLoadVoxelMesh(entry.voxPath.data(), entry.optimization, std::move(entry.callback));
            _callsQueue.pop_front();
        }
    }
}

namespace resource {
    std::shared_ptr<MeshProvider> MeshProvider::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const char *resourceList) {
        return std::make_shared<MeshProviderImpl>(platform, resourceList);
    }
}
