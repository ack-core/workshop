
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
        void getOrLoadTexture(const char *texPath, util::callback<void(const std::unique_ptr<std::uint8_t[]> &data, std::uint32_t w, std::uint32_t h)> &&completion) override;
        
        
        //void getOrLoad2DTexture(const char *name, util::callback<void(const foundation::RenderTexturePtr &)> &&completion) override;
        //void getOrLoadGrayscaleData(const char *name, util::callback<void(const std::unique_ptr<std::uint8_t[]> &data, std::uint32_t w, std::uint32_t h)> &&completion) override;
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        const std::shared_ptr<foundation::RenderingInterface> _rendering;
        const foundation::RenderTexturePtr _empty;
        
        struct RawTexture {
            std::unique_ptr<std::uint8_t[]> data;
            std::uint32_t width, height;
            std::uint32_t bytesPerPixel;
        };
        
        std::unordered_map<std::string, TextureInfo> _textureInfos;
//        std::unordered_map<std::string, foundation::RenderTexturePtr> _textures2d;
//        std::unordered_map<std::string, RawTexture> _rawTextures;

        std::unordered_map<std::string, RawTexture> _textures;

        struct QueueEntry {
            std::string texPath;
            util::callback<void(const std::unique_ptr<std::uint8_t[]> &data, std::uint32_t w, std::uint32_t h)> callback;
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
        TextureInfo info;
        
        while (std::getline(source, line)) {
            printf("-->> %s\n", line.data());
            std::istringstream input = std::istringstream(line);
            
            if (line.length()) {
                if (input >> path >> type >> info.width >> info.height) {
                    if (type == "rgba") {
                        info.type = TextureInfo::Type::RGBA8;
                    }
                    else if (type == "grayscale") {
                        info.type = TextureInfo::Type::GRAYSCALE8;
                    }
                    else {
                        _platform->logError("[TextureProviderImpl::TextureProviderImpl] Unknown texture type '%s'\n", type.data());
                        break;
                    }
                    
                    _textureInfos.emplace(path, info);
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
        auto index = _textureInfos.find(texPath);
        return index != _textureInfos.end() ? &index->second : nullptr;
    }
    
    void TextureProviderImpl::getOrLoadTexture(const char *texPath, util::callback<void(const std::unique_ptr<std::uint8_t[]> &data, std::uint32_t w, std::uint32_t h)> &&completion) {
        if (_asyncInProgress) {
            _callsQueue.emplace_back(QueueEntry {
                .texPath = texPath,
                .callback = std::move(completion)
            });
            
            return;
        }
        
        std::string path = std::string(texPath);

        auto index = _textures.find(path);
        if (index != _textures.end()) {
            completion(index->second.data, index->second.width, index->second.height);
        }
        else {
            _asyncInProgress = true;
            _platform->loadFile((path + ".png").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<std::uint8_t[]> &&mem, std::size_t len) mutable {
                if (std::shared_ptr<TextureProviderImpl> self = weak.lock()) {
                    if (len) {
                        struct AsyncContext {
                            std::unique_ptr<std::uint8_t[]> data;
                            std::uint32_t w, h;
                            std::uint32_t bytesPerPixel;
                        };
                        
                        self->_platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak, path, mem = std::move(mem), len](AsyncContext &ctx) {
                            if (std::shared_ptr<TextureProviderImpl> self = weak.lock()) {
                                upng_t *upng = upng_new_from_bytes(mem.get(), (unsigned long)(len));
                                
                                if (upng != nullptr && *reinterpret_cast<const unsigned *>(mem.get()) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK) {
                                    ctx.bytesPerPixel = upng_get_components(upng);
                                    ctx.w = upng_get_width(upng);
                                    ctx.h = upng_get_height(upng);
                                    ctx.data = std::make_unique<std::uint8_t[]>(ctx.w * ctx.h * ctx.bytesPerPixel);
                                    std::memcpy(ctx.data.get(), upng_get_buffer(upng), ctx.w * ctx.h * ctx.bytesPerPixel);
                                }
                                else {
                                
                                }
                            }
                        },
                        [weak, path, completion = std::move(completion)](AsyncContext &ctx) {
                            if (std::shared_ptr<TextureProviderImpl> self = weak.lock()) {
                                self->_asyncInProgress = false;

                                if (ctx.data) {
                                    RawTexture &texture = self->_textures.emplace(path, RawTexture{std::move(ctx.data), ctx.w, ctx.h, ctx.bytesPerPixel}).first->second;
                                    completion(texture.data, texture.width, texture.height);
                                }
                                else {
                                    completion(nullptr, 0, 0);
                                }
                                
                                while (self->_asyncInProgress == false && self->_callsQueue.size()) {
                                    QueueEntry &entry = self->_callsQueue.front();
                                    self->getOrLoadTexture(entry.texPath.data(), std::move(entry.callback));
                                    self->_callsQueue.pop_front();
                                }
                            }
                        }));
                    }
                    else {
                        self->_platform->logError("[TextureProviderImpl::getOrLoadTexture] Unable to find file '%s'", path.data());
                        completion(nullptr, 0, 0);
                    }
                }
            });
        }
    }
    
    /*
    void TextureProviderImpl::getOrLoad2DTexture(const char *texPath, util::callback<void(const foundation::RenderTexturePtr &)> &&completion) {
        std::string path = std::string(texPath);
        auto index = _textures2d.find(path);
        if (index != _textures2d.end()) {
            completion(index->second);
        }
        else {
            if (texPath[0]) {
                _platform->loadFile((path + ".png").data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<std::uint8_t[]> &&data, std::size_t size) {
                    if (std::shared_ptr<TextureProviderImpl> self = weak.lock()) {
                        const foundation::PlatformInterfacePtr &platform = self->_platform;
                        
                        if (size) {
                            upng_t *upng = upng_new_from_bytes(data.get(), (unsigned long)(size));
                            
                            if (upng != nullptr) {
                                if (*reinterpret_cast<const unsigned *>(data.get()) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK && upng_get_format(upng) == UPNG_RGBA8) {
                                    std::uint32_t w = upng_get_width(upng);
                                    std::uint32_t h = upng_get_height(upng);
                                    
                                    auto index = self->_textures2d.find(path);
                                    if (index != self->_textures2d.end()) {
                                        completion(index->second);
                                    }
                                    else {
                                        foundation::RenderTexturePtr &texture = self->_textures2d.emplace(path, foundation::RenderTexturePtr{}).first->second;
                                        texture = self->_rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, w, h, { upng_get_buffer(upng) });
                                        completion(texture);
                                    }
                                }
                                else {
                                    platform->logError("[TextureProviderImpl::getOrLoad2DTexture] '%s' is not UPNG_RGBA8 file", path.data());
                                }
                                
                                upng_free(upng);
                                return;
                            }
                            else {
                                platform->logError("[TextureProviderImpl::getOrLoad2DTexture] '%s' is not valid png file", path.data());
                            }
                        }
                        else {
                            platform->logError("[TextureProviderImpl::getOrLoad2DTexture] Unable to find file '%s'", path.data());
                        }
                        
                        completion(nullptr);
                    }
                });
            }
            else {
                completion(nullptr);
            }
        }
    }
    */
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
