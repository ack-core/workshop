
#include "texture_provider.h"
#include "thirdparty/upng/upng.h"

#include <unordered_map>
#include <sstream>

namespace resource {
    class TextureProviderImpl : public std::enable_shared_from_this<TextureProviderImpl>, public TextureProvider {
    public:
        TextureProviderImpl(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering, const char *resourceList);
        ~TextureProviderImpl() override;
        
        const TextureInfo *getTextureInfo(const char *texPath) override;
        void getOrLoad2DTexture(const char *name, std::function<void(const foundation::RenderTexturePtr &)> &&completion) override;
        void getOrLoadGrayscaleData(const char *name, std::function<void(const std::unique_ptr<std::uint8_t[]> &data, std::uint32_t w, std::uint32_t h)> &&completion) override;
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        const std::shared_ptr<foundation::RenderingInterface> _rendering;
        const foundation::RenderTexturePtr _empty;
        
        struct RawTexture {
            std::unique_ptr<std::uint8_t[]> data;
            std::uint32_t width;
            std::uint32_t height;
        };
        
        std::unordered_map<std::string, foundation::RenderTexturePtr> _textures2d;
        std::unordered_map<std::string, RawTexture> _grayscaleTextures;
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
        std::string line;
        
        while (std::getline(source, line)) {
            printf("-->> %s\n", line.data());
        }
    }
    
    TextureProviderImpl::~TextureProviderImpl() {
    
    }
    
    const TextureInfo *TextureProviderImpl::getTextureInfo(const char *texPath) {
        return nullptr;
    }
    
    void TextureProviderImpl::getOrLoad2DTexture(const char *texPath, std::function<void(const foundation::RenderTexturePtr &)> &&completion) {
        auto index = _textures2d.find(texPath);
        if (index != _textures2d.end()) {
            completion(index->second);
        }
        else {
            if (texPath[0]) {
                std::string path = std::string(texPath) + ".png";
                _platform->loadFile(path.data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&data, std::size_t size) {
                    if (std::shared_ptr<TextureProviderImpl> self = weak.lock()) {
                        if (size) {
                            upng_t *upng = upng_new_from_bytes(data.get(), (unsigned long)(size));
                            
                            if (upng != nullptr) {
                                if (*reinterpret_cast<const unsigned *>(data.get()) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK && upng_get_format(upng) == UPNG_RGBA8) {
                                    std::uint32_t w = upng_get_width(upng);
                                    std::uint32_t h = upng_get_height(upng);
                                    
                                    foundation::RenderTexturePtr &texture = self->_textures2d.emplace(path, foundation::RenderTexturePtr{}).first->second;
                                    texture = self->_rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, w, h, { upng_get_buffer(upng) });
                                    completion(texture);
                                    return;
                                }
                                else {
                                    self->_platform->logError("[TextureProviderImpl::getOrLoad2DTexture] '%s' is not UPNG_RGBA8 file", path.data());
                                }
                                
                                upng_free(upng);
                            }
                            else {
                                self->_platform->logError("[TextureProviderImpl::getOrLoad2DTexture] '%s' is not valid png file", path.data());
                            }
                        }
                        else {
                            self->_platform->logError("[TextureProviderImpl::getOrLoad2DTexture] Unable to find file '%s'", path.data());
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
    
    void TextureProviderImpl::getOrLoadGrayscaleData(const char *texPath, std::function<void(const std::unique_ptr<std::uint8_t[]> &data, std::uint32_t w, std::uint32_t h)> &&completion) {
        auto index = _grayscaleTextures.find(texPath);
        if (index != _grayscaleTextures.end()) {
            completion(index->second.data, index->second.width, index->second.height);
        }
        else {
            if (texPath[0]) {
                std::string path = std::string(texPath) + ".png";
                _platform->loadFile(path.data(), [weak = weak_from_this(), path, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&data, std::size_t size) {
                    if (std::shared_ptr<TextureProviderImpl> self = weak.lock()) {
                        if (size) {
                            upng_t *upng = upng_new_from_bytes(data.get(), (unsigned long)(size));
                            
                            if (upng != nullptr) {
                                // TODO: decode in async
                                if (*reinterpret_cast<const unsigned *>(data.get()) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK && upng_get_format(upng) == UPNG_LUMINANCE8) {
                                    RawTexture rawTexture;
                                    rawTexture.width = upng_get_width(upng);
                                    rawTexture.height = upng_get_height(upng);
                                    rawTexture.data = std::make_unique<std::uint8_t[]>(rawTexture.width * rawTexture.height);
                                    memcpy(rawTexture.data.get(), upng_get_buffer(upng), rawTexture.width * rawTexture.height);
                                    
                                    RawTexture &texture = self->_grayscaleTextures.emplace(path, std::move(rawTexture)).first->second;
                                    completion(texture.data, texture.width, texture.height);
                                    return;
                                }
                                else {
                                    self->_platform->logError("[TextureProviderImpl::getOrLoadGrayscaleTextureData] '%s' is not UPNG_LUMINANCE8 file", path.data());
                                }
                                
                                upng_free(upng);
                            }
                            else {
                                self->_platform->logError("[TextureProviderImpl::getOrLoadGrayscaleTextureData] '%s' is not valid png file", path.data());
                            }
                        }
                        else {
                            self->_platform->logError("[TextureProviderImpl::getOrLoadGrayscaleTextureData] Unable to find file '%s'", path.data());
                        }
                        
                        completion(nullptr, 0, 0);
                    }
                });
            }
            else {
                completion(nullptr, 0, 0);
            }
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
