
#include "interfaces.h"
#include "mesh_factory.h"

#include "foundation/parsing.h"

#include <sstream>
#include <iomanip>

namespace voxel {
    std::vector<Chunk> loadModel(const std::shared_ptr<foundation::PlatformInterface> &platform, const char *fullPath, const int16_t(&offset)[3], bool centered, int16_t(&modelBounds)[3]) {
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
                        std::int8_t sizeZ = *(std::int8_t *)(data + 12);
                        std::int8_t sizeX = *(std::int8_t *)(data + 16);
                        std::int8_t sizeY = *(std::int8_t *)(data + 20);

                        std::int16_t centeringZ = 0;
                        std::int16_t centeringX = 0;

                        modelBounds[0] = sizeX;
                        modelBounds[1] = sizeY;
                        modelBounds[2] = sizeZ;

                        if (centered) {
                            centeringZ = sizeZ / 2;
                            centeringX = sizeX / 2;
                        }

                        data += 24;

                        if (memcmp(data, "XYZI", 4) == 0) {
                            std::size_t voxelCount = *(std::uint32_t *)(data + 12);

                            data += 16;
                            result[i].voxels.resize(voxelCount);

                            for (std::size_t c = 0; c < voxelCount; c++) {
                                result[i].voxels[c].positionZ = *(std::int8_t *)(data + c * 4 + 0) - centeringZ + offset[2];
                                result[i].voxels[c].positionX = *(std::int8_t *)(data + c * 4 + 1) - centeringX + offset[0];
                                result[i].voxels[c].positionY = *(std::int8_t *)(data + c * 4 + 2) + offset[1];
                                result[i].voxels[c].colorIndex = *(std::uint8_t *)(data + c * 4 + 3) - 1;

                                // TODO: voxel mesh optimization
                                result[i].voxels[c].sizeX = 1;
                                result[i].voxels[c].sizeY = 1;
                                result[i].voxels[c].sizeZ = 1;
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
}

namespace voxel {
    StaticMeshImpl::StaticMeshImpl(const std::shared_ptr<foundation::RenderingStructuredData> &geometry) : _geometry(geometry) {
        
    }

    StaticMeshImpl::~StaticMeshImpl() {

    }

    const std::shared_ptr<foundation::RenderingStructuredData> &StaticMeshImpl::getGeometry() const {
        return _geometry;
    }
}

namespace voxel {
    DynamicMeshImpl::DynamicMeshImpl(const std::shared_ptr<Model> &model)
        : _model(model)
    {
        ::memset(_transform, 0, 16 * sizeof(float));
    }

    DynamicMeshImpl::~DynamicMeshImpl() {

    }

    void DynamicMeshImpl::setTransform(const float(&position)[3], float rotationXZ) {

    }

    void DynamicMeshImpl::playAnimation(const char *name, std::function<void(DynamicMesh &)> &&finished, bool cycled, bool resetAnimationTime) {
        auto index = _model->animations.find(name);

        if (index != _model->animations.end()) {
            _currentAnimation = &index->second;
            _currentFrame = _currentAnimation->firstFrame;
            _lastFrame = 0;
            _finished = std::move(finished);
            _cycled = cycled;

            if (resetAnimationTime) {
                _time = 0.0f;
            }
        }
        else {
            _currentAnimation = nullptr;
        }
    }

    void DynamicMeshImpl::update(float dtSec) {
        if (_currentAnimation) {
            std::uint32_t frame = std::uint32_t(_time * _currentAnimation->frameRate);

            if (frame != _lastFrame) {
                if (_currentFrame == _currentAnimation->lastFrame) {
                    _currentFrame = _currentAnimation->firstFrame;

                    if (_finished) {
                        _finished(*this);
                    };

                    _time -= float(_currentAnimation->lastFrame - _currentAnimation->firstFrame + 1) / _currentAnimation->frameRate;
                    frame = std::uint32_t(_time * _currentAnimation->frameRate);

                    if (_cycled == false) {
                        _currentAnimation = nullptr;
                    }
                }
                else {
                    _currentFrame++;
                }

                _lastFrame = frame;
            }

            _time += dtSec;
        }
        else {
            _currentFrame = 0;
        }
    }

    const float(&DynamicMeshImpl::getTransform() const)[16]{
        return _transform;
    }

    const std::shared_ptr<foundation::RenderingStructuredData> &DynamicMeshImpl::getGeometry() const {
        return _model->voxels;
    }

    const std::uint32_t DynamicMeshImpl::getFrameStartIndex() const {
        return _model->frames[_currentFrame].index;
    }

    const std::uint32_t DynamicMeshImpl::getFrameSize() const {
        return _model->frames[_currentFrame].size;
    }
}

namespace voxel {
    MeshFactoryImpl::MeshFactoryImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering)
        : _platform(platform)
        , _rendering(rendering)
    {

    }

    MeshFactoryImpl::~MeshFactoryImpl() {

    }

    bool MeshFactoryImpl::loadVoxels(const char *voxFullPath, int x, int y, int z, Rotation rotation, std::vector<Voxel> &out) {
        std::string modelPath = voxFullPath;

        int16_t modelOffset[3] = { 0, 0, 0 };
        int16_t modelBounds[3];

        std::vector<voxel::Chunk> frames = voxel::loadModel(_platform, modelPath.data(), modelOffset, false, modelBounds);

        if (frames.size()) {
            unsigned totalVoxelCount = 0;
            std::vector<Voxel> &voxels = frames[0].voxels;

            void (*rotate[4])(Voxel & voxel, int16_t(&modelBounds)[3]) = {
                [](Voxel &voxel, int16_t(&modelBounds)[3]) {
                },
                [](Voxel &voxel, int16_t(&modelBounds)[3]) {
                    int16_t x = modelBounds[2] - voxel.positionZ - 1;
                    int16_t z = voxel.positionX;

                    voxel.positionX = x;
                    voxel.positionZ = z;
                },
                [](Voxel &voxel, int16_t(&modelBounds)[3]) {
                    int16_t x = modelBounds[0] - voxel.positionX - 1;
                    int16_t z = modelBounds[2] - voxel.positionZ - 1;

                    voxel.positionX = x;
                    voxel.positionZ = z;
                },
                [](Voxel &voxel, int16_t(&modelBounds)[3]) {
                    int16_t x = voxel.positionZ;
                    int16_t z = modelBounds[0] - voxel.positionX - 1;

                    voxel.positionX = x;
                    voxel.positionZ = z;
                },
            };

            for (auto &item : voxels) {
                rotate[int(rotation)](item, modelBounds);
                item.positionX += x;
                item.positionY += y;
                item.positionZ += z;
            }

            if (frames.size() > 1) {
                _platform->logMsg("[MeshFactory::createStaticMesh] More than one frame loaded for '%s'. First one is used", modelPath.data());
            }

            out.insert(out.end(), voxels.begin(), voxels.end());
            return true;
        }
        else {
            _platform->logError("[MeshFactory::createStaticMesh] No frames loaded for '%s'", modelPath.data());
        }

        return false;
    }

    std::shared_ptr<StaticMesh> MeshFactoryImpl::createStaticMesh(const std::vector<Voxel> &voxels) {
        std::shared_ptr<foundation::RenderingStructuredData> geometry = _rendering->createData(&voxels[0], uint32_t(voxels.size()), sizeof(Voxel));
        std::shared_ptr<StaticMesh> result = std::make_shared<StaticMeshImpl>(geometry);
        return result;
    }

    std::shared_ptr<DynamicMesh> MeshFactoryImpl::createDynamicMesh(const char *resourcePath, const float(&position)[3], float rotationXZ) {
        std::shared_ptr<DynamicMesh> result;
        std::shared_ptr<Model> model;

        auto index = _models.find(resourcePath);
        if (index != _models.end() && (model = index->second.lock()) != nullptr) {
            result = std::make_shared<DynamicMeshImpl>(model);
        }
        else {
            std::string infoPath = std::string(resourcePath) + ".info";
            std::string modelPath = std::string(resourcePath) + ".vox";

            std::unique_ptr<uint8_t[]> infoData;
            std::size_t infoSize;
            
            model = std::make_shared<Model>();

            if (_platform->loadFile(infoPath.data(), infoData, infoSize)) {
                int16_t modelOffset[3] = {0, 0, 0};
                int16_t modelBounds[3];

                std::istringstream stream(std::string(reinterpret_cast<const char *>(infoData.get()), infoSize));
                std::string keyword;

                while (stream >> keyword) {
                    if (keyword == "animation") {
                        std::string animationName;
                        std::uint32_t firstFrame, lastFrame;
                        float frameRate;

                        if (stream >> gears::expect<'='> >> gears::quoted(animationName) >> firstFrame >> lastFrame >> frameRate) {
                            model->animations.emplace(std::move(animationName), Model::Animation{ firstFrame, lastFrame, frameRate });
                        }
                        else {
                            _platform->logError("[MeshFactory::createDynamicMesh] Invalid animation arguments in '%s'", infoPath.data());
                            break;
                        }
                    }
                    else if (keyword == "offset") {
                        if (bool(stream >> gears::expect<'='> >> modelOffset[0] >> modelOffset[1] >> modelOffset[2]) == false) {
                            _platform->logError("[MeshFactory::createDynamicMesh] Invalid offset arguments in '%s'", infoPath.data());
                            break;
                        }
                    }
                    else {
                        _platform->logError("[MeshFactory::createDynamicMesh] Unreconized keyword '%s' in '%s'", keyword.data(), infoPath.data());
                        break;
                    }
                }

                std::vector<voxel::Chunk> frames = voxel::loadModel(_platform, modelPath.data(), modelOffset, true, modelBounds);

                if (frames.size()) {
                    unsigned totalVoxelCount = 0;
                    std::vector<Voxel> voxels;

                    for (auto &item : frames) {
                        model->frames.emplace_back(Model::Frame{ totalVoxelCount, unsigned(item.voxels.size()) });
                        totalVoxelCount += unsigned(item.voxels.size());
                        voxels.insert(voxels.end(), item.voxels.begin(), item.voxels.end());
                    }

                    model->voxels = _rendering->createData(&voxels[0], totalVoxelCount, sizeof(Voxel));
                    result = std::make_shared<DynamicMeshImpl>(model);
                }
                else {
                    _platform->logError("[MeshFactory::createDynamicMesh] No frames loaded for '%s'", modelPath.data());
                }
            }
        }

        result->setTransform(position, rotationXZ);
        return result;
    }
}

namespace voxel {
    std::shared_ptr<MeshFactory> MeshFactory::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering) {
        return std::make_shared<MeshFactoryImpl>(platform, rendering);
    }
}