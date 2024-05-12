
#include "ground_provider.h"
#include "grounds_list.h"
#include "foundation/layouts.h"
#include "thirdparty/upng/upng.h"

#include <list>

namespace resource {
    class GroundProviderImpl : public std::enable_shared_from_this<GroundProviderImpl>, public GroundProvider {
    public:
        GroundProviderImpl(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering);
        ~GroundProviderImpl() override;
        
        const GroundInfo *getGroundInfo(const char *groundPath) override;
        void getOrLoadGround(const char *groundPath, util::callback<void(const foundation::RenderDataPtr &, const foundation::RenderTexturePtr &)> &&completion) override;
        void update(float dtSec) override;
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        const std::shared_ptr<foundation::RenderingInterface> _rendering;
        
        struct GroundData : public GroundInfo {
            struct Vertex {
                float x, y, z;
                float nx, ny, nz;
                float u, v;
            };

            std::vector<Vertex> vertexes;
            std::vector<std::uint32_t> indexes;
        };
        struct GroundMesh {
            foundation::RenderDataPtr data;
            foundation::RenderTexturePtr texture;
        };
        
        std::unordered_map<std::string, GroundMesh> _grounds;
        
        struct QueueEntry {
            std::string groundPath;
            util::callback<void(const foundation::RenderDataPtr &, const foundation::RenderTexturePtr &)> callback;
        };
        
        std::list<QueueEntry> _callsQueue;
        bool _asyncInProgress;
    };
    
    GroundProviderImpl::GroundProviderImpl(
        const std::shared_ptr<foundation::PlatformInterface> &platform,
        const foundation::RenderingInterfacePtr &rendering
    )
    : _platform(platform)
    , _rendering(rendering)
    , _asyncInProgress(false)
    {
    }
    GroundProviderImpl::~GroundProviderImpl() {
    
    }
    
    const GroundInfo *GroundProviderImpl::getGroundInfo(const char *groundPath) {
        auto index = GROUNDS_LIST.find(groundPath);
        return index != GROUNDS_LIST.end() ? &index->second : nullptr;
    }
    
    void GroundProviderImpl::getOrLoadGround(const char *groundPath, util::callback<void(const foundation::RenderDataPtr &, const foundation::RenderTexturePtr &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueue.emplace_back(QueueEntry {
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
                if (std::shared_ptr<GroundProviderImpl> self = weak.lock()) {
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
                                if (std::shared_ptr<GroundProviderImpl> self = weak.lock()) {
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
                                if (std::shared_ptr<GroundProviderImpl> self = weak.lock()) {
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
    
    void GroundProviderImpl::update(float dtSec) {
        while (_asyncInProgress == false && _callsQueue.size()) {
            QueueEntry &entry = _callsQueue.front();
            getOrLoadGround(entry.groundPath.data(), std::move(entry.callback));
            _callsQueue.pop_front();
        }
    }
}

namespace resource {
    std::shared_ptr<GroundProvider> GroundProvider::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const foundation::RenderingInterfacePtr &rendering) {
        return std::make_shared<GroundProviderImpl>(platform, rendering);
    }
}
