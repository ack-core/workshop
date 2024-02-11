
#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace resource {
    struct TextureInfo {
        std::uint32_t width;
        std::uint32_t height;
        
        enum class Type {
            UNKNOWN,
            GRAYSCALE8,
            RGBA8,
        }
        type;
    };
    
    class TextureProvider {
    public:
        static std::shared_ptr<TextureProvider> instance(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering
        );
        
    public:
        // Get texture info
        // @texPath - path to file without extension
        // @return  - info or nullptr
        //
        virtual const TextureInfo *getTextureInfo(const char *texPath) = 0;
        
        // Asynchronously Load texture from file if it isn't loaded yet
        // @texPath - path to file without extension
        // @return  - raw data and size or nullptr
        //
        virtual void getOrLoadTexture(const char *texPath, util::callback<void(const std::unique_ptr<std::uint8_t[]> &, const resource::TextureInfo &)> &&completion) = 0;
        
        // Asynchronously Load texture from file if it isn't loaded yet
        // @texPath - path to file without extension
        // @return  - texture object or nullptr
        //
        virtual void getOrLoadTexture(const char *texPath, util::callback<void(const foundation::RenderTexturePtr &)> &&completion) = 0;
        
        // Provider tracks resources life time and tries to free them
        //
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~TextureProvider() = default;
    };
    
    using TextureProviderPtr = std::shared_ptr<TextureProvider>;
}

