
#include "mesh_factory.h"

#include "thirdparty/upng/upng.h"
#include "foundation/gears/parsing.h"

namespace {
    // XFace Indeces : [-y-z, -y+z, +y-z, +y+z]
    // YFace Indeces : [-z-x, -z+x, +z-x, +z+x]
    // ZFace Indeces : [-y-x, -y+x, +y-x, +y+x]
//    const char *g_staticMeshShaderSrc = R"(
//        fixed {
//            axis[3] : float4 =
//                [0.0, 0.0, 1.0, 0.0]
//                [1.0, 0.0, 0.0, 0.0]
//                [0.0, 1.0, 0.0, 0.0]
//
//            cube[12] : float4 =
//                [-0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
//                [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0][0.5, 0.5, -0.5, 1.0]
//                [0.5, 0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0]
//
//            faceUV[4] : float2 = [0.0, 0.0][1.0, 0.0][0.0, 1.0][1.0, 1.0]
//        }
//        inout {
//            texcoord : float2
//            koeff : float4
//        }
//        vssrc {
//            float3 cubeCenter = float3(instance_position_color.xyz);
//            float3 toCamera = frame_cameraPosition.xyz - cubeCenter;
//            float3 toCamSign = _sign(toCamera);
//            float4 relVertexPos = float4(toCamSign, 1.0) * fixed_cube[vertex_ID];
//            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
//            float3 faceNormal = toCamSign * fixed_axis[vertex_ID >> 2].xyz;
//
//            int3 faceIndeces = int3(float3(2.0, 2.0, 2.0) * step(0.0, relVertexPos.yzy) + step(0.0, relVertexPos.zxx));
//
//            float4 koeff = float4(0.0, 0.0, 0.0, 0.0);
//            koeff = koeff + step(0.5, -faceNormal.x) * instance_lightFaceNX;
//            koeff = koeff + step(0.5,  faceNormal.x) * instance_lightFacePX;
//            koeff = koeff + step(0.5, -faceNormal.y) * instance_lightFaceNY;
//            koeff = koeff + step(0.5,  faceNormal.y) * instance_lightFacePY;
//            koeff = koeff + step(0.5, -faceNormal.z) * instance_lightFaceNZ;
//            koeff = koeff + step(0.5,  faceNormal.z) * instance_lightFacePZ;
//
//            float2 texcoord = float2(0.0, 0.0);
//            texcoord = texcoord + step(0.5, abs(faceNormal.x)) * fixed_faceUV[faceIndeces.x];
//            texcoord = texcoord + step(0.5, abs(faceNormal.y)) * fixed_faceUV[faceIndeces.y];
//            texcoord = texcoord + step(0.5, abs(faceNormal.z)) * fixed_faceUV[faceIndeces.z];
//
//            output_position = _transform(absVertexPos, frame_viewProjMatrix);
//            output_texcoord = texcoord;
//            output_koeff = koeff;
//        }
//        fssrc {
//            float m0 = mix(input_koeff[0], input_koeff[1], input_texcoord.x);
//            float m1 = mix(input_koeff[2], input_koeff[3], input_texcoord.x);
//            float k = mix(m0, m1, input_texcoord.y);
//            output_color = float4(k, k, k, 1.0);
//        }
//    )";
//
//    // XFace Indeces : [-y-z, -y+z, +y-z, +y+z]
//    // YFace Indeces : [-z-x, -z+x, +z-x, +z+x]
//    // ZFace Indeces : [-y-x, -y+x, +y-x, +y+x]
//    const char *g_dynamicMeshShaderSrc = R"(
//        fixed {
//            axis[3] : float4 =
//                [0.0, 0.0, 1.0, 0.0]
//                [1.0, 0.0, 0.0, 0.0]
//                [0.0, 1.0, 0.0, 0.0]
//
//            cube[12] : float4 =
//                [-0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
//                [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0][0.5, 0.5, -0.5, 1.0]
//                [0.5, 0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0]
//
//            faceUV[4] : float2 = [0.0, 0.0][1.0, 0.0][0.0, 1.0][1.0, 1.0]
//        }
//        const {
//            modelTransform : matrix4
//        }
//        inout {
//            texcoord : float2
//            koeff : float4
//        }
//        vssrc {
//            float3 cubeCenter = float3(instance_position_color.xyz);
//            float3 toCamera = frame_cameraPosition.xyz - _transform(float4(cubeCenter, 1.0), const_modelTransform).xyz;
//            float3 toCamSign = _sign(_transform(const_modelTransform, float4(toCamera, 0.0)).xyz);
//            float4 relVertexPos = float4(toCamSign, 1.0) * fixed_cube[vertex_ID];
//            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
//            float3 faceNormal = toCamSign * fixed_axis[vertex_ID >> 2].xyz;
//
//            int3 faceIndeces = int3(float3(2.0, 2.0, 2.0) * step(0.0, relVertexPos.yzy) + step(0.0, relVertexPos.zxx));
//
//            //int xFaceIndex = int(2.0 * step(0.0, relVertexPos.y) + step(0.0, relVertexPos.z));
//            //int yFaceIndex = int(2.0 * step(0.0, relVertexPos.z) + step(0.0, relVertexPos.x));
//            //int zFaceIndex = int(2.0 * step(0.0, relVertexPos.y) + step(0.0, relVertexPos.x));
//
//            float4 koeff = float4(0.0, 0.0, 0.0, 0.0);
//            koeff = koeff + step(0.5, -faceNormal.x) * instance_lightFaceNX;
//            koeff = koeff + step(0.5,  faceNormal.x) * instance_lightFacePX;
//            koeff = koeff + step(0.5, -faceNormal.y) * instance_lightFaceNY;
//            koeff = koeff + step(0.5,  faceNormal.y) * instance_lightFacePY;
//            koeff = koeff + step(0.5, -faceNormal.z) * instance_lightFaceNZ;
//            koeff = koeff + step(0.5,  faceNormal.z) * instance_lightFacePZ;
//
//            float2 texcoord = float2(0.0, 0.0);
//            texcoord = texcoord + step(0.5, abs(faceNormal.x)) * fixed_faceUV[faceIndeces.x];
//            texcoord = texcoord + step(0.5, abs(faceNormal.y)) * fixed_faceUV[faceIndeces.y];
//            texcoord = texcoord + step(0.5, abs(faceNormal.z)) * fixed_faceUV[faceIndeces.z];
//
//            output_position = _transform(_transform(absVertexPos, const_modelTransform), frame_viewProjMatrix);
//            output_texcoord = texcoord;
//            output_koeff = koeff;
//        }
//        fssrc {
//            float m0 = mix(input_koeff[0], input_koeff[1], input_texcoord.x);
//            float m1 = mix(input_koeff[2], input_koeff[3], input_texcoord.x);
//            float k = mix(m0, m1, input_texcoord.y);
//            output_color = float4(k, k, k, 1.0);
//        }
//    )";
}

