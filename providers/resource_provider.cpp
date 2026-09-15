
#include "resource_provider.h"
#include "resource_list.h"
#include "foundation/layouts.h"

#include "thirdparty/upng/upng.h"

#include <list>
#include <memory>

namespace {
    struct TextureAsyncContext {
        ByteDataPtr data;
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
        util::Description description;
        std::vector<std::vector<VTXMVOX>> voxels;
    };
    struct GroundAsyncContext {
        struct Vertex {
            float x, y, z;
            float nx, ny, nz;
            float u, v;
        };
        struct VGEntry {
            enum class SourceType {
                TEXTURE = 0,
                VOXMESH,
            };
            util::Description description;
            ByteDataPtr sourceData;
            math::vector2i sourceSize;
            SourceType sourceType;
        };

        math::vector2i textureSize;
        ByteDataPtr textureData;
        ByteDataPtr mapData;
        std::vector<Vertex> vertexes;
        std::vector<std::uint32_t> indexes;
        std::vector<VGEntry> vg;
    };

    static_assert(sizeof(MeshAsyncContext::VTXMVOX) == 8, "");
    
    bool readEmitter(const std::uint8_t *data, util::Description &desc, size_t &read) {
        const std::uint8_t *origin = data;
        if (memcmp(data, "EMTR", 4) == 0) {
            const std::size_t descLen = *(std::uint32_t *)(data + 4) - 4;
            const std::uint8_t *src = data + 8;
            desc = util::Description::parse(src, descLen);
            read = 8 + descLen;
            return true;
        }
        return false;
    }

    void readMesh(MeshAsyncContext &ctx, const std::uint8_t *data) {
        ctx.voxels.clear();
        
        if (memcmp(data, "VOX ", 4) == 0) {
            if (*(std::int32_t *)(data + 4) == 0x7f) { // vox made by gen_meshes.py
                data += 24;
                const std::uint32_t cfglen = *(std::uint32_t *)(data + 0);
                ctx.description = util::Description::parse((const std::uint8_t *)(data + 4), cfglen);
                data += 4 + cfglen;
                const math::vector3f originOffset = ctx.description.getVector3f("offset", {});
                
                std::uint32_t frameCount = *(std::uint32_t *)data;
                ctx.voxels.resize(frameCount);
                data += sizeof(std::uint32_t);
                
                for (std::uint32_t f = 0; f < frameCount; f++) {
                    const std::uint32_t voxelCount = *(std::uint32_t *)data;
                    data += sizeof(std::uint32_t);
                    ctx.voxels[f].resize(voxelCount);
                    
                    for (std::uint32_t i = 0; i < voxelCount; i++) {
                        const MeshAsyncContext::Voxel &src = *(MeshAsyncContext::Voxel *)data;
                        MeshAsyncContext::VTXMVOX &voxel = ctx.voxels[f][i];
                        voxel.positionX = src.positionX - originOffset.x;
                        voxel.positionY = src.positionY - originOffset.y;
                        voxel.positionZ = src.positionZ - originOffset.z;
                        voxel.colorIndex = src.colorIndex;
                        voxel.mask = src.mask;

                        data += sizeof(MeshAsyncContext::Voxel);
                    }
                }
            }
        }
    }

