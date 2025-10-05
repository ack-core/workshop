
#include "resource_provider.h"
#include "textures_list.h"
#include "meshes_list.h"
#include "grounds_list.h"
#include "foundation/layouts.h"

#include "thirdparty/upng/upng.h"

#include <list>
#include <memory>

namespace {
    struct TextureAsyncContext {
        std::unique_ptr<std::uint8_t[]> data;
        std::uint32_t w, h;
        foundation::RenderTextureFormat format;
    };
    struct MeshAsyncContext {
        struct Voxel {
            std::int16_t positionX, positionY, positionZ;
            std::uint8_t colorIndex, mask, scaleX, scaleY, scaleZ, reserved;
        };
        struct VTXMVOX {
            std::int16_t positionX, positionY, positionZ;
            std::uint8_t colorIndex, mask;
        };
        util::IntegerOffset3D originOffset = {0, 0, 0};
        std::vector<std::vector<VTXMVOX>> voxels;
    };
    
    bool readEmitter(resource::EmitterDescription &desc, const std::uint8_t *data) {
        if (memcmp(data, "EMTR", 4) == 0) {
            std::uint64_t flags = *(std::int64_t *)(data + 4);
            desc.looped = (flags & 0x1) != 0;
            desc.additiveBlend = (flags & 0x2) != 0;
            desc.startShapeFill = (flags & 0x4) != 0;
            desc.endShapeFill = (flags & 0x8) != 0;
            data += 12;
            desc.randomSeed = *(std::int32_t *)(data + 0);
            desc.particlesToEmit = *(std::int16_t *)(data + 4);
            desc.emissionTimeMs = *(std::int16_t *)(data + 6);
            desc.particleLifeTimeMs = *(std::int16_t *)(data + 8);
            desc.bakingFrameTimeMs = *(std::int16_t *)(data + 10);
            data += 12;
            desc.particleStartSpeed = *(float *)(data + 0);
            data += 4;
            desc.particleOrientation = *(std::uint8_t *)(data + 0);
            desc.startShapeType = *(std::uint8_t *)(data + 1);
            desc.endShapeType = *(std::uint8_t *)(data + 2);
            desc.shapeDistributionType = *(std::uint8_t *)(data + 3);
            data += 4;
            desc.startShapeArgs = math::vector3f(*(float *)(data + 0), *(float *)(data + 4), *(float *)(data + 8));
            data += 12;
            desc.endShapeArgs = math::vector3f(*(float *)(data + 0), *(float *)(data + 4), *(float *)(data + 8));
            data += 12;
            desc.endShapeOffset = math::vector3f(*(float *)(data + 0), *(float *)(data + 4), *(float *)(data + 8));
            data += 12;
            desc.minXYZ = math::vector3f(*(float *)(data + 0), *(float *)(data + 4), *(float *)(data + 8));
            data += 12;
            desc.maxXYZ = math::vector3f(*(float *)(data + 0), *(float *)(data + 4), *(float *)(data + 8));
            data += 12;
            desc.maxSize = math::vector2f(*(float *)(data + 0), *(float *)(data + 4));
            data += 8;
            desc.texturePath = (const char *)(data + 1);
            data += *(std::uint8_t *)(data + 0);
            return true;
        }
        return false;
    }

