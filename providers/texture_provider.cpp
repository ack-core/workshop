
#include "texture_provider.h"
#include "textures_list.h"
#include "thirdparty/upng/upng.h"

#include <list>
#include <memory>

namespace resource {
    class TextureProviderImpl : public std::enable_shared_from_this<TextureProviderImpl>, public TextureProvider {
    public:
        TextureProviderImpl(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering);
        ~TextureProviderImpl() override;
        
        const TextureInfo *getTextureInfo(const char *texPath) override;
        
        void getOrLoadTexture(const char *texPath, util::callback<void(const foundation::RenderTexturePtr &)> &&completion) override;
        void update(float dtSec) override;
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        const std::shared_ptr<foundation::RenderingInterface> _rendering;
        
        struct TextureData : public TextureInfo {
            foundation::RenderTexturePtr ptr;
        };
        
        std::unordered_map<std::string, TextureData> _textures;
        
        struct QueueEntry {
            std::string texPath;
            util::callback<void(const foundation::RenderTexturePtr &)> callbackPtr;
        };
        
        std::list<QueueEntry> _callsQueue;
        bool _asyncInProgress;
    };
    
    TextureProviderImpl::TextureProviderImpl(
        const std::shared_ptr<foundation::PlatformInterface> &platform,
        const foundation::RenderingInterfacePtr &rendering
    )
    : _platform(platform)
    , _rendering(rendering)
    , _asyncInProgress(false)
    {
    }
    
    TextureProviderImpl::~TextureProviderImpl() {
    
    }
    
    const TextureInfo *TextureProviderImpl::getTextureInfo(const char *texPath) {
        auto index = TEXTURES_LIST.find(texPath);
        return index != TEXTURES_LIST.end() ? &index->second : nullptr;
    }
    
    void TextureProviderImpl::getOrLoadTexture(const char *texPath, util::callback<void(const foundation::RenderTexturePtr &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueue.emplace_back(QueueEntry {
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
                if (std::shared_ptr<TextureProviderImpl> self = weak.lock()) {
                    if (len) {
                        struct AsyncContext {
                            std::unique_ptr<std::uint8_t[]> data;
                            std::uint32_t w, h;
                            foundation::RenderTextureFormat format;
                        };
                        
                        self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak, path, mem = std::move(mem), len](AsyncContext &ctx) {
                            if (std::shared_ptr<TextureProviderImpl> self = weak.lock()) {
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
                        [weak, path, completion = std::move(completion)](AsyncContext &ctx) {
                            if (std::shared_ptr<TextureProviderImpl> self = weak.lock()) {
                                self->_asyncInProgress = false;
                                
                                if (ctx.data) {
                                    TextureData &texture = self->_textures.emplace(path, TextureData{ctx.w, ctx.h, ctx.format}).first->second;
                                    texture.ptr = self->_rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, ctx.w, ctx.h, {ctx.data.get()});
                                    completion(texture.ptr);
                                }
                                else {
                                    self->_platform->logError("[TextureProviderImpl::getOrLoadTexture (ptr)] Async operation has failed for file '%s'", path.data());
                                    completion(nullptr);
                                }
                            }
                        }));
                    }
                    else {
                        self->_platform->logError("[TextureProviderImpl::getOrLoadTexture (ptr)] Unable to find file '%s'", path.data());
                        completion(nullptr);
                    }
                }
            });
        }
    }
    
    void TextureProviderImpl::update(float dtSec) {
        while (_asyncInProgress == false && _callsQueue.size()) {
            QueueEntry &entry = _callsQueue.front();
            
            if (entry.callbackPtr) {
                getOrLoadTexture(entry.texPath.data(), std::move(entry.callbackPtr));
            }
            
            _callsQueue.pop_front();
        }
    }
}

namespace resource {
    std::shared_ptr<TextureProvider> TextureProvider::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const foundation::RenderingInterfacePtr &rendering) {
        return std::make_shared<TextureProviderImpl>(platform, rendering);
    }
}