    void readGround(GroundAsyncContext &ctx, const std::uint8_t *data, std::size_t len) {
        const std::uint8_t *binstart = data;
        
        if (memcmp(data, "GROUND", 6) == 0) {
            data += 16;
            const int sizeX = *(int *)(data + 0);
            const int sizeY = *(int *)(data + 4);
            const int sizeZ = *(int *)(data + 8);
            data += 12;
            
            ctx.vg.reserve(4);
            
            const int descLen = *(int *)(data + 0);
            util::Description vg = util::Description::parse((const std::uint8_t *)(data + 4), descLen);
            for (auto &item : vg.getDescriptions("vegetation")) {
                ctx.vg.emplace_back(GroundAsyncContext::VGEntry { std::move(*item), nullptr });
            }
            data += 4 + descLen;
            
            const int vxcnt = *(int *)(data + 0);
            const int ixcnt = *(int *)(data + 4);
            data += 8;
            
            ctx.vertexes.reserve(vxcnt);
            ctx.indexes.reserve(ixcnt);
            
            for (int i = 0; i < vxcnt; i++) {
                ctx.vertexes.emplace_back(reinterpret_cast<const GroundAsyncContext::Vertex *>(data)[i]);
            }
            data += sizeof(GroundAsyncContext::Vertex) * vxcnt;
            for (int i = 0; i < ixcnt; i++) {
                ctx.indexes.emplace_back(reinterpret_cast<const std::uint32_t *>(data)[i]);
            }
            data += sizeof(std::uint32_t) * ixcnt;
            const std::uint32_t vgEntriesCount = *(std::uint32_t *)(data + 0);
            const std::uint32_t textureOffset = *(std::uint32_t *)(data + 4);
            const std::uint32_t textureLen = *(std::uint32_t *)(data + 8);
            const std::uint32_t mapOffset = *(std::uint32_t *)(data + 12);
            const std::uint32_t mapLen = *(std::uint32_t *)(data + 16);
            data += 5 * sizeof(std::uint32_t);
            
            upng_t *upng = upng_new_from_bytes(binstart + textureOffset, (unsigned long)textureLen);
            if (upng != nullptr) {
                if (*reinterpret_cast<const std::uint32_t *>(binstart + textureOffset) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK && upng_get_format(upng) == UPNG_LUMINANCE8) {
                    ctx.textureSize = math::vector2i(upng_get_width(upng), upng_get_height(upng));
                    ctx.textureData = std::make_unique<std::uint8_t[]>(ctx.textureSize.x * ctx.textureSize.y);
                    std::memcpy(ctx.textureData.get(), upng_get_buffer(upng), ctx.textureSize.x * ctx.textureSize.y);
                }
                
                upng_free(upng);
            }
            if (ctx.textureData && ctx.vg.size() == vgEntriesCount) {
                upng_t *upng = upng_new_from_bytes(binstart + mapOffset, (unsigned long)mapLen);
                if (upng != nullptr) {
                    const math::vector2i mapSize = math::vector2i(ctx.textureSize.x + 1, ctx.textureSize.y + 1);
                    if (*reinterpret_cast<const std::uint32_t *>(binstart + mapOffset) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK && upng_get_format(upng) == UPNG_RGBA8) {
                        ctx.mapData = std::make_unique<std::uint8_t[]>(mapSize.x * mapSize.y * 4);
                        std::memcpy(ctx.mapData.get(), upng_get_buffer(upng), mapSize.x * mapSize.y * 4);
                    }
                    
                    upng_free(upng);
                }
                for (std::uint32_t i = 0; i < vgEntriesCount; i++) {
                    ctx.vg[i].sourceType = GroundAsyncContext::VGEntry::SourceType(ctx.vg[i].description.getInteger("geometry", 0) == 2);
                    const std::uint32_t vgSourceOffset = *(std::uint32_t *)(data + 0);
                    const std::uint32_t vgSourceLen = *(std::uint32_t *)(data + 4);
                    data += 8;
                    
                    if (vgSourceLen) {
                        if (ctx.vg[i].sourceType == GroundAsyncContext::VGEntry::SourceType::TEXTURE) {
                            upng_t *upng = upng_new_from_bytes(binstart + vgSourceOffset, (unsigned long)vgSourceLen);
                            if (upng != nullptr) {
                                if (*reinterpret_cast<const std::uint32_t *>(binstart + vgSourceOffset) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK && upng_get_format(upng) == UPNG_LUMINANCE8) {
                                    int w = upng_get_width(upng);
                                    int h = upng_get_height(upng);
                                    ctx.vg[i].sourceSize = math::vector2i(w, h);
                                    ctx.vg[i].sourceData = std::make_unique<std::uint8_t[]>(w * h);
                                    std::memcpy(ctx.vg[i].sourceData.get(), upng_get_buffer(upng), w * h);
                                }
                                
                                upng_free(upng);
                            }
                        }
                        if (ctx.vg[i].sourceType == GroundAsyncContext::VGEntry::SourceType::VOXMESH) {
                            const std::uint32_t voxelCount = *(std::uint32_t *)(binstart + vgSourceOffset);
                            ctx.vg[i].sourceSize = math::vector2i(voxelCount, 0);
                            ctx.vg[i].sourceData = std::make_unique<std::uint8_t[]>(sizeof(MeshAsyncContext::VTXMVOX) * voxelCount);
                            for (std::uint32_t c = 0; c < voxelCount; c++) {
                                const MeshAsyncContext::Voxel *src = (const MeshAsyncContext::Voxel *)(binstart + vgSourceOffset + 4);
                                MeshAsyncContext::VTXMVOX *voxels = reinterpret_cast<MeshAsyncContext::VTXMVOX *>(ctx.vg[i].sourceData.get());
                                voxels[c].positionX = src[c].positionX;
                                voxels[c].positionY = src[c].positionY;
                                voxels[c].positionZ = src[c].positionZ;
                                voxels[c].colorIndex = src[c].colorIndex;
                                voxels[c].mask = src[c].mask;
                            }
                        }
                    }
                }
                return;
            }

            ctx.indexes.clear();
            ctx.vertexes.clear();
            ctx.textureData = nullptr;
            ctx.mapData = nullptr;
        }
    }