    void readMesh(MeshAsyncContext &ctx, const std::uint8_t *data) {
        ctx.voxels.clear();
        
        if (memcmp(data, "VOX ", 4) == 0) {
            if (*(std::int32_t *)(data + 4) == 0x7f) { // vox made by gen_meshes.py
                data += 32;
                ctx.originOffset.x = *(std::int8_t *)(data + 0);
                ctx.originOffset.y = *(std::int8_t *)(data + 1);
                ctx.originOffset.z = *(std::int8_t *)(data + 2);
                data += 4;
                std::uint32_t frameCount = *(std::uint32_t *)data;
                ctx.voxels.resize(frameCount);
                data += sizeof(std::uint32_t);
                
                for (std::uint32_t f = 0; f < frameCount; f++) {
                    std::uint32_t voxelCount = *(std::uint32_t *)data;
                    data += sizeof(std::uint32_t);
                    ctx.voxels[f].resize(voxelCount);
                    
                    for (std::uint32_t i = 0; i < voxelCount; i++) {
                        const MeshAsyncContext::Voxel &src = *(MeshAsyncContext::Voxel *)data;
                        MeshAsyncContext::VTXMVOX &voxel = ctx.voxels[f][i];
                        voxel.positionX = src.positionX - ctx.originOffset.x;
                        voxel.positionY = src.positionY - ctx.originOffset.y;
                        voxel.positionZ = src.positionZ - ctx.originOffset.z;
                        voxel.colorIndex = src.colorIndex;
                        voxel.mask = src.mask;

                        data += sizeof(MeshAsyncContext::Voxel);
                    }
                }
            }
        }
    }
}

namespace resource {
    class ResourceProviderImpl : public std::enable_shared_from_this<ResourceProviderImpl>, public ResourceProvider {
    public:
        ResourceProviderImpl(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering);
        ~ResourceProviderImpl() override;
        
        auto getTextureInfo(const char *texPath) -> const TextureInfo * override;
        auto getMeshInfo(const char *voxPath) -> const MeshInfo * override;
        auto getGroundInfo(const char *groundPath) -> const GroundInfo * override;
        
        void getOrLoadTexture(const char *texPath, util::callback<void(const foundation::RenderTexturePtr &)> &&completion) override;
        void getOrLoadVoxelMesh(const char *meshPath, util::callback<void(const std::vector<foundation::RenderDataPtr> &, const util::IntegerOffset3D &)> &&completion) override;
        void getOrLoadGround(const char *groundPath, util::callback<void(const foundation::RenderDataPtr &, const foundation::RenderTexturePtr &)> &&completion) override;
        void getOrLoadEmitter(const char *cfgPath, util::callback<void(const resource::EmitterDescriptionPtr &, const foundation::RenderTexturePtr &, const foundation::RenderTexturePtr &)> &&completion) override;

        void removeTexture(const char *texturePath) override;
        void removeMesh(const char *meshPath) override;
        void removeGround(const char *groundPath) override;
        void removeEmitter(const char *configPath) override;

        void update(float dtSec) override;
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        const std::shared_ptr<foundation::RenderingInterface> _rendering;
        
        struct TextureData : public TextureInfo { // TODO: remove parent
            foundation::RenderTexturePtr ptr;
            bool outdated = false;
        };
        struct VoxelMesh {
            std::vector<foundation::RenderDataPtr> frames;
            util::IntegerOffset3D originOffset;
            bool outdated = false;
        };
        struct GroundMesh {
            foundation::RenderDataPtr data;
            foundation::RenderTexturePtr texture;
            bool outdated = false;
        };
        struct Emitter {
            EmitterDescriptionPtr params;
            foundation::RenderTexturePtr map;
            foundation::RenderTexturePtr texture;
            bool outdated = false;
        };
        
        std::unordered_map<std::string, TextureData> _textures;
        std::unordered_map<std::string, VoxelMesh> _meshes;
        std::unordered_map<std::string, GroundMesh> _grounds;
        std::unordered_map<std::string, Emitter> _emitters;
        
        struct QueueEntryTexture {
            std::string texPath;
            util::callback<void(const foundation::RenderTexturePtr &)> callbackPtr;
        };
        struct QueueEntryMesh {
            std::string meshPath;
            util::callback<void(const std::vector<foundation::RenderDataPtr> &, const util::IntegerOffset3D &)> callback;
        };
        struct QueueEntryGround {
            std::string groundPath;
            util::callback<void(const foundation::RenderDataPtr &, const foundation::RenderTexturePtr &)> callback;
        };
        struct QueueEntryEmitter {
            std::string configPath;
            util::callback<void(const resource::EmitterDescriptionPtr &, const foundation::RenderTexturePtr &, const foundation::RenderTexturePtr &)> callback;
        };
        
