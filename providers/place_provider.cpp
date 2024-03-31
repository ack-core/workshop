
#include "place_provider.h"
#include "places_list.h"
#include "thirdparty/upng/upng.h"

#include <list>

namespace resource {
    class PlaceProviderImpl : public std::enable_shared_from_this<PlaceProviderImpl>, public PlaceProvider {
    public:
        PlaceProviderImpl(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering);
        ~PlaceProviderImpl() override;
        
        const PlaceInfo *getPlaceInfo(const char *placePath) override;
        void getOrLoadPlace(const char *placePath, util::callback<void(const std::unique_ptr<PlaceData> &)> &&completion) override;
        void update(float dtSec) override;
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        const std::shared_ptr<foundation::RenderingInterface> _rendering;
        
        std::unordered_map<std::string, std::unique_ptr<PlaceData>> _places;
        
        struct QueueEntry {
            std::string placePath;
            util::callback<void(const std::unique_ptr<PlaceData> &)> callback;
        };
        
        std::list<QueueEntry> _callsQueue;
        bool _asyncInProgress;
    };
    
    PlaceProviderImpl::PlaceProviderImpl(
        const std::shared_ptr<foundation::PlatformInterface> &platform,
        const foundation::RenderingInterfacePtr &rendering
    )
    : _platform(platform)
    , _rendering(rendering)
    , _asyncInProgress(false)
    {
    }
    PlaceProviderImpl::~PlaceProviderImpl() {
    
    }
    
    const PlaceInfo *PlaceProviderImpl::getPlaceInfo(const char *placePath) {
        auto index = PLACES_LIST.find(placePath);
        return index != PLACES_LIST.end() ? &index->second : nullptr;
    }
    
    void PlaceProviderImpl::getOrLoadPlace(const char *placePath, util::callback<void(const std::unique_ptr<PlaceData> &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueue.emplace_back(QueueEntry {
                .placePath = placePath,
                .callback = std::move(completion)
            });
            
            return;
        }
        
        std::string path = std::string(placePath);
        
        auto index = _places.find(path);
        if (index != _places.end()) {
            completion(index->second);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".plc").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<PlaceProviderImpl> self = weak.lock()) {
                    if (len) {
                        const std::uint8_t *data = mem.get();
                        
                        if (memcmp(data, "PLACE", 5) == 0) {
                            std::unique_ptr<PlaceData> place = std::make_unique<PlaceData>();
                            
                            data += 16;
                            place->sizeX = *(int *)(data + 0);
                            place->sizeY = *(int *)(data + 4);
                            place->sizeZ = *(int *)(data + 8);
                            int vxcnt = *(int *)(data + 12);
                            int ixcnt = *(int *)(data + 16);
                            data += 20;
                            
                            place->vertexes.reserve(vxcnt);
                            place->indexes.reserve(ixcnt);
                            
                            for (int i = 0; i < vxcnt; i++) {
                                place->vertexes.emplace_back(reinterpret_cast<const PlaceData::Vertex *>(data)[i]);
                            }
                            data += sizeof(PlaceData::Vertex) * vxcnt;
                            
                            for (int i = 0; i < ixcnt; i++) {
                                place->indexes.emplace_back(reinterpret_cast<const std::uint32_t *>(data)[i]);
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
                                if (std::shared_ptr<PlaceProviderImpl> self = weak.lock()) {
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
                                            self->_platform->logError("[PlaceProviderImpl::getOrLoadPlace] '%s' must have lum8 image", path.data());
                                        }
                                    }
                                    else {
                                        self->_platform->logError("[PlaceProviderImpl::getOrLoadPlace] '%s' does not contain valid image", path.data());
                                    }
                                }
                            },
                            [weak, path, completion = std::move(completion), placeData = std::move(place)](AsyncContext &ctx) mutable {
                                if (std::shared_ptr<PlaceProviderImpl> self = weak.lock()) {
                                    self->_asyncInProgress = false;
                                    
                                    if (ctx.data && placeData) {
                                        placeData->texture = self->_rendering->createTexture(foundation::RenderTextureFormat::R8UN, ctx.w, ctx.h, {ctx.data.get()});
                                        const std::unique_ptr<PlaceData> &place = self->_places.emplace(path, std::move(placeData)).first->second;
                                        completion(place);
                                    }
                                    else {
                                        completion(nullptr);
                                    }
                                }
                            }));
                        }
                        else {
                            self->_platform->logError("[PlaceProviderImpl::getOrLoadPlace] '%s' is not a valid place file", path.data());
                        }
                    }
                    else {
                        self->_platform->logError("[MeshProviderImpl::getOrLoadPlace] Unable to find file '%s'", path.data());
                        completion(nullptr);
                    }
                }
            });
        }
    }
    
    void PlaceProviderImpl::update(float dtSec) {
        while (_asyncInProgress == false && _callsQueue.size()) {
            QueueEntry &entry = _callsQueue.front();
            getOrLoadPlace(entry.placePath.data(), std::move(entry.callback));
            _callsQueue.pop_front();
        }
    }
}

namespace resource {
    std::shared_ptr<PlaceProvider> PlaceProvider::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const foundation::RenderingInterfacePtr &rendering) {
        return std::make_shared<PlaceProviderImpl>(platform, rendering);
    }
}