    bool readPrefabs(std::unordered_map<std::string, util::Description> &prefabs, const std::uint8_t *data) {
        if (memcmp(data, "PREFABS!", 4) == 0) {
            std::uint32_t prefabsCount = *(std::uint32_t *)(data + 8);
            data += 12;
            prefabs.clear();
            
            for (std::uint32_t i = 0; i < prefabsCount; i++) {
                std::string prefabPath = reinterpret_cast<const char *>(data + 4);
                data += *(std::uint32_t *)(data + 0);
                const std::size_t descLen = *(std::uint32_t *)(data + 0) - 4;
                prefabs.emplace(std::move(prefabPath), util::Description::parse(data + 4, descLen));
                data += *(std::uint32_t *)(data + 0);
            }
            
            return true;
        }

        return false;
    }
}

namespace resource {
    class ResourceProviderImpl : public std::enable_shared_from_this<ResourceProviderImpl>, public ResourceProvider {
    public:
        ResourceProviderImpl(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            const ByteDataPtr &prefabSrcData,
            std::size_t prefabSrcLength
        );
        ~ResourceProviderImpl() override;
        
        auto getTextureInfo(const char *texPath) -> const TextureInfo * override;
        auto getMeshInfo(const char *voxPath) -> const MeshInfo * override;
        auto getGroundInfo(const char *groundPath) -> const GroundInfo * override;
        
        void getOrLoadTexture(const char *texPath, util::callback<void(const foundation::RenderTexturePtr &)> &&completion) override;
        void getOrLoadVoxelMesh(const char *meshPath, util::callback<void(const std::vector<foundation::RenderDataPtr> &, const util::Description &)> &&completion) override;
        void getOrLoadGround(const char *groundPath, util::callback<void(const foundation::RenderDataPtr &, const foundation::RenderTexturePtr &, const GroundMapDescription &)> &&completion) override;
        void getOrLoadEmitter(const char *descPath, util::callback<void(const util::Description &, const foundation::RenderTexturePtr &, const foundation::RenderTexturePtr &)> &&completion) override;
        void getOrLoadDescription(const char *descPath, util::callback<void(const util::Description &)> &&completion) override;
        
        auto getPrefab(const char *prefabPath) -> const util::Description & override;
        