        std::list<QueueEntryTexture> _callsQueueTexture;
        std::list<QueueEntryMesh> _callsQueueMesh;
        std::list<QueueEntryGround> _callsQueueGround;
        std::list<QueueEntryEmitter> _callsQueueEmitter;

        bool _asyncInProgress;
    };
    
    ResourceProviderImpl::ResourceProviderImpl(
        const std::shared_ptr<foundation::PlatformInterface> &platform,
        const foundation::RenderingInterfacePtr &rendering
    )
    : _platform(platform)
    , _rendering(rendering)
    , _asyncInProgress(false)
    {
    }
    
    ResourceProviderImpl::~ResourceProviderImpl() {
    
    }
    
    const TextureInfo *ResourceProviderImpl::getTextureInfo(const char *texPath) {
        auto index = TEXTURES_LIST.find(texPath);
        return index != TEXTURES_LIST.end() ? &index->second : nullptr;
    }
    
    const MeshInfo *ResourceProviderImpl::getMeshInfo(const char *voxPath) {
        auto index = MESHES_LIST.find(voxPath);
        return index != MESHES_LIST.end() ? &index->second : nullptr;
    }
    
    const GroundInfo *ResourceProviderImpl::getGroundInfo(const char *groundPath) {
        auto index = GROUNDS_LIST.find(groundPath);
        return index != GROUNDS_LIST.end() ? &index->second : nullptr;
    }
    