namespace voxel {
/*
    struct Chunk {
        std::vector<Voxel> voxels;
        uint16_t modelBounds[3];
    };

    std::vector<Chunk> loadModel(const std::shared_ptr<foundation::PlatformInterface> &platform, const char *fullPath, const int16_t(&offset)[3]) {
        const std::int32_t version = 150;

        std::vector<Chunk> result;
        std::unique_ptr<std::uint8_t[]> voxData;
        std::size_t voxSize = 0;

        if (platform->loadFile(fullPath, voxData, voxSize)) {
            std::uint8_t *data = voxData.get();

            if (memcmp(data, "VOX ", 4) == 0 && *(std::int32_t *)(data + 4) == version) {
                // skip bytes of main chunk to start of the first child ('PACK')
                data += 20;
                std::int32_t modelCount = 1;

                if (memcmp(data, "PACK", 4) == 0) {
                    std::int32_t modelCount = *(std::int32_t *)(data + 12);

                    data += 16;
                }

                result.resize(modelCount);

                for (std::int32_t i = 0; i < modelCount; i++) {
                    if (memcmp(data, "SIZE", 4) == 0) {
                        std::uint8_t sizeZ = *(std::uint8_t *)(data + 12);
                        std::uint8_t sizeX = *(std::uint8_t *)(data + 16);
                        std::uint8_t sizeY = *(std::uint8_t *)(data + 20);

                        result[i].modelBounds[0] = sizeX;
                        result[i].modelBounds[1] = sizeY;
                        result[i].modelBounds[2] = sizeZ;

                        data += 24;

                        if (memcmp(data, "XYZI", 4) == 0) {
                            std::size_t voxelCount = *(std::uint32_t *)(data + 12);

                            data += 16;
                            result[i].voxels.resize(voxelCount);

                            for (std::size_t c = 0; c < voxelCount; c++) {
                                result[i].voxels[c].positionZ = std::int16_t(*(std::uint8_t *)(data + c * 4 + 0)) - offset[2];
                                result[i].voxels[c].positionX = std::int16_t(*(std::uint8_t *)(data + c * 4 + 1)) - offset[0];
                                result[i].voxels[c].positionY = std::int16_t(*(std::uint8_t *)(data + c * 4 + 2)) + offset[1];
                                result[i].voxels[c].colorIndex = *(std::uint8_t *)(data + c * 4 + 3) - 1;
                            }

                            data += voxelCount * 4;
                        }
                        else {
                            platform->logError("[voxel::loadModel] XYZI[%d] chunk is not found in '%s'", i, fullPath);
                            break;
                        }
                    }
                    else {
                        platform->logError("[voxel::loadModel] SIZE[%d] chunk is not found in '%s'", i, fullPath);
                        break;
                    }
                }
            }
            else {
                platform->logError("[voxel::loadModel] Incorrect vox-header in '%s'", fullPath);
            }
        }
        else {
            platform->logError("[voxel::loadModel] Unable to find file '%s'", fullPath);
        }

        return result;
    }
    */
}

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
                                
                                if (x > 0 && x < sizeX - 1 && y > 0 && y < sizeY - 1 && z > 0 && z < sizeZ - 1) {
                                    voxelArray[arrayIndex(x, y, z)].isMesh = 1;
                                    meshVoxelCount++;
                                }
                            }

                            output.frames[i].voxels = std::make_unique<Voxel[]>(meshVoxelCount);
                            output.frames[i].voxelCount = meshVoxelCount;

                            for (std::size_t c = 0, k = 0; c < voxelCount; c++) {
                                std::uint8_t z = *(std::uint8_t *)(data + c * 4 + 0);
                                std::uint8_t x = *(std::uint8_t *)(data + c * 4 + 1);
                                std::uint8_t y = *(std::uint8_t *)(data + c * 4 + 2);
                                
                                if (voxelArray[arrayIndex(x, y, z)].isMesh) {
                                    Voxel &targetVoxel = output.frames[i].voxels[k++];
                                    
                                    targetVoxel.positionZ = std::int16_t(z) + offset[2] - 1;
                                    targetVoxel.positionX = std::int16_t(x) + offset[0] - 1;
                                    targetVoxel.positionY = std::int16_t(y) + offset[1] - 1;
                                    targetVoxel.colorIndex = *(std::uint8_t *)(data + c * 4 + 3) - 1;
                                    std::fill_n(targetVoxel.lightFaceNX, 24, 0xFF);
                                    
                                    struct MultiplySign {
                                        std::int8_t a, b;
                                    };
                                    
                                    MultiplySign offsets[] = {{-1, -1}, {+1, -1}, {-1, +1}, {+1, +1}};
                                    std::int8_t faceSign[] = {-1, 1};
                                    
                                    const std::uint8_t STEP = 72;
                                    
                                    for (std::size_t s = 0; s < 2; s++) {
                                        std::uint8_t *lightFaceX = targetVoxel.lightFaceNX + s * 4;
                                        std::uint8_t *lightFaceY = targetVoxel.lightFaceNY + s * 4;
                                        std::uint8_t *lightFaceZ = targetVoxel.lightFaceNZ + s * 4;
                                        
                                        for (std::size_t f = 0; f < 4; f++) {
                                            std::uint8_t reducedLight = 0;
                                            if (voxelArray[arrayIndex(x + 0 * offsets[f].a, y + faceSign[s], z + 1 * offsets[f].b)].isExist) reducedLight += STEP;
                                            if (voxelArray[arrayIndex(x + 1 * offsets[f].a, y + faceSign[s], z + 0 * offsets[f].b)].isExist) reducedLight += STEP;
                                            if (voxelArray[arrayIndex(x + 1 * offsets[f].a, y + faceSign[s], z + 1 * offsets[f].b)].isExist) reducedLight = std::max(STEP, reducedLight);
                                            lightFaceY[f] -= reducedLight;

                                            reducedLight = 0;
                                            if (voxelArray[arrayIndex(x + 0 * offsets[f].a, y + 1 * offsets[f].b, z + faceSign[s])].isExist) reducedLight += STEP;
                                            if (voxelArray[arrayIndex(x + 1 * offsets[f].a, y + 0 * offsets[f].b, z + faceSign[s])].isExist) reducedLight += STEP;
                                            if (voxelArray[arrayIndex(x + 1 * offsets[f].a, y + 1 * offsets[f].b, z + faceSign[s])].isExist) reducedLight = std::max(STEP, reducedLight);
                                            lightFaceZ[f] -= reducedLight;

                                            reducedLight = 0;
                                            if (voxelArray[arrayIndex(x + faceSign[s], y + 0 * offsets[f].b, z + 1 * offsets[f].a)].isExist) reducedLight += STEP;
                                            if (voxelArray[arrayIndex(x + faceSign[s], y + 1 * offsets[f].b, z + 0 * offsets[f].a)].isExist) reducedLight += STEP;
                                            if (voxelArray[arrayIndex(x + faceSign[s], y + 1 * offsets[f].b, z + 1 * offsets[f].a)].isExist) reducedLight = std::max(STEP, reducedLight);
                                            lightFaceX[f] -= reducedLight;
                                        }
                                    }
                                }
                            }

                            data += voxelCount * 4;
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