        void removeTexture(const char *texturePath) override;
        void removeMesh(const char *meshPath) override;
        void removeGround(const char *groundPath) override;
        void removeEmitter(const char *configPath) override;
        void removeDescription(const char *descPath) override;
        void reloadPrefabs(util::callback<void()> &&completion) override;
        
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
            util::Description description;
            bool outdated = false;
        };
        struct GroundMesh {
            foundation::RenderDataPtr data;
            foundation::RenderTexturePtr texture;
            GroundMapDescription mapDesc;
            bool outdated = false;
        };
        struct Emitter {
            util::Description params;
            foundation::RenderTexturePtr map;
            foundation::RenderTexturePtr texture;
            bool outdated = false;
        };
        struct Description {
            util::Description desc;
            bool outdated = false;
        };
        
        std::unordered_map<std::string, TextureData> _textures;
        std::unordered_map<std::string, VoxelMesh> _meshes;
        std::unordered_map<std::string, GroundMesh> _grounds;
        std::unordered_map<std::string, Emitter> _emitters;
        std::unordered_map<std::string, Description> _descriptions;

        std::unordered_map<std::string, util::Description> _prefabs;
        
        struct QueueEntryTexture {
            std::string texPath;
            util::callback<void(const foundation::RenderTexturePtr &)> callbackPtr;
        };
        struct QueueEntryMesh {
            std::string meshPath;
            util::callback<void(const std::vector<foundation::RenderDataPtr> &, const util::Description &)> callback;
        };
        struct QueueEntryGround {
            std::string groundPath;
            util::callback<void(const foundation::RenderDataPtr &, const foundation::RenderTexturePtr &, const GroundMapDescription &)> callback;
        };
        struct QueueEntryEmitter {
            std::string descPath;
            util::callback<void(const util::Description &, const foundation::RenderTexturePtr &, const foundation::RenderTexturePtr &)> callback;
        };
        struct QueueEntryDescription {
            std::string descPath;
            util::callback<void(const util::Description &)> callback;
        };

        std::list<QueueEntryTexture> _callsQueueTexture;
        std::list<QueueEntryMesh> _callsQueueMesh;
        std::list<QueueEntryGround> _callsQueueGround;
        std::list<QueueEntryEmitter> _callsQueueEmitter;
        std::list<QueueEntryDescription> _callsQueueDescription;

