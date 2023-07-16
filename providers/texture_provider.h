
#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace resource {
    struct TextureInfo {
        int width;
        int height;
        
        enum class Type {
            GRAYSCALE8,
            RGBA8
        }
        type;
    };
    
    class TextureProvider {
    public:
        static std::shared_ptr<TextureProvider> instance(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            const char *resourceList
        );
        
    public:
        // Get texture info
        // @texPath - path to file without extension
        // @return  - info or nullptr
        //
        virtual const TextureInfo *getTextureInfo(const char *texPath) = 0;
        
        // Load texture from file if it isn't loaded yet
        // @texPath - path to file without extension
        // @return  - texture object or nullptr
        //
        virtual void getOrLoad2DTexture(const char *texPath, util::callback<void(const foundation::RenderTexturePtr &)> &&completion) = 0;
        
        // Load grayscale data from file if it isn't loaded yet
        // @texPath - path to file without extension
        // @return  - raw grayscale data and size or nullptr
        //
        virtual void getOrLoadGrayscaleData(const char *texPath, util::callback<void(const std::unique_ptr<std::uint8_t[]> &data, std::uint32_t w, std::uint32_t h)> &&completion) = 0;
        
    public:
        virtual ~TextureProvider() = default;
    };
    
    using TextureProviderPtr = std::shared_ptr<TextureProvider>;
}

