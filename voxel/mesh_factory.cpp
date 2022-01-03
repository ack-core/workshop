
#include "mesh_factory.h"

#include "thirdparty/upng/upng.h"

#include "foundation/gears/math.h"
#include "foundation/gears/parsing.h"

#include <unordered_map>

namespace {
    const char *g_staticMeshShaderSrc = R"(
        fixed {
            axis[3] : float4 = 
                [0.0, 0.0, 1.0, 0.0]
                [1.0, 0.0, 0.0, 0.0]
                [0.0, 1.0, 0.0, 0.0]
            cube[12] : float4 = 
                [-0.5, 0.5, 0.5, 1.0]
                [-0.5, -0.5, 0.5, 1.0]
                [0.5, 0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0]
                [0.5, 0.5, 0.5, 1.0]
                [0.5, -0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, 0.5, 1.0]
                [-0.5, 0.5, -0.5, 1.0]
                [-0.5, 0.5, 0.5, 1.0]
        }
        inout {
            texcoord : float2
            normal : float3
        }
        vssrc {
            float4 center = float4(instance_position.xyz, 1.0);
            float3 camSign = _sign(_cameraPosition.xyz - instance_position.xyz);
            float4 cube_position = float4(camSign, 0.0) * cube[vertex_ID] + center; //
            output_position = _transform(cube_position, _viewProjMatrix);
            output_texcoord = float2(instance_scale_color.w / 255.0, 0);
            output_normal = camSign * axis[vertex_ID >> 2];
        }
        fssrc {
            float light = 0.7 + 0.3 * _dot(input_normal, _norm(float3(0.1, 2.0, 0.3)));
            output_color = float4(_tex2d(0, input_texcoord).xyz * light * light, 1.0);
        }
    )";

    const char *g_dynamicMeshShaderSrc = R"(
        fixed {
            axis[3] : float4 = 
                [0.0, 0.0, 1.0, 0.0]
                [1.0, 0.0, 0.0, 0.0]
                [0.0, 1.0, 0.0, 0.0]
            cube[12] : float4 = 
                [-0.5, 0.5, 0.5, 1.0]
                [-0.5, -0.5, 0.5, 1.0]
                [0.5, 0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0]
                [0.5, 0.5, 0.5, 1.0]
                [0.5, -0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, 0.5, 1.0]
                [-0.5, 0.5, -0.5, 1.0]
                [-0.5, 0.5, 0.5, 1.0]
        }
        const {
            modelTransform : matrix4
        }
        inout {
            texcoord : float2
            normal : float3
        }
        vssrc {
            float4 center = float4(instance_position.xyz, 1.0);
            float3 camSign = _sign(_transform(modelTransform, float4(_cameraPosition.xyz - _transform(center, modelTransform).xyz, 0)).xyz);
            float4 cube_position = float4(camSign, 0.0) * cube[vertex_ID] + center; //
            output_position = _transform(_transform(cube_position, modelTransform), _viewProjMatrix);
            output_texcoord = float2(instance_scale_color.w / 255.0, 0);
            output_normal = camSign * axis[vertex_ID >> 2];
        }
        fssrc {
            float light = 0.7 + 0.3 * _dot(input_normal, _norm(float3(0.1, 2.0, 0.3)));
            output_color = float4(_tex2d(0, input_texcoord).xyz * light * light, 1.0);
        }
    )";
}

namespace voxel {
    struct Chunk {
        std::vector<Voxel> voxels;
        int16_t modelBounds[3];
    };