        bool _asyncInProgress;
    };
    
    ResourceProviderImpl::ResourceProviderImpl(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const ByteDataPtr &prefabSrcData,
        std::size_t prefabSrcLength
    )
    : _platform(platform)
    , _rendering(rendering)
    , _asyncInProgress(false)
    {
        if (readPrefabs(_prefabs, prefabSrcData.get()) == false) {
            _platform->logError("[ResourceProviderImpl::ResourceProviderImpl] Invalid prefabs.bin");
        }
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
            _platform->loadFile((path + ".png").data(), [weak = weak_from_this(), path, completion = std::move(completion)](ByteDataPtr &&mem, std::size_t len) mutable {
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
        
    void ResourceProviderImpl::getOrLoadVoxelMesh(const char *meshPath, util::callback<void(const std::vector<foundation::RenderDataPtr> &, const util::Description &)> &&completion) {
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
            completion(index->second.frames, index->second.description);
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
                                        frames[f] = self->_rendering->createData(layouts::VTXMVOX, ctx.voxels[f].data(), std::uint32_t(ctx.voxels[f].size()));
                                    }
                                    
                                    self->_meshes.erase(path);
                                    const VoxelMesh &result = self->_meshes.emplace(path, VoxelMesh{std::move(frames), std::move(ctx.description)}).first->second;
                                    completion(result.frames, result.description);
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
                        completion({}, {});
                    }
                }
            });
        }
    }
    
    void ResourceProviderImpl::getOrLoadGround(const char *groundPath, util::callback<void(const foundation::RenderDataPtr &, const foundation::RenderTexturePtr &, const GroundMapDescription &)> &&completion) {
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
            completion(index->second.data, index->second.texture, index->second.mapDesc);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".grd").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                    if (len) {
                        self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<GroundAsyncContext>>([weak, path, mem = std::move(mem), len](GroundAsyncContext &ctx) {
                            //--- worker thread ---
                            if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                                readGround(ctx, mem.get(), len);
                            }
                            //--- worker thread ---
                        },
                        [weak, path, completion = std::move(completion)](GroundAsyncContext &ctx) {
                            if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                                self->_asyncInProgress = false;

                                if (ctx.textureData && ctx.indexes.size()) {
                                    const std::uint32_t vcnt = std::uint32_t(ctx.vertexes.size());
                                    const std::uint32_t icnt = std::uint32_t(ctx.indexes.size());
                                    
                                    self->_grounds.erase(path);
                                    GroundMesh &groundMesh = self->_grounds.emplace(path, GroundMesh{}).first->second;
                                    groundMesh.texture = self->_rendering->createTexture(foundation::RenderTextureFormat::R8UN, ctx.textureSize.x, ctx.textureSize.y, {ctx.textureData.get()});
                                    groundMesh.data = self->_rendering->createData(layouts::VTXNRMUV, ctx.vertexes.data(), vcnt, ctx.indexes.data(), icnt);
                                    groundMesh.mapDesc.vegetation.reserve(ctx.vg.size());
                                    groundMesh.mapDesc.map = std::move(ctx.mapData);
                                    for (GroundAsyncContext::VGEntry &item : ctx.vg) {
                                        if (item.sourceData) {
                                            GroundMapDescription::Vegetation &newVG = groundMesh.mapDesc.vegetation.emplace_back(GroundMapDescription::Vegetation {});
                                            newVG.description = std::move(item.description);
                                            
                                            if (item.sourceType == GroundAsyncContext::VGEntry::SourceType::TEXTURE) {
                                                newVG.texture = self->_rendering->createTexture(foundation::RenderTextureFormat::R8UN, item.sourceSize.x, item.sourceSize.y, {item.sourceData.get()});
                                            }
                                            if (item.sourceType == GroundAsyncContext::VGEntry::SourceType::VOXMESH) {
                                                newVG.voxelSource = std::move(item.sourceData);
                                                newVG.voxelCount = item.sourceSize.x;
                                            }
                                        }
                                    }
                                    completion(groundMesh.data, groundMesh.texture, groundMesh.mapDesc);
                                }
                                else {
                                    self->_platform->logError("[ResourceProviderImpl::getOrLoadGround] failed to load ground file '%s'", path.data());
                                    completion(nullptr, nullptr, {});
                                }
                            }
                        }));

                    }
                    else {
                        self->_asyncInProgress = false;
                        self->_platform->logError("[ResourceProviderImpl::getOrLoadGround] Unable to find file '%s'", path.data());
                        completion(nullptr, nullptr, {});
                    }
                }
            });
        }
    }

    void ResourceProviderImpl::getOrLoadEmitter(const char *descPath, util::callback<void(const util::Description &, const foundation::RenderTexturePtr &, const foundation::RenderTexturePtr &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueueEmitter.emplace_back(QueueEntryEmitter {
                .descPath = descPath,
                .callback = std::move(completion)
            });
            
            return;
        }
        
        std::string path = std::string(descPath);

        auto index = _emitters.find(path);
        if (index != _emitters.end() && index->second.outdated == false) {
            completion(index->second.params, index->second.map, index->second.texture);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".bin").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                    if (len) {
                        std::size_t imgoff = 0;
                        util::Description desc;
                        
                        // TODO: readEmitter -> async
                        if (readEmitter(mem.get(), desc, imgoff)) {
                            const std::string &texture = desc.getString("texture", "<Unknown>");
                            
                            self->_emitters.erase(path);
                            Emitter &emitter = self->_emitters.emplace(path, Emitter{}).first->second;
                            emitter.params = std::move(desc);
                            
                            const std::uint32_t mapWidth = *(std::int32_t *)(mem.get() + imgoff + 0);
                            const std::uint32_t mapHeight = *(std::int32_t *)(mem.get() + imgoff + 4);
                            const std::uint8_t *rgba = mem.get() + imgoff + 8;
                            
                            if (mapWidth && mapHeight) {
                                emitter.map = self->_rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, mapWidth, mapHeight, { rgba });
                            }
                            
                            self->_asyncInProgress = false;
                            self->getOrLoadTexture(texture.data(), [weak, &emitter, completion = std::move(completion)](const foundation::RenderTexturePtr &texture) mutable {
                                if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                                    emitter.texture = texture;
                                    completion(emitter.params, emitter.map, emitter.texture);
                                }
                            });
                        }
                        else {
                            self->_asyncInProgress = false;
                            self->_platform->logError("[ResourceProviderImpl::getOrLoadEmitter] '%s' is not a valid emitter binary file", path.data());
                            completion({}, nullptr, nullptr);
                        }
                    }
                    else {
                        self->_asyncInProgress = false;
                        self->_platform->logError("[ResourceProviderImpl::getOrLoadEmitter] Unable to find file '%s'", path.data());
                        completion({}, nullptr, nullptr);
                    }
                }
            });
        }

    }
    
    void ResourceProviderImpl::getOrLoadDescription(const char *descPath, util::callback<void(const util::Description &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueueDescription.emplace_back(QueueEntryDescription {
                .descPath = descPath,
                .callback = std::move(completion)
            });
            
            return;
        }
        
        std::string path = std::string(descPath);

        auto index = _descriptions.find(path);
        if (index != _descriptions.end() && index->second.outdated == false) {
            completion(index->second.desc);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".txt").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                    self->_asyncInProgress = false;
                    if (len) {
                        util::Description desc = util::Description::parse(mem.get(), len);
                        
                        if (desc.empty() == false) {
                            self->_descriptions.erase(path);
                            const util::Description &result = self->_descriptions.emplace(path, Description{ std::move(desc), false }).first->second.desc;
                            completion(result);
                        }
                        else {
                            self->_platform->logError("[ResourceProviderImpl::getOrLoadDescription] '%s' is not a valid description", path.data());
                            completion({});
                        }
                    }
                    else {
                        self->_platform->logError("[ResourceProviderImpl::getOrLoadEmitter] Unable to find file '%s'", path.data());
                        completion({});
                    }
                }
            });
        }
    }
    
    const util::Description &ResourceProviderImpl::getPrefab(const char *prefabPath) {
        auto index = _prefabs.find(prefabPath);
        if (index != _prefabs.end()) {
            return index->second;
        }
        else {
            _platform->logError("[ResourceProviderImpl::getPrefab] Unable to find prefab '%s'", prefabPath);
        }
        
        return util::Description::emptyDesc;
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
    void ResourceProviderImpl::removeDescription(const char *descPath) {
        auto index = _descriptions.find(descPath);
        if (index != _descriptions.end()) {
            index->second.outdated = true;
        }
    }
    void ResourceProviderImpl::reloadPrefabs(util::callback<void()> &&completion) {
        _platform->loadFile(resource::PREFAB_BIN, [this, cb = std::move(completion)](std::unique_ptr<std::uint8_t []> &&prefabsData, std::size_t prefabsSize) {
            if (prefabsSize && readPrefabs(_prefabs, prefabsData.get())) {
                cb();
                _platform->logMsg("[ResourceProviderImpl::reloadPrefabs] prefabs.bin reloaded");
            }
            else {
                _platform->logError("[ResourceProviderImpl::reloadPrefabs] Invalid prefabs.bin");
            }
        });
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
            getOrLoadEmitter(entry.descPath.data(), std::move(entry.callback));
            _callsQueueEmitter.pop_front();
        }
        while (_asyncInProgress == false && _callsQueueDescription.size()) {
            QueueEntryDescription &entry = _callsQueueDescription.front();
            getOrLoadDescription(entry.descPath.data(), std::move(entry.callback));
            _callsQueueDescription.pop_front();
        }
    }
}

namespace resource {
    std::shared_ptr<ResourceProvider> ResourceProvider::instance(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const ByteDataPtr &prefabSrcData,
        std::size_t prefabSrcLength
    )
    {
        return std::make_shared<ResourceProviderImpl>(platform, rendering, prefabSrcData, prefabSrcLength);
    }
}
