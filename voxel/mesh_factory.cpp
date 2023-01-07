
#include "mesh_factory.h"

#include "thirdparty/upng/upng.h"
#include "foundation/gears/parsing.h"

namespace voxel {
    class MeshFactoryImpl : public std::enable_shared_from_this<MeshFactoryImpl>, public MeshFactory {
    public:
        MeshFactoryImpl(const foundation::PlatformInterfacePtr &platform);
        ~MeshFactoryImpl() override;
        
        bool createMesh(const char *resourcePath, const int16_t(&offset)[3], Mesh &output) override;
        
    private:
        std::shared_ptr<foundation::PlatformInterface> _platform;
    };
    
    MeshFactoryImpl::MeshFactoryImpl(const std::shared_ptr<foundation::PlatformInterface> &platform) : _platform(platform) {

    }
    
    MeshFactoryImpl::~MeshFactoryImpl() {

    }
    
    bool MeshFactoryImpl::createMesh(const char *voxPath, const int16_t(&offset)[3], Mesh &output) {
        const std::int32_t version = 150;
        
        std::unique_ptr<std::uint8_t[]> voxData;
        std::size_t voxSize = 0;

        if (_platform->loadFile(voxPath, voxData, voxSize)) {
            std::uint8_t *data = voxData.get();
            
            if (memcmp(data, "VOX ", 4) == 0 && *(std::int32_t *)(data + 4) == version) {
                // skip bytes of main chunk to start of the first child ('PACK')
                data += 20;
                std::int32_t frameCount = 1;
                
                if (memcmp(data, "PACK", 4) == 0) {
                    frameCount = *(std::int32_t *)(data + 12);
                    data += 16;
                }
                
                output.frames = std::make_unique<Mesh::Frame[]>(frameCount);
                output.frameCount = frameCount;
                
                for (std::int32_t i = 0; i < frameCount; i++) {
                    if (memcmp(data, "SIZE", 4) == 0) {
//                        std::uint16_t sizeZ = *(std::uint16_t *)(data + 12);
//                        std::uint16_t sizeX = *(std::uint16_t *)(data + 16);
//                        std::uint16_t sizeY = *(std::uint16_t *)(data + 20);
                        
//                        struct InterVoxel {
//                            std::uint8_t isExist;
//                        };
//
//                        std::unique_ptr<InterVoxel[]> voxelArray = std::make_unique<InterVoxel[]>(sizeX * sizeY * sizeZ);
//                        std::fill_n(voxelArray.get(), sizeX * sizeY * sizeZ, InterVoxel{0});
//                        auto arrayIndex = [&](std::uint8_t x, std::uint8_t y, std::uint8_t z) {
//                            return x + y * sizeX + z * sizeY * sizeX;
//                        };
                        
                        data += 24;
                        
                        if (memcmp(data, "XYZI", 4) == 0) {
                            std::size_t voxelCount = *(std::uint32_t *)(data + 12);
                            
                            data += 16;
                            
                            output.frames[i].voxels = std::make_unique<VoxelInfo[]>(voxelCount);
                            output.frames[i].voxelCount = voxelCount;
                            
                            for (std::size_t c = 0, k = 0; c < voxelCount; c++) {
                                std::uint8_t z = *(std::uint8_t *)(data + c * 4 + 0);
                                std::uint8_t x = *(std::uint8_t *)(data + c * 4 + 1);
                                std::uint8_t y = *(std::uint8_t *)(data + c * 4 + 2);
                                
                                VoxelInfo &targetVoxel = output.frames[i].voxels[k++];
                                
                                targetVoxel.positionZ = std::int16_t(z) + offset[2];
                                targetVoxel.positionX = std::int16_t(x) + offset[0];
                                targetVoxel.positionY = std::int16_t(y) + offset[1];
                                targetVoxel.colorIndex = *(std::uint8_t *)(data + c * 4 + 3);
                            }
                            
                            //data += voxelCount * 4;
                            return true;
                        }
                        else {
                            _platform->logError("[voxel::createMesh] XYZI[%d] chunk is not found in '%s'", i, voxPath);
                            break;
                        }
                    }
                    else {
                        _platform->logError("[voxel::createMesh] SIZE[%d] chunk is not found in '%s'", i, voxPath);
                        break;
                    }
                }
            }
            else {
                _platform->logError("[voxel::createMesh] Incorrect vox-header in '%s'", voxPath);
            }
        }
        else {
            _platform->logError("[voxel::createMesh] Unable to find file '%s'", voxPath);
        }

        return false;
    }
}

namespace voxel {
    std::shared_ptr<MeshFactory> MeshFactory::instance(const std::shared_ptr<foundation::PlatformInterface> &platform) {
        return std::make_shared<MeshFactoryImpl>(platform);
    }
}
