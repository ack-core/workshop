
#include "texture_provider.h"
#include "thirdparty/upng/upng.h"

#include <unordered_map>
#include <sstream>
#include <list>

namespace resource {
    class TextureProviderImpl : public std::enable_shared_from_this<TextureProviderImpl>, public TextureProvider {
    public:
        TextureProviderImpl(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering, const char *resourceList);
        ~TextureProviderImpl() override;
        
        const TextureInfo *getTextureInfo(const char *texPath) override;

        void getOrLoadTexture(const char *texPath, util::callback<void(const std::unique_ptr<std::uint8_t[]> &, const TextureInfo &)> &&completion) override;
        void getOrLoadTexture(const char *texPath, util::callback<void(const foundation::RenderTexturePtr &)> &&completion) override;
        auto getOrLoadTexture(const char *texPath) -> const foundation::RenderTexturePtr override;

        void update(float dtSec) override;
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        const std::shared_ptr<foundation::RenderingInterface> _rendering;
        const foundation::RenderTexturePtr _empty;
        
        struct TextureData : public TextureInfo {
            std::unique_ptr<std::uint8_t[]> data;
            foundation::RenderTexturePtr ptr;
        };
        
        std::unordered_map<std::string, TextureData> _textures;
        
        struct QueueEntry {
            std::string texPath;
            util::callback<void(const std::unique_ptr<std::uint8_t[]> &, const TextureInfo &)> callbackData;
            util::callback<void(const foundation::RenderTexturePtr &)> callbackPtr;
        };
        
        std::list<QueueEntry> _callsQueue;
        bool _asyncInProgress;
    };
    
    TextureProviderImpl::TextureProviderImpl(
        const std::shared_ptr<foundation::PlatformInterface> &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const char *resourceList
    )
    : _platform(platform)
    , _rendering(rendering)
    {
        std::istringstream source = std::istringstream(resourceList);
        std::string line, path, type;
        
        while (std::getline(source, line)) {
            printf("-->> %s", line.data());
            std::istringstream input = std::istringstream(line);
            TextureData data = {};
            
            if (line.length()) {
                if (input >> path >> type >> data.width >> data.height) {
                    if (type == "rgba") {
                        data.type = TextureInfo::Type::RGBA8;
                    }
                    else if (type == "grayscale") {
                        data.type = TextureInfo::Type::GRAYSCALE8;
                    }
                    else {
                        _platform->logError("[TextureProviderImpl::TextureProviderImpl] Unknown texture type '%s'\n", type.data());
                        break;
                    }
                    
                    _textures.emplace(path, std::move(data));
                }
                else {
                    _platform->logError("[TextureProviderImpl::TextureProviderImpl] Bad texture info '%s'\n", line.data());
                }
            }
        }
    }
    
    TextureProviderImpl::~TextureProviderImpl() {
    
    }
    
    const TextureInfo *TextureProviderImpl::getTextureInfo(const char *texPath) {
        auto index = _textures.find(texPath);
        return index != _textures.end() ? &index->second : nullptr;
    }
    
    namespace {
        struct AsyncContext {
            std::unique_ptr<std::uint8_t[]> data;
            std::uint32_t w, h;
            TextureInfo::Type type;
            foundation::RenderTextureFormat format;
        };
        
        void decodePngToContext(const foundation::PlatformInterfacePtr &platform, AsyncContext &ctx, const char *path, std::uint8_t *source, std::size_t len) {
            upng_t *upng = upng_new_from_bytes(source, (unsigned long)(len));
            
            if (upng != nullptr && *reinterpret_cast<const unsigned *>(source) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK) {
                foundation::RenderTextureFormat format = foundation::RenderTextureFormat::UNKNOWN;
                TextureInfo::Type type = TextureInfo::Type::UNKNOWN;
                std::uint32_t bytesPerPixel = 0;
                
                if (upng_get_format(upng) == UPNG_RGBA8) {
                    format = foundation::RenderTextureFormat::RGBA8UN;
                    type = TextureInfo::Type::RGBA8;
                    bytesPerPixel = 4;
                }
                else if (upng_get_format(upng) == UPNG_LUMINANCE8) {
                    format = foundation::RenderTextureFormat::R8UN;
                    type = TextureInfo::Type::GRAYSCALE8;
                    bytesPerPixel = 1;
                }
                
                if (type != TextureInfo::Type::UNKNOWN) {
                    ctx.format = format;
                    ctx.type = type;
                    ctx.w = upng_get_width(upng);
                    ctx.h = upng_get_height(upng);
                    ctx.data = std::make_unique<std::uint8_t[]>(ctx.w * ctx.h * bytesPerPixel);
                    std::memcpy(ctx.data.get(), upng_get_buffer(upng), ctx.w * ctx.h * bytesPerPixel);
                }
                else {
                    platform->logError("[TextureProviderImpl::getOrLoadTexture] '%s' must have a valid format (rgba8, lum8)", path);
                }
                
                upng_free(upng);
            }
            else {
                platform->logError("[TextureProviderImpl::getOrLoadTexture] '%s' is not a valid png file", path);
            }
        }
    }
    