    std::vector<Chunk> loadModel(const std::shared_ptr<foundation::PlatformInterface> &platform, const char *fullPath, const int16_t(&offset)[3], bool centered) {
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

                        std::int16_t centeringZ = 0;
                        std::int16_t centeringX = 0;

                        result[i].modelBounds[0] = sizeX;
                        result[i].modelBounds[1] = sizeY;
                        result[i].modelBounds[2] = sizeZ;

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
                                result[i].voxels[c].positionZ = std::int16_t(*(std::uint8_t *)(data + c * 4 + 0)) - centeringZ + offset[2];
                                result[i].voxels[c].positionX = std::int16_t(*(std::uint8_t *)(data + c * 4 + 1)) - centeringX + offset[0];
                                result[i].voxels[c].positionY = std::int16_t(*(std::uint8_t *)(data + c * 4 + 2)) + offset[1];
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
    struct Model {
        struct Frame {
            std::uint32_t index;
            std::uint32_t size;
        };
        struct Animation {
            std::uint32_t firstFrame;
            std::uint32_t lastFrame;
            float frameRate;
        };

        std::shared_ptr<foundation::RenderingStructuredData> voxels;
        std::unordered_map<std::string, Animation> animations;
        std::vector<Frame> frames;
    };

    class MeshFactoryImpl : public std::enable_shared_from_this<MeshFactoryImpl>, public MeshFactory {
    public:
        MeshFactoryImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering, const char *palettePath);
        ~MeshFactoryImpl() override;

        bool appendVoxels(const char *voxFullPath, int x, int y, int z, Rotation rotation, std::vector<Voxel> &target) override;
        std::shared_ptr<StaticMesh> createStaticMesh(std::vector<Voxel> &voxels) override;
        std::shared_ptr<DynamicMesh> createDynamicMesh(const char *resourcePath, const float(&position)[3], float rotationXZ) override;
        void drawDynamicMesh(const std::shared_ptr<Model> &model, const float(&transform)[16]);

    private:
        std::shared_ptr<foundation::PlatformInterface> _platform;
        std::shared_ptr<foundation::RenderingInterface> _rendering;

        std::shared_ptr<foundation::RenderingShader> _staticModelShader;
        std::shared_ptr<foundation::RenderingShader> _dynamicModelShader;
        std::shared_ptr<foundation::RenderingTexture2D> _palette;

        std::unordered_map<std::string, std::weak_ptr<Model>> _models;
        std::unordered_map<std::string, voxel::Chunk> _cache;
    };

    class StaticMeshImpl : public StaticMesh {
    public:
        StaticMeshImpl(const std::shared_ptr<foundation::RenderingStructuredData> &geometry) : _geometry(geometry) {

        }

        ~StaticMeshImpl() override {

        }

        void updateAndDraw(float dtSec) override {

        }

    private:
        std::shared_ptr<foundation::RenderingInterface> _rendering;
        std::shared_ptr<foundation::RenderingStructuredData> _geometry;
    };

    class DynamicMeshImpl : public DynamicMesh {
    public:
        DynamicMeshImpl(const std::shared_ptr<MeshFactoryImpl> &factory, const std::shared_ptr<Model> &model) : _factory(factory), _model(model) {
            *reinterpret_cast<math::transform3f *>(_transform) = math::transform3f::identity();
        }

        ~DynamicMeshImpl() override {

        }

        void updateAndDraw(float dtSec) override {
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

            _factory->drawDynamicMesh(_model, _transform);

            //_factory->getRenderingInterface().applyShader(g_dynamicModelShaderPtr, _transform);
            //_factory->getRenderingInterface().applyTextures({ g_palette.get() });
            //_factory->getRenderingInterface().drawGeometry(nullptr, _model->voxels, 0, 12, 0, _model->voxels->getCount(), foundation::RenderingTopology::TRIANGLESTRIP);
        }

        void setTransform(const float(&position)[3], float rotationXZ) override {
            *reinterpret_cast<math::transform3f *>(_transform) = math::transform3f({ 0, 1, 0 }, rotationXZ).translated(*reinterpret_cast<const math::vector3f *>(position));
        }

        void playAnimation(const char *name, std::function<void(DynamicMesh &)> &&finished, bool cycled, bool resetAnimationTime) override {
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

    private:
        std::shared_ptr<MeshFactoryImpl> _factory;
        std::shared_ptr<Model> _model;

        Model::Animation *_currentAnimation = nullptr;
        std::function<void(DynamicMesh &)> _finished;

        float _time = 0.0f;
        float _transform[16];

        bool _cycled = false;

        std::uint32_t _lastFrame = 0;
        std::uint32_t _currentFrame = 0;
    };

