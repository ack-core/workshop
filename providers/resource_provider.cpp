
#include "resource_provider.h"
#include "textures_list.h"
#include "meshes_list.h"
#include "grounds_list.h"
#include "foundation/layouts.h"

#include "thirdparty/upng/upng.h"

#include <list>
#include <memory>

namespace {
    struct MeshAsyncContext {
        struct Voxel {
            std::int16_t positionX, positionY, positionZ;
            std::uint8_t colorIndex, mask, scaleX, scaleY, scaleZ, reserved;
        };
        struct Frame {
            std::unique_ptr<Voxel[]> voxels;
            std::uint16_t voxelCount = 0;
        };
    
        std::unique_ptr<Frame[]> frames;
        std::uint16_t frameCount = 0;
        std::size_t maxVoxelCount = 0;
    };
    
    bool readVox(MeshAsyncContext &ctx, const std::uint8_t *data) {
        if (memcmp(data, "VOX ", 4) == 0) {
            if (*(std::int32_t *)(data + 4) == 0x7f) { // vox made by gen_meshes.py
                data += 32;
                ctx.frameCount = *(std::uint32_t *)data;
                ctx.frames = std::make_unique<MeshAsyncContext::Frame[]>(ctx.frameCount);
                data += sizeof(std::uint32_t);
                
                for (std::uint32_t f = 0; f < ctx.frameCount; f++) {
                    MeshAsyncContext::Frame &frame = ctx.frames[f];
                    frame.voxelCount = *(std::uint32_t *)data;
                    frame.voxels = std::make_unique<MeshAsyncContext::Voxel[]>(frame.voxelCount);
                    
                    if (frame.voxelCount > ctx.maxVoxelCount) {
                        ctx.maxVoxelCount = frame.voxelCount;
                    }
                    
                    data += sizeof(std::uint32_t);
                    
                    for (std::uint32_t i = 0; i < frame.voxelCount; i++) {
                        frame.voxels[i] = *(MeshAsyncContext::Voxel *)data;
                        data += sizeof(MeshAsyncContext::Voxel);
                    }
                }
                
                return true;
            }
        }
        
        return false;
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
        void getOrLoadVoxelStatic(const char *voxPath, util::callback<void(const foundation::RenderDataPtr &)> &&completion) override;
        void getOrLoadVoxelObject(const char *voxPath, util::callback<void(const std::vector<foundation::RenderDataPtr> &)> &&completion) override;
        void getOrLoadGround(const char *groundPath, util::callback<void(const foundation::RenderDataPtr &, const foundation::RenderTexturePtr &)> &&completion) override;

        void update(float dtSec) override;
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        const std::shared_ptr<foundation::RenderingInterface> _rendering;
        
        struct TextureData : public TextureInfo { // TODO: remove
            foundation::RenderTexturePtr ptr;
        };
        struct GroundMesh {
            foundation::RenderDataPtr data;
            foundation::RenderTexturePtr texture;
        };
        
        std::unordered_map<std::string, TextureData> _textures;
        std::unordered_map<std::string, foundation::RenderDataPtr> _statics;
        std::unordered_map<std::string, std::vector<foundation::RenderDataPtr>> _objects;
        std::unordered_map<std::string, GroundMesh> _grounds;
        
        struct QueueEntryTexture {
            std::string texPath;
            util::callback<void(const foundation::RenderTexturePtr &)> callbackPtr;
        };
        struct QueueEntryStatic {
            std::string voxPath;
            util::callback<void(const foundation::RenderDataPtr &)> callback;
        };
        struct QueueEntryObject {
            std::string voxPath;
            util::callback<void(const std::vector<foundation::RenderDataPtr> &)> callback;
        };
        struct QueueEntryGround {
            std::string groundPath;
            util::callback<void(const foundation::RenderDataPtr &, const foundation::RenderTexturePtr &)> callback;
        };
        
        std::list<QueueEntryTexture> _callsQueueTexture;
        std::list<QueueEntryStatic> _callsQueueStatic;
        std::list<QueueEntryObject> _callsQueueObject;
        std::list<QueueEntryGround> _callsQueueGround;
        
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
        if (index != _textures.end() && index->second.ptr) {
            completion(index->second.ptr);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".png").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<std::uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                    if (len) {
                        struct TextureAsyncContext {
                            std::unique_ptr<std::uint8_t[]> data;
                            std::uint32_t w, h;
                            foundation::RenderTextureFormat format;
                        };
                        
                        self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<TextureAsyncContext>>([weak, path, mem = std::move(mem), len](TextureAsyncContext &ctx) {
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
                        },
                        [weak, path, completion = std::move(completion)](TextureAsyncContext &ctx) {
                            if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                                self->_asyncInProgress = false;
                                
                                if (ctx.data) {
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
                        self->_platform->logError("[TextureProviderImpl::getOrLoadTexture] Unable to find file '%s'", path.data());
                        completion(nullptr);
                    }
                }
            });
        }
    }
    
    void ResourceProviderImpl::getOrLoadVoxelStatic(const char *voxPath, util::callback<void(const foundation::RenderDataPtr &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueueStatic.emplace_back(QueueEntryStatic {
                .voxPath = voxPath,
                .callback = std::move(completion)
            });
            
            return;
        }
        
        std::string path = std::string(voxPath);

        auto index = _statics.find(path);
        if (index != _statics.end()) {
            completion(index->second);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".vox").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                    if (len) {
                        self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<MeshAsyncContext>>([weak, path, bin = std::move(mem), len](MeshAsyncContext &ctx) {
                            if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                                if (readVox(ctx, bin.get()) == false) {
                                    self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelStatic] '%s' is not a valid vox file", path.data());
                                }
                            }
                        },
                        [weak, path, completion = std::move(completion)](MeshAsyncContext &ctx) {
                            if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                                self->_asyncInProgress = false;
                                
                                if (ctx.frameCount) {
                                    struct VTXSVOX {
                                        std::int16_t positionX, positionY, positionZ;
                                        std::uint8_t colorIndex, mask;
                                        std::uint8_t scaleX, scaleY, scaleZ, reserved;
                                    };
                                    
                                    std::vector<VTXSVOX> voxels (ctx.frames[0].voxelCount);
                                    
                                    for (std::size_t i = 0; i < voxels.size(); i++) {
                                        const MeshAsyncContext::Voxel &src = ctx.frames[0].voxels[i];
                                        VTXSVOX &voxel = voxels[i];
                                        voxel.positionX = src.positionX;
                                        voxel.positionY = src.positionY;
                                        voxel.positionZ = src.positionZ;
                                        voxel.colorIndex = src.colorIndex;
                                        voxel.mask = src.mask;
                                        voxel.scaleX = src.scaleX;
                                        voxel.scaleY = src.scaleY;
                                        voxel.scaleZ = src.scaleZ;
                                        voxel.reserved = 0;
                                    }
                                    
                                    foundation::RenderDataPtr mesh = self->_rendering->createData(voxels.data(), layouts::VTXSVOX, ctx.frames[0].voxelCount);
                                    self->_statics.emplace(path, mesh);
                                    completion(mesh);
                                }
                                else {
                                    completion(nullptr);
                                }
                            }
                        }));
                    }
                    else {
                        self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelStatic] Unable to find file '%s'", path.data());
                        completion(nullptr);
                    }
                }
            });
        }
    }
    
    void ResourceProviderImpl::getOrLoadVoxelObject(const char *voxPath, util::callback<void(const std::vector<foundation::RenderDataPtr> &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueueObject.emplace_back(QueueEntryObject {
                .voxPath = voxPath,
                .callback = std::move(completion)
            });
            
            return;
        }
        
        std::string path = std::string(voxPath);

        auto index = _objects.find(path);
        if (index != _objects.end()) {
            completion(index->second);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".vox").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                    if (len) {
                        self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<MeshAsyncContext>>([weak, path, bin = std::move(mem), len](MeshAsyncContext &ctx) {
                            if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                                if (readVox(ctx, bin.get()) == false) {
                                    self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelObject] '%s' is not a valid vox file", path.data());
                                }
                            }
                        },
                        [weak, path, completion = std::move(completion)](MeshAsyncContext &ctx) {
                            if (std::shared_ptr<ResourceProviderImpl> self = weak.lock()) {
                                self->_asyncInProgress = false;
                                
                                if (ctx.frameCount) {
                                    struct VTXDVOX {
                                        float positionX, positionY, positionZ;
                                        std::uint8_t colorIndex, mask, r0, r1;
                                    };
                                    
                                    std::vector<foundation::RenderDataPtr> frames (ctx.frameCount);
                                    std::vector<VTXDVOX> voxels (ctx.maxVoxelCount);
                                    
                                    for (std::size_t f = 0; f < frames.size(); f++) {
                                        for (std::size_t i = 0; i < voxels.size(); i++) {
                                            const MeshAsyncContext::Voxel &src = ctx.frames[f].voxels[i];
                                            VTXDVOX &voxel = voxels[i];
                                            voxel.positionX = float(src.positionX);
                                            voxel.positionY = float(src.positionY);
                                            voxel.positionZ = float(src.positionZ);
                                            voxel.colorIndex = src.colorIndex;
                                            voxel.mask = src.mask;
                                            voxel.r0 = 0;
                                            voxel.r1 = 0;
                                        }
                                        
                                        frames[f] = self->_rendering->createData(voxels.data(), layouts::VTXDVOX, ctx.frames[f].voxelCount);
                                    }
                                    
                                    const std::vector<foundation::RenderDataPtr> &result = self->_objects.emplace(path, std::move(frames)).first->second;
                                    completion(result);
                                }
                                else {
                                    completion({});
                                }
                            }
                        }));
                    }
                    else {
                        self->_platform->logError("[MeshProviderImpl::getOrLoadVoxelObject] Unable to find file '%s'", path.data());
                        completion({});
                    }
                }
            });
        }
    }
    
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
        if (index != _grounds.end()) {
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
    
    void ResourceProviderImpl::update(float dtSec) {
        while (_asyncInProgress == false && _callsQueueTexture.size()) {
            QueueEntryTexture &entry = _callsQueueTexture.front();
            getOrLoadTexture(entry.texPath.data(), std::move(entry.callbackPtr));
            _callsQueueTexture.pop_front();
        }
        while (_asyncInProgress == false && _callsQueueStatic.size()) {
            QueueEntryStatic &entry = _callsQueueStatic.front();
            getOrLoadVoxelStatic(entry.voxPath.data(), std::move(entry.callback));
            _callsQueueStatic.pop_front();
        }
        while (_asyncInProgress == false && _callsQueueObject.size()) {
            QueueEntryObject &entry = _callsQueueObject.front();
            getOrLoadVoxelObject(entry.voxPath.data(), std::move(entry.callback));
            _callsQueueObject.pop_front();
        }
        while (_asyncInProgress == false && _callsQueueGround.size()) {
            QueueEntryGround &entry = _callsQueueGround.front();
            getOrLoadGround(entry.groundPath.data(), std::move(entry.callback));
            _callsQueueGround.pop_front();
        }
    }
}

namespace resource {
    std::shared_ptr<ResourceProvider> ResourceProvider::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const foundation::RenderingInterfacePtr &rendering) {
        return std::make_shared<ResourceProviderImpl>(platform, rendering);
    }
}