    void ResourceProviderImpl::getOrLoadTexture(const char *texPath, util::callback<void(const foundation::RenderTexturePtr &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueueTexture.emplace_back(QueueEntryTexture {
                .texPath = texPath,
                .callbackPtr = std::move(completion)
            });
            
            return;
        }
        
        std::string path = std::string(texPath);
        
        auto index = _textures.find(path);
        if (index != _textures.end() && index->second.outdated == false) {
            completion(index->second.ptr);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".png").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<std::uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                    if (len) {
                        self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<TextureAsyncContext>>([weak, path, mem = std::move(mem), len](TextureAsyncContext &ctx) {
                            //--- worker thread ---
                            if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                                upng_t *upng = upng_new_from_bytes(mem.get(), (unsigned long)(len));
                                if (upng != nullptr && *reinterpret_cast<const unsigned *>(mem.get()) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK) {
                                    foundation::RenderTextureFormat format = foundation::RenderTextureFormat::UNKNOWN;
                                    std::uint32_t bytesPerPixel = 0;
                                    
                                    if (upng_get_format(upng) == UPNG_RGBA8) {
                                        format = foundation::RenderTextureFormat::RGBA8UN;
                                        bytesPerPixel = 4;
                                    }
                                    else if (upng_get_format(upng) == UPNG_LUMINANCE8) {
                                        format = foundation::RenderTextureFormat::R8UN;
                                        bytesPerPixel = 1;
                                    }
                                    
                                    if (format != foundation::RenderTextureFormat::UNKNOWN) {
                                        ctx.format = format;
                                        ctx.w = upng_get_width(upng);
                                        ctx.h = upng_get_height(upng);
                                        ctx.data = std::make_unique<std::uint8_t[]>(ctx.w * ctx.h * bytesPerPixel);
                                        std::memcpy(ctx.data.get(), upng_get_buffer(upng), ctx.w * ctx.h * bytesPerPixel);
                                    }
                                    else {
                                        self->_platform->logError("[TextureProviderImpl::getOrLoadTexture] '%s' must have a valid format (rgba8, lum8)", path.data());
                                    }
                                    
                                    upng_free(upng);
                                }
                                else {
                                    self->_platform->logError("[TextureProviderImpl::getOrLoadTexture] '%s' is not a valid png file", path.data());
                                }
                            }
                            //--- worker thread ---
                        },
                        [weak, path, completion = std::move(completion)](TextureAsyncContext &ctx) {
                            if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                                self->_asyncInProgress = false;
                                
                                if (ctx.data) {
                                    self->_textures.erase(path);
                                    TextureData &texture = self->_textures.emplace(path, TextureData{ctx.w, ctx.h, ctx.format}).first->second;
                                    texture.ptr = self->_rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, ctx.w, ctx.h, {ctx.data.get()});
                                    completion(texture.ptr);
                                }
                                else {
                                    self->_platform->logError("[TextureProviderImpl::getOrLoadTexture] Async operation has failed for file '%s'", path.data());
                                    completion(nullptr);
                                }
                            }
                        }));
                    }
                    else {
                        self->_asyncInProgress = false;
                        self->_platform->logError("[TextureProviderImpl::getOrLoadTexture] Unable to find file '%s'", path.data());
                        completion(nullptr);
                    }
                }
            });
        }
    }
        
    void ResourceProviderImpl::getOrLoadVoxelMesh(const char *meshPath, util::callback<void(const std::vector<foundation::RenderDataPtr> &, const util::IntegerOffset3D &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueueMesh.emplace_back(QueueEntryMesh {
                .meshPath = meshPath,
                .callback = std::move(completion)
            });
            
            return;
        }
        
        std::string path = std::string(meshPath);

        auto index = _meshes.find(path);
        if (index != _meshes.end() && index->second.outdated == false) {
            completion(index->second.frames, index->second.originOffset);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".vxm").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                    if (len) {
                        self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<MeshAsyncContext>>([weak, path, mem = std::move(mem), len](MeshAsyncContext &ctx) {
                            //--- worker thread ---
                            if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                                readMesh(ctx, mem.get());
                            }
                            //--- worker thread ---
                        },
                        [weak, path, completion = std::move(completion)](MeshAsyncContext &ctx) {
                            if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                                self->_asyncInProgress = false;
                                
                                if (ctx.voxels.size()) {
                                    std::vector<foundation::RenderDataPtr> frames (ctx.voxels.size());
                                    for (std::size_t f = 0; f < ctx.voxels.size(); f++) {
                                        frames[f] = self->_rendering->createData(ctx.voxels[f].data(), layouts::VTXMVOX, std::uint32_t(ctx.voxels[f].size()));
                                    }
                                    
                                    self->_meshes.erase(path);
                                    const VoxelMesh &result = self->_meshes.emplace(path, VoxelMesh{std::move(frames), ctx.originOffset}).first->second;
                                    completion(result.frames, result.originOffset);
                                }
                                else {
                                    self->_platform->logError("[ResourceProviderImpl::getOrLoadVoxelMesh] '%s' is not a valid vxm file", path.data());
                                }
                            }
                        }));
                    }
                    else {
                        self->_asyncInProgress = false;
                        self->_platform->logError("[ResourceProviderImpl::getOrLoadVoxelMesh] Unable to find file '%s'", path.data());
                        completion({}, {0, 0, 0});
                    }
                }
            });
        }
    }
    
    // TODO: separate ground and its texture, make it like emitters
    void ResourceProviderImpl::getOrLoadGround(const char *groundPath, util::callback<void(const foundation::RenderDataPtr &, const foundation::RenderTexturePtr &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueueGround.emplace_back(QueueEntryGround {
                .groundPath = groundPath,
                .callback = std::move(completion)
            });
            
            return;
        }
        
        std::string path = std::string(groundPath);
        
        auto index = _grounds.find(path);
        if (index != _grounds.end() && index->second.outdated == false) {
            completion(index->second.data, index->second.texture);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".grd").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                    struct GroundData : public GroundInfo {
                        struct Vertex {
                            float x, y, z;
                            float nx, ny, nz;
                            float u, v;
                        };

                        std::vector<Vertex> vertexes;
                        std::vector<std::uint32_t> indexes;
                    };
                    
                    if (len) {
                        const std::uint8_t *data = mem.get();
                        
                        if (memcmp(data, "GROUND", 6) == 0) {
                            std::unique_ptr<GroundData> ground = std::make_unique<GroundData>();
                            
                            data += 16;
                            ground->sizeX = *(int *)(data + 0);
                            ground->sizeY = *(int *)(data + 4);
                            ground->sizeZ = *(int *)(data + 8);
                            int vxcnt = *(int *)(data + 12);
                            int ixcnt = *(int *)(data + 16);
                            data += 20;
                            
                            ground->vertexes.reserve(vxcnt);
                            ground->indexes.reserve(ixcnt);
                            
                            for (int i = 0; i < vxcnt; i++) {
                                ground->vertexes.emplace_back(reinterpret_cast<const GroundData::Vertex *>(data)[i]);
                            }
                            data += sizeof(GroundData::Vertex) * vxcnt;
                            for (int i = 0; i < ixcnt; i++) {
                                ground->indexes.emplace_back(reinterpret_cast<const std::uint32_t *>(data)[i]);
                            }
                            data += sizeof(std::uint32_t) * ixcnt;
                            std::uint32_t textureFlags = *(std::uint32_t *)data;
                            data += sizeof(std::uint32_t);
                            std::size_t offset = data - mem.get();
                            
                            struct AsyncContext {
                                std::unique_ptr<std::uint8_t[]> data;
                                std::uint32_t w, h;
                            };
                            self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak, path, bin = std::move(mem), offset, len](AsyncContext &ctx) {
                                if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                                    const std::uint8_t *data = bin.get() + offset;
                                    upng_t *upng = upng_new_from_bytes(data, (unsigned long)(len - offset));
                                    if (upng != nullptr && *reinterpret_cast<const unsigned *>(data) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK) {
                                        if (upng_get_format(upng) == UPNG_LUMINANCE8) {
                                            ctx.w = upng_get_width(upng);
                                            ctx.h = upng_get_height(upng);
                                            ctx.data = std::make_unique<std::uint8_t[]>(ctx.w * ctx.h);
                                            std::memcpy(ctx.data.get(), upng_get_buffer(upng), ctx.w * ctx.h);
                                        }
                                        else {
                                            self->_platform->logError("[GroundProviderImpl::getOrLoadGround] '%s' must have lum8 image", path.data());
                                        }
                                    }
                                    else {
                                        self->_platform->logError("[GroundProviderImpl::getOrLoadGround] '%s' does not contain valid image", path.data());
                                    }
                                }
                            },
                            [weak, path, completion = std::move(completion), gd = std::move(ground)](AsyncContext &ctx) mutable {
                                if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                                    self->_asyncInProgress = false;
                                    
                                    if (ctx.data && gd) {
                                        std::uint32_t vcnt = std::uint32_t(gd->vertexes.size());
                                        std::uint32_t icnt = std::uint32_t(gd->indexes.size());
                                        
                                        self->_grounds.erase(path);
                                        GroundMesh &groundMesh = self->_grounds.emplace(path, GroundMesh{}).first->second;
                                        groundMesh.texture = self->_rendering->createTexture(foundation::RenderTextureFormat::R8UN, ctx.w, ctx.h, {ctx.data.get()});
                                        groundMesh.data = self->_rendering->createData(gd->vertexes.data(), layouts::VTXNRMUV, vcnt, gd->indexes.data(), icnt);
                                        completion(groundMesh.data, groundMesh.texture);
                                    }
                                    else {
                                        completion(nullptr, nullptr);
                                    }
                                }
                            }));
                        }
                        else {
                            self->_platform->logError("[GroundProviderImpl::getOrLoadGround] '%s' is not a valid ground file", path.data());
                        }
                    }
                    else {
                        self->_platform->logError("[MeshProviderImpl::getOrLoadGround] Unable to find file '%s'", path.data());
                        completion(nullptr, nullptr);
                    }
                }
            });
        }
    }

    void ResourceProviderImpl::getOrLoadEmitter(const char *configPath, util::callback<void(const resource::EmitterDescriptionPtr &, const foundation::RenderTexturePtr &, const foundation::RenderTexturePtr &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueueEmitter.emplace_back(QueueEntryEmitter {
                .configPath = configPath,
                .callback = std::move(completion)
            });
            
            return;
        }
        
        std::string path = std::string(configPath);

        auto index = _emitters.find(path);
        if (index != _emitters.end() && index->second.outdated == false) {
            completion(index->second.params, index->second.map, index->second.texture);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".bin").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                    if (len) {
                        EmitterDescriptionPtr desc = std::make_unique<EmitterDescription>();
                        if (readEmitter(*desc, mem.get())) {
                            self->_emitters.erase(path);
                            Emitter &emitter = self->_emitters.emplace(path, Emitter{}).first->second;
                            emitter.params = std::move(desc);
                            
                            self->_asyncInProgress = false;
                            self->getOrLoadTexture((path + ".map").data(), [weak, &emitter, completion = std::move(completion)](const foundation::RenderTexturePtr &mapTexture) mutable {
                                if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                                    emitter.map = mapTexture;
                                    
                                    self->getOrLoadTexture(emitter.params->texturePath.data(), [weak, &emitter, completion = std::move(completion)](const foundation::RenderTexturePtr &texture) mutable {
                                        if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                                            emitter.texture = texture;
                                            completion(emitter.params, emitter.map, emitter.texture);
                                        }
                                    });
                                }
                            });
                        }
                        else {
                            self->_asyncInProgress = false;
                            self->_platform->logError("[ResourceProviderImpl::getOrLoadEmitter] '%s' is not a valid emitter binary file", path.data());
                            completion(nullptr, nullptr, nullptr);
                        }
                    }
                    else {
                        self->_asyncInProgress = false;
                        self->_platform->logError("[ResourceProviderImpl::getOrLoadEmitter] Unable to find file '%s'", path.data());
                        completion(nullptr, nullptr, nullptr);
                    }
                }
            });
        }

    }

    void ResourceProviderImpl::removeTexture(const char *texturePath) {
        auto index = _textures.find(texturePath);
        if (index != _textures.end()) {
            index->second.outdated = true;
        }
    }
    void ResourceProviderImpl::removeMesh(const char *meshPath) {
        auto index = _meshes.find(meshPath);
        if (index != _meshes.end()) {
            index->second.outdated = true;
        }
    }
    void ResourceProviderImpl::removeGround(const char *groundPath) {
        auto index = _grounds.find(groundPath);
        if (index != _grounds.end()) {
            index->second.outdated = true;
        }
    }
    void ResourceProviderImpl::removeEmitter(const char *configPath) {
        auto index = _emitters.find(configPath);
        if (index != _emitters.end()) {
            index->second.outdated = true;
        }
    }
    
    void ResourceProviderImpl::update(float dtSec) {
        while (_asyncInProgress == false && _callsQueueTexture.size()) {
            QueueEntryTexture &entry = _callsQueueTexture.front();
            getOrLoadTexture(entry.texPath.data(), std::move(entry.callbackPtr));
            _callsQueueTexture.pop_front();
        }
        while (_asyncInProgress == false && _callsQueueMesh.size()) {
            QueueEntryMesh &entry = _callsQueueMesh.front();
            getOrLoadVoxelMesh(entry.meshPath.data(), std::move(entry.callback));
            _callsQueueMesh.pop_front();
        }
        while (_asyncInProgress == false && _callsQueueGround.size()) {
            QueueEntryGround &entry = _callsQueueGround.front();
            getOrLoadGround(entry.groundPath.data(), std::move(entry.callback));
            _callsQueueGround.pop_front();
        }
        while (_asyncInProgress == false && _callsQueueEmitter.size()) {
            QueueEntryEmitter &entry = _callsQueueEmitter.front();
            getOrLoadEmitter(entry.configPath.data(), std::move(entry.callback));
            _callsQueueEmitter.pop_front();
        }
    }
}

namespace resource {
    std::shared_ptr<ResourceProvider> ResourceProvider::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const foundation::RenderingInterfacePtr &rendering) {
        return std::make_shared<ResourceProviderImpl>(platform, rendering);
    }
}