    MeshFactoryImpl::MeshFactoryImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering, const char *palettePath)
        : _platform(platform)
        , _rendering(rendering)
    {
        _staticModelShader = rendering->createShader("static_voxel_mesh", g_staticMeshShaderSrc,
            // vertex
            {
                {"ID", foundation::RenderingShaderInputFormat::VERTEX_ID}
            },
            // instance
            {
                {"position", foundation::RenderingShaderInputFormat::SHORT4},
                {"scale_color", foundation::RenderingShaderInputFormat::BYTE4}
            }
        );

        _dynamicModelShader = rendering->createShader("dynamic_voxel_mesh", g_dynamicMeshShaderSrc,
            // vertex
            {
                {"ID", foundation::RenderingShaderInputFormat::VERTEX_ID}
            },
            // instance
            {
                {"position", foundation::RenderingShaderInputFormat::SHORT4},
                {"scale_color", foundation::RenderingShaderInputFormat::BYTE4}
            }
        );

        std::unique_ptr<std::uint8_t[]> paletteData;
        std::size_t paletteSize;

        if (_platform->loadFile(palettePath, paletteData, paletteSize)) {
            upng_t *upng = upng_new_from_bytes(paletteData.get(), (unsigned long)(paletteSize));

            if (upng != nullptr) {
                if (*reinterpret_cast<const unsigned *>(paletteData.get()) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK) {
                    if (upng_get_format(upng) == UPNG_RGBA8 && upng_get_width(upng) == 256 && upng_get_height(upng) == 1) {
                        _palette = _rendering->createTexture(foundation::RenderingTextureFormat::RGBA8UN, 256, 1, { upng_get_buffer(upng) });
                    }
                    else {
                        _platform->logError("[MeshFactoryImpl::MeshFactoryImpl] '%s' is not 256x1 RGBA png file", palettePath);
                    }
                }
                else {
                    _platform->logError("[MeshFactoryImpl::MeshFactoryImpl] '%s' is not a valid png file", palettePath);
                }

                upng_free(upng);
            }
        }
        else {
            _platform->logError("[MeshFactoryImpl::MeshFactoryImpl] '%s' not found", palettePath);
        }
    }

    MeshFactoryImpl::~MeshFactoryImpl() {

    }

    bool MeshFactoryImpl::appendVoxels(const char *voxFullPath, int x, int y, int z, Rotation rotation, std::vector<Voxel> &target) {
        return false;
    }

    std::shared_ptr<StaticMesh> MeshFactoryImpl::createStaticMesh(std::vector<Voxel> &voxels) {
        return {};
    }

    std::shared_ptr<DynamicMesh> MeshFactoryImpl::createDynamicMesh(const char *resourcePath, const float(&position)[3], float rotationXZ) {
        std::shared_ptr<DynamicMesh> result;
        std::shared_ptr<Model> model;

        auto index = _models.find(resourcePath);
        if (index != _models.end() && (model = index->second.lock()) != nullptr) {
            result = std::make_shared<DynamicMeshImpl>(shared_from_this(), model);
        }
        else {
            std::string infoPath = std::string(resourcePath) + ".info";
            std::string modelPath = std::string(resourcePath) + ".vox";

            std::unique_ptr<uint8_t[]> infoData;
            std::size_t infoSize;

            model = std::make_shared<Model>();
            int16_t modelOffset[3] = { 0, 0, 0 };

            if (_platform->loadFile(infoPath.data(), infoData, infoSize)) {
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
            }

            std::vector<voxel::Chunk> frames = loadModel(_platform, modelPath.data(), modelOffset, true);

            if (frames.size()) {
                unsigned totalVoxelCount = 0;
                std::vector<Voxel> voxels;

                for (auto &item : frames) {
                    model->frames.emplace_back(Model::Frame{ totalVoxelCount, unsigned(item.voxels.size()) });
                    totalVoxelCount += unsigned(item.voxels.size());
                    voxels.insert(voxels.end(), item.voxels.begin(), item.voxels.end());
                }

                model->voxels = _rendering->createData(&voxels[0], totalVoxelCount, sizeof(Voxel));
                result = std::make_shared<DynamicMeshImpl>(shared_from_this(), model);
            }
            else {
                _platform->logError("[MeshFactory::createDynamicMesh] No frames loaded for '%s'", modelPath.data());
            }
        }

        if (result) {
            result->setTransform(position, rotationXZ);
        }

        return result;
    }

    void MeshFactoryImpl::drawDynamicMesh(const std::shared_ptr<Model> &model, const float (&transform)[16]) {
        _rendering->applyShader(_dynamicModelShader, transform);
        _rendering->applyTextures({ _palette.get() });
        _rendering->drawGeometry(nullptr, model->voxels, 0, 12, 0, model->voxels->getCount(), foundation::RenderingTopology::TRIANGLESTRIP);
    }
}