    void TextureProviderImpl::getOrLoadTexture(const char *texPath, util::callback<void(const std::unique_ptr<std::uint8_t[]> &, const TextureInfo &)> &&completion) {
        if (_asyncInProgress) {
            _callsQueue.emplace_back(QueueEntry {
                .texPath = texPath,
                .callbackData = std::move(completion)
            });
            
            return;
        }
        
        std::string path = std::string(texPath);
        
        auto index = _textures.find(path);
        if (index != _textures.end() && index->second.data) {
            completion(index->second.data, index->second);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".png").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<std::uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<TextureProviderImpl> self = weak.lock()) {
                    if (len) {
                        self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak, path, mem = std::move(mem), len](AsyncContext &ctx) {
                            if (std::shared_ptr<TextureProviderImpl> self = weak.lock()) {
                                decodePngToContext(self->_platform, ctx, path.data(), mem.get(), len);
                            }
                        },
                        [weak, path, completion = std::move(completion)](AsyncContext &ctx) {
                            if (std::shared_ptr<TextureProviderImpl> self = weak.lock()) {
                                self->_asyncInProgress = false;
                                
                                if (ctx.data) {
                                    TextureData &texture = self->_textures.emplace(path, TextureData{ctx.w, ctx.h, ctx.type}).first->second;
                                    texture.data = std::move(ctx.data);
                                    completion(texture.data, texture);
                                }
                                else {
                                    self->_platform->logError("[TextureProviderImpl::getOrLoadTexture (data)] Async operation has failed for file '%s'", path.data());
                                    completion(nullptr, {});
                                }
                            }
                        }));
                    }
                    else {
                        self->_platform->logError("[TextureProviderImpl::getOrLoadTexture (data)] Unable to find file '%s'", path.data());
                        completion(nullptr, {});
                    }
                }
            });
        }
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
                        self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak, path, mem = std::move(mem), len](AsyncContext &ctx) {
                            if (std::shared_ptr<TextureProviderImpl> self = weak.lock()) {
                                decodePngToContext(self->_platform, ctx, path.data(), mem.get(), len);
                            }
                        },
                        [weak, path, completion = std::move(completion)](AsyncContext &ctx) {
                            if (std::shared_ptr<TextureProviderImpl> self = weak.lock()) {
                                self->_asyncInProgress = false;
                                
                                if (ctx.data) {
                                    TextureData &texture = self->_textures.emplace(path, TextureData{ctx.w, ctx.h, ctx.type}).first->second;
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

    const foundation::RenderTexturePtr TextureProviderImpl::getOrLoadTexture(const char *texPath) {
        std::unique_ptr<uint8_t[]> data;
        std::size_t size;
        
        if (_platform->loadFile((std::string(texPath) + ".png").data(), data, size)) {
            upng_t *upng = upng_new_from_bytes(data.get(), (unsigned long)(size));
            
            if (upng != nullptr && *reinterpret_cast<const unsigned *>(data.get()) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK) {
                if (upng_get_format(upng) == UPNG_RGBA8) {
                    TextureData &texture = _textures.emplace(texPath, TextureData{upng_get_width(upng), upng_get_height(upng), TextureInfo::Type::RGBA8}).first->second;
                    texture.ptr = _rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, upng_get_width(upng), upng_get_height(upng), { upng_get_buffer(upng) });
                    return texture.ptr;
                }
                else if (upng_get_format(upng) == UPNG_LUMINANCE8) {
                    TextureData &texture = _textures.emplace(texPath, TextureData{upng_get_width(upng), upng_get_height(upng), TextureInfo::Type::GRAYSCALE8}).first->second;
                    texture.ptr = _rendering->createTexture(foundation::RenderTextureFormat::R8UN, upng_get_width(upng), upng_get_height(upng), { upng_get_buffer(upng) });
                    return texture.ptr;
                }
                else {
                    _platform->logError("[TextureProviderImpl::getOrLoadTexture] '%s' must have a valid format (rgba8, lum8)", texPath);
                }
                
                upng_free(upng);
            }
            else {
                _platform->logError("[TextureProviderImpl::getOrLoadTexture] '%s' is not a valid png file", texPath);
            }
        }
        
        return nullptr;
    }
    
    void TextureProviderImpl::update(float dtSec) {
        while (_asyncInProgress == false && _callsQueue.size()) {
            QueueEntry &entry = _callsQueue.front();
            
            if (entry.callbackData) {
                getOrLoadTexture(entry.texPath.data(), std::move(entry.callbackData));
            }
            if (entry.callbackPtr) {
                getOrLoadTexture(entry.texPath.data(), std::move(entry.callbackPtr));
            }
            
            _callsQueue.pop_front();
        }
    }
}

namespace resource {
    std::shared_ptr<TextureProvider> TextureProvider::instance(
        const std::shared_ptr<foundation::PlatformInterface> &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const char *resourceList
    ) {
        return std::make_shared<TextureProviderImpl>(platform, rendering, resourceList);
    }
}
