
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
                        std::uint8_t sizeZ = *(std::uint8_t *)(data + 12);
                        std::uint8_t sizeX = *(std::uint8_t *)(data + 16);
                        std::uint8_t sizeY = *(std::uint8_t *)(data + 20);
                        
                        struct InterVoxel {
                            std::uint8_t isExist : 1;
                            std::uint8_t isMesh : 1;
                        };
                        
                        std::unique_ptr<InterVoxel[]> voxelArray = std::make_unique<InterVoxel[]>(sizeX * sizeY * sizeZ);
                        std::fill_n(voxelArray.get(), sizeX * sizeY * sizeZ, InterVoxel{0});
                        auto arrayIndex = [&](std::uint8_t x, std::uint8_t y, std::uint8_t z) {
                            return x + y * sizeX + z * sizeY * sizeX;
                        };
                        
                        data += 24;
                        
                        if (memcmp(data, "XYZI", 4) == 0) {
                            std::size_t voxelCount = *(std::uint32_t *)(data + 12);
                            
                            data += 16;
                            std::uint16_t meshVoxelCount = 0;
                            
                            for (std::size_t c = 0; c < voxelCount; c++) {
                                std::uint8_t z = *(std::uint8_t *)(data + c * 4 + 0);
                                std::uint8_t x = *(std::uint8_t *)(data + c * 4 + 1);
                                std::uint8_t y = *(std::uint8_t *)(data + c * 4 + 2);
                                
                                voxelArray[arrayIndex(x, y, z)].isExist = 1;
                                
                                // TODO: remove invisible voxels
                                
                                //if (x > 0 && x < sizeX - 1 && y > 0 && y < sizeY - 1 && z > 0 && z < sizeZ - 1) {
                                    voxelArray[arrayIndex(x, y, z)].isMesh = 1;
                                    meshVoxelCount++;
                                //}
                            }
                            
                            output.frames[i].positions = std::make_unique<VoxelPosition[]>(meshVoxelCount);
                            output.frames[i].voxels = std::make_unique<VoxelInfo[]>(meshVoxelCount);
                            output.frames[i].voxelCount = meshVoxelCount;
                            
                            for (std::size_t c = 0, k = 0; c < voxelCount; c++) {
                                std::uint8_t z = *(std::uint8_t *)(data + c * 4 + 0);
                                std::uint8_t x = *(std::uint8_t *)(data + c * 4 + 1);
                                std::uint8_t y = *(std::uint8_t *)(data + c * 4 + 2);
                                
                                if (voxelArray[arrayIndex(x, y, z)].isMesh) {
                                    VoxelPosition &targetPosition = output.frames[i].positions[k];
                                    VoxelInfo &targetVoxel = output.frames[i].voxels[k++];
                                    
                                    targetVoxel.positionZ = std::int16_t(z) + offset[2];
                                    targetVoxel.positionX = std::int16_t(x) + offset[0];
                                    targetVoxel.positionY = std::int16_t(y) + offset[1];
                                    targetVoxel.colorIndex = *(std::uint8_t *)(data + c * 4 + 3);
                                    std::fill_n(targetVoxel.lightFaceNX, 24, 0xFF);
                                    
                                    targetPosition.positionX = targetVoxel.positionX;
                                    targetPosition.positionY = targetVoxel.positionY;
                                    targetPosition.positionZ = targetVoxel.positionZ;
                                    
                                    struct MultiplySign {
                                        std::int8_t a, b;
                                    };
                                    
                                    MultiplySign offsets[] = {{-1, -1}, {+1, -1}, {-1, +1}, {+1, +1}};
                                    std::int8_t faceSign[] = {-1, 1};
                                    
                                    const std::uint8_t CORNER_EXIST = 0x80;
                                    
                                    for (std::size_t s = 0; s < 2; s++) {
                                        std::uint8_t *lightFaceX = targetVoxel.lightFaceNX + s * 4;
                                        std::uint8_t *lightFaceY = targetVoxel.lightFaceNY + s * 4;
                                        std::uint8_t *lightFaceZ = targetVoxel.lightFaceNZ + s * 4;
                                        
                                        for (std::size_t f = 0; f < 4; f++) {
                                            std::uint8_t config = 0;
                                            
                                            auto evalReducedLight = [](std::uint8_t config) {
                                                std::uint8_t reducedLight = 0;
                                                if ((config & 0xf) > 1) reducedLight = 200;
                                                else if ((config & 0xf) && (config & CORNER_EXIST)) reducedLight = 160;
                                                else if (config == 1 || config == CORNER_EXIST) reducedLight = 130;
                                                return reducedLight;
                                            };
                                            
                                            if (voxelArray[arrayIndex(x + 0 * offsets[f].a, y + faceSign[s], z + 1 * offsets[f].b)].isExist) config++;
                                            if (voxelArray[arrayIndex(x + 1 * offsets[f].a, y + faceSign[s], z + 0 * offsets[f].b)].isExist) config++;
                                            if (voxelArray[arrayIndex(x + 1 * offsets[f].a, y + faceSign[s], z + 1 * offsets[f].b)].isExist) config |= CORNER_EXIST;
                                            lightFaceY[f] -= evalReducedLight(config);
                                            
                                            config = 0;
                                            if (voxelArray[arrayIndex(x + 0 * offsets[f].a, y + 1 * offsets[f].b, z + faceSign[s])].isExist) config++;
                                            if (voxelArray[arrayIndex(x + 1 * offsets[f].a, y + 0 * offsets[f].b, z + faceSign[s])].isExist) config++;
                                            if (voxelArray[arrayIndex(x + 1 * offsets[f].a, y + 1 * offsets[f].b, z + faceSign[s])].isExist) config |= CORNER_EXIST;
                                            lightFaceZ[f] -= evalReducedLight(config);
                                            
                                            config = 0;
                                            if (voxelArray[arrayIndex(x + faceSign[s], y + 0 * offsets[f].b, z + 1 * offsets[f].a)].isExist) config++;
                                            if (voxelArray[arrayIndex(x + faceSign[s], y + 1 * offsets[f].b, z + 0 * offsets[f].a)].isExist) config++;
                                            if (voxelArray[arrayIndex(x + faceSign[s], y + 1 * offsets[f].b, z + 1 * offsets[f].a)].isExist) config |= CORNER_EXIST;
                                            lightFaceX[f] -= evalReducedLight(config);
                                        }
                                    }
                                }
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