//namespace voxel {
//    StaticMeshImpl::StaticMeshImpl(const std::shared_ptr<MeshFactoryImpl> &factory, const std::shared_ptr<foundation::RenderingStructuredData> &geometry) : _factory(factory), _geometry(geometry) {
//        
//    }
//
//    StaticMeshImpl::~StaticMeshImpl() {
//
//    }
//
//    void StaticMeshImpl::updateAndDraw(float dtSec) {
//        _factory->getRenderingInterface().applyShader(g_staticModelShaderPtr);
//        _factory->getRenderingInterface().applyTextures({ g_palette.get() });
//        _factory->getRenderingInterface().drawGeometry(nullptr, _geometry, 0, 12, 0, _geometry->getCount(), foundation::RenderingTopology::TRIANGLESTRIP);
//    }
//}

//namespace voxel {
//    DynamicMeshImpl::DynamicMeshImpl(const std::shared_ptr<MeshFactoryImpl> &factory, const std::shared_ptr<Model> &model)
//        : _factory(factory)
//        , _model(model)
//    {
//        *reinterpret_cast<math::transform3f *>(_transform) = math::transform3f::identity();
//    }
//
//    DynamicMeshImpl::~DynamicMeshImpl() {
//
//    }
//
//    void DynamicMeshImpl::setTransform(const float(&position)[3], float rotationXZ) {
//        *reinterpret_cast<math::transform3f *>(_transform) = math::transform3f::transform3f({0, 1, 0}, rotationXZ).translated(*reinterpret_cast<const math::vector3f *>(position));
//    }
//
//    void DynamicMeshImpl::playAnimation(const char *name, std::function<void(DynamicMesh &)> &&finished, bool cycled, bool resetAnimationTime) {
//        auto index = _model->animations.find(name);
//
//        if (index != _model->animations.end()) {
//            _currentAnimation = &index->second;
//            _currentFrame = _currentAnimation->firstFrame;
//            _lastFrame = 0;
//            _finished = std::move(finished);
//            _cycled = cycled;
//
//            if (resetAnimationTime) {
//                _time = 0.0f;
//            }
//        }
//        else {
//            _currentAnimation = nullptr;
//        }
//    }
//
//    void DynamicMeshImpl::updateAndDraw(float dtSec) {
//        if (_currentAnimation) {
//            std::uint32_t frame = std::uint32_t(_time * _currentAnimation->frameRate);
//
//            if (frame != _lastFrame) {
//                if (_currentFrame == _currentAnimation->lastFrame) {
//                    _currentFrame = _currentAnimation->firstFrame;
//
//                    if (_finished) {
//                        _finished(*this);
//                    };
//
//                    _time -= float(_currentAnimation->lastFrame - _currentAnimation->firstFrame + 1) / _currentAnimation->frameRate;
//                    frame = std::uint32_t(_time * _currentAnimation->frameRate);
//
//                    if (_cycled == false) {
//                        _currentAnimation = nullptr;
//                    }
//                }
//                else {
//                    _currentFrame++;
//                }
//
//                _lastFrame = frame;
//            }
//
//            _time += dtSec;
//        }
//        else {
//            _currentFrame = 0;
//        }
//
//        _factory->getRenderingInterface().applyShader(g_dynamicModelShaderPtr, _transform);
//        _factory->getRenderingInterface().applyTextures({ g_palette.get() });
//        _factory->getRenderingInterface().drawGeometry(nullptr, _model->voxels, 0, 12, 0, _model->voxels->getCount(), foundation::RenderingTopology::TRIANGLESTRIP);
//    }
//}

