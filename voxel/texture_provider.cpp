
#include "texture_provider.h"
#include "thirdparty/upng/upng.h"

#include <unordered_map>

namespace voxel {
    class TextureProviderImpl : public TextureProvider {
    public:
        TextureProviderImpl(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering);
        ~TextureProviderImpl() override;
        
        const foundation::RenderTexturePtr &getOrLoad2DTexture(const char *name) override;
        bool getOrLoadTextureData(const char *name, std::unique_ptr<std::uint8_t[]> &data, std::uint32_t &w, std::uint32_t &h) override;
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        const std::shared_ptr<foundation::RenderingInterface> _rendering;
        const foundation::RenderTexturePtr _empty;
        
        std::unordered_map<std::string, foundation::RenderTexturePtr> _textures2d;
        std::unordered_map<std::string, upng_t *> _texturesRaw;
    };
    
    TextureProviderImpl::TextureProviderImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const foundation::RenderingInterfacePtr &rendering)
    : _platform(platform)
    , _rendering(rendering)
    {
    
    }
    
    TextureProviderImpl::~TextureProviderImpl() {
    
    }
    
    const foundation::RenderTexturePtr &TextureProviderImpl::getOrLoad2DTexture(const char *name) {
        auto index = _textures2d.find(name);
        if (index != _textures2d.end()) {
            return index->second;
        }
        else {
            std::unique_ptr<std::uint8_t[]> data;
            std::size_t size;
            
            if (_platform->loadFile((std::string(name) + ".png").data(), data, size)) {
                upng_t *upng = upng_new_from_bytes(data.get(), (unsigned long)(size));
                
                if (upng != nullptr) {
                    if (*reinterpret_cast<const unsigned *>(data.get()) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK && upng_get_format(upng) == UPNG_RGBA8) {
                        std::uint32_t w = upng_get_width(upng);
                        std::uint32_t h = upng_get_height(upng);
                        
                        foundation::RenderTexturePtr &texture = _textures2d.emplace(std::string(name), foundation::RenderTexturePtr{}).first->second;
                        texture = _rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, w, h, { upng_get_buffer(upng) });
                        return texture;
                    }
                    
                    upng_free(upng);
                }
            }
        }
        
        return _empty;
    }
    
    bool TextureProviderImpl::getOrLoadTextureData(const char *name, std::unique_ptr<std::uint8_t[]> &data, std::uint32_t &w, std::uint32_t &h) {
        auto index = _texturesRaw.find(name);
        if (index != _texturesRaw.end()) {
            w = upng_get_width(index->second);
            h = upng_get_height(index->second);
            std::uint32_t len = w * h * upng_get_components(index->second);
            data = std::make_unique<std::uint8_t[]>(len);
            memcpy(data.get(), upng_get_buffer(index->second), len);
            return true;
        }
        else {
            std::unique_ptr<std::uint8_t[]> binary;
            std::size_t size;
            
            if (_platform->loadFile((std::string(name) + ".png").data(), binary, size)) {
                upng_t *upng = upng_new_from_bytes(binary.get(), (unsigned long)(size));
                
                if (upng != nullptr) {
                    if (*reinterpret_cast<const unsigned *>(binary.get()) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK) {
                        _texturesRaw.emplace(std::string(name), upng);
                        w = upng_get_width(upng);
                        h = upng_get_height(upng);
                        std::uint32_t len = w * h * upng_get_components(upng);
                        data = std::make_unique<std::uint8_t[]>(len);
                        memcpy(data.get(), upng_get_buffer(upng), len);
                        upng_free(upng);
                        return true;
                    }
                    
                    upng_free(upng);
                }
            }
        }
        
        return false;
    }
}

namespace voxel {
    std::shared_ptr<TextureProvider> TextureProvider::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const foundation::RenderingInterfacePtr &rendering) {
        return std::make_shared<TextureProviderImpl>(platform, rendering);
    }
}