//namespace voxel {
//    MeshFactoryImpl::MeshFactoryImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering, const char *palettePath)
//        : _platform(platform)
//        , _rendering(rendering)
//    {
//        g_staticModelShaderPtr = rendering->createShader("static_voxel_mesh", g_staticMeshShaderSrc,
//            // vertex
//            {
//                {"ID", foundation::RenderingShaderInputFormat::VERTEX_ID}
//            },
//            // instance
//            {
//                {"position", foundation::RenderingShaderInputFormat::SHORT4},
//                {"scale_color", foundation::RenderingShaderInputFormat::BYTE4}
//            }
//        );
//
//        g_dynamicModelShaderPtr = rendering->createShader("dynamic_voxel_mesh", g_dynamicMeshShaderSrc,
//            // vertex
//            {
//                {"ID", foundation::RenderingShaderInputFormat::VERTEX_ID}
//            },
//            // instance
//            {
//                {"position", foundation::RenderingShaderInputFormat::SHORT4},
//                {"scale_color", foundation::RenderingShaderInputFormat::BYTE4}
//            }
//        );
//
//        std::unique_ptr<std::uint8_t[]> paletteData;
//        std::size_t paletteSize;
//
//        if (_platform->loadFile(palettePath, paletteData, paletteSize)) {
//            upng_t *upng = upng_new_from_bytes(paletteData.get(), unsigned long(paletteSize));
//
//            if (upng != nullptr) {
//                if (*reinterpret_cast<const unsigned *>(paletteData.get()) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK) {
//                    if (upng_get_format(upng) == UPNG_RGBA8 && upng_get_width(upng) == 256 && upng_get_height(upng) == 1) {
//                        g_palette = _rendering->createTexture(foundation::RenderingTextureFormat::RGBA8UN, 256, 1, { upng_get_buffer(upng) });
//                    }
//                    else {
//                        _platform->logError("[MeshFactoryImpl::MeshFactoryImpl] '%s' is not 256x1 RGBA png file", palettePath);
//                    }
//                }
//                else {
//                    _platform->logError("[MeshFactoryImpl::MeshFactoryImpl] '%s' is not a valid png file", palettePath);
//                }
//
//                upng_free(upng);
//            }
//        }
//        else {
//            _platform->logError("[MeshFactoryImpl::MeshFactoryImpl] '%s' not found", palettePath);
//        }
//    }
//
//    MeshFactoryImpl::~MeshFactoryImpl() {
//        g_staticModelShaderPtr = nullptr;
//        g_dynamicModelShaderPtr = nullptr;
//    }
//
//    bool MeshFactoryImpl::loadVoxels(const char *voxFullPath, int x, int y, int z, Rotation rotation, std::vector<Voxel> &out) {
//        std::string modelPath = voxFullPath;
//        voxel::Chunk chunk;
//
//        auto index = _cache.find(modelPath);
//        if (index != _cache.end()) {
//            chunk = index->second;
//        }
//        else {
//            int16_t modelOffset[3] = { 0, 0, 0 };
//            std::vector<voxel::Chunk> frames = voxel::loadModel(_platform, modelPath.data(), modelOffset, false);
//
//            if (frames.size()) {
//                chunk = _cache.emplace(modelPath, std::move(frames[0])).first->second;
//
//                if (frames.size() > 1) {
//                    _platform->logMsg("[MeshFactory::createStaticMesh] More than one frame loaded for '%s'. First one is used", modelPath.data());
//                }
//            }
//            else {
//                _platform->logError("[MeshFactory::createStaticMesh] No frames loaded for '%s'", modelPath.data());
//                return false;
//            }
//        }
//
//        void (*rotate[4])(Voxel & voxel, int16_t(&modelBounds)[3]) = {
//            [](Voxel &voxel, int16_t(&modelBounds)[3]) {
//            },
//            [](Voxel &voxel, int16_t(&modelBounds)[3]) {
//                int16_t x = modelBounds[2] - voxel.positionZ - 1;
//                int16_t z = voxel.positionX;
//
//                voxel.positionX = x;
//                voxel.positionZ = z;
//            },
//            [](Voxel &voxel, int16_t(&modelBounds)[3]) {
//                int16_t x = modelBounds[0] - voxel.positionX - 1;
//                int16_t z = modelBounds[2] - voxel.positionZ - 1;
//
//                voxel.positionX = x;
//                voxel.positionZ = z;
//            },
//            [](Voxel &voxel, int16_t(&modelBounds)[3]) {
//                int16_t x = voxel.positionZ;
//                int16_t z = modelBounds[0] - voxel.positionX - 1;
//
//                voxel.positionX = x;
//                voxel.positionZ = z;
//            },
//        };
//
//        for (auto &item : chunk.voxels) {
//            rotate[int(rotation)](item, chunk.modelBounds);
//            item.positionX += x;
//            item.positionY += y;
//            item.positionZ += z;
//        }
//
//        out.insert(out.end(), chunk.voxels.begin(), chunk.voxels.end());
//        return true;
//    }
//
//    std::shared_ptr<StaticMesh> MeshFactoryImpl::createStaticMesh(const Voxel *voxels, std::size_t count) {
//        std::shared_ptr<foundation::RenderingStructuredData> geometry = _rendering->createData(voxels, uint32_t(count), sizeof(Voxel));
//        std::shared_ptr<StaticMesh> result = std::make_shared<StaticMeshImpl>(shared_from_this(), geometry);
//        return result;
//    }
//
//    std::shared_ptr<DynamicMesh> MeshFactoryImpl::createDynamicMesh(const char *resourcePath, const float(&position)[3], float rotationXZ) {
//        std::shared_ptr<DynamicMesh> result;
//        std::shared_ptr<Model> model;
//
//        auto index = _models.find(resourcePath);
//        if (index != _models.end() && (model = index->second.lock()) != nullptr) {
//            result = std::make_shared<DynamicMeshImpl>(shared_from_this(), model);
//        }
//        else {
//            std::string infoPath = std::string(resourcePath) + ".info";
//            std::string modelPath = std::string(resourcePath) + ".vox";
//
//            std::unique_ptr<uint8_t[]> infoData;
//            std::size_t infoSize;
//            
//            model = std::make_shared<Model>();
//
//            if (_platform->loadFile(infoPath.data(), infoData, infoSize)) {
//                int16_t modelOffset[3] = {0, 0, 0};
//
//                std::istringstream stream(std::string(reinterpret_cast<const char *>(infoData.get()), infoSize));
//                std::string keyword;
//
//                while (stream >> keyword) {
//                    if (keyword == "animation") {
//                        std::string animationName;
//                        std::uint32_t firstFrame, lastFrame;
//                        float frameRate;
//
//                        if (stream >> gears::expect<'='> >> gears::quoted(animationName) >> firstFrame >> lastFrame >> frameRate) {
//                            model->animations.emplace(std::move(animationName), Model::Animation{ firstFrame, lastFrame, frameRate });
//                        }
//                        else {
//                            _platform->logError("[MeshFactory::createDynamicMesh] Invalid animation arguments in '%s'", infoPath.data());
//                            break;
//                        }
//                    }
//                    else if (keyword == "offset") {
//                        if (bool(stream >> gears::expect<'='> >> modelOffset[0] >> modelOffset[1] >> modelOffset[2]) == false) {
//                            _platform->logError("[MeshFactory::createDynamicMesh] Invalid offset arguments in '%s'", infoPath.data());
//                            break;
//                        }
//                    }
//                    else {
//                        _platform->logError("[MeshFactory::createDynamicMesh] Unreconized keyword '%s' in '%s'", keyword.data(), infoPath.data());
//                        break;
//                    }
//                }
//
//                std::vector<voxel::Chunk> frames = voxel::loadModel(_platform, modelPath.data(), modelOffset, true);
//
//                if (frames.size()) {
//                    unsigned totalVoxelCount = 0;
//                    std::vector<Voxel> voxels;
//
//                    for (auto &item : frames) {
//                        model->frames.emplace_back(Model::Frame{ totalVoxelCount, unsigned(item.voxels.size()) });
//                        totalVoxelCount += unsigned(item.voxels.size());
//                        voxels.insert(voxels.end(), item.voxels.begin(), item.voxels.end());
//                    }
//
//                    model->voxels = _rendering->createData(&voxels[0], totalVoxelCount, sizeof(Voxel));
//                    result = std::make_shared<DynamicMeshImpl>(shared_from_this(), model);
//                }
//                else {
//                    _platform->logError("[MeshFactory::createDynamicMesh] No frames loaded for '%s'", modelPath.data());
//                }
//            }
//            else {
//                _platform->logError("[MeshFactory::createDynamicMesh] '%s' not found", infoPath.data());
//            }
//        }
//
//        if (result) {
//            result->setTransform(position, rotationXZ);
//        }
//
//        return result;
//    }
//
//    foundation::RenderingInterface &MeshFactoryImpl::getRenderingInterface() {
//        return *_rendering;
//    }
//}
//
namespace voxel {
    std::shared_ptr<MeshFactory> MeshFactory::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering, const char *palettePath) {
        return std::make_shared<MeshFactoryImpl>(platform, rendering, palettePath);
    }
}
