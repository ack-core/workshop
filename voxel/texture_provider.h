
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform.h"
#include "foundation/rendering.h"

namespace voxel {
    class TextureProvider {
    public:
        static std::shared_ptr<TextureProvider> instance(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering);
        
    public:
        // Load texture from file if it isn't loaded yet
        // @name   - path to '.png' file without extension
        // @return - texture object
        //
        virtual auto getOrLoad2DTexture(const char *name) -> const foundation::RenderTexturePtr & = 0;
        virtual bool getOrLoadTextureData(const char *name, std::unique_ptr<std::uint8_t[]> &data, std::uint32_t &w, std::uint32_t &h) = 0;
        
    public:
        virtual ~TextureProvider() = default;
    };
    
    using TextureProviderPtr = std::shared_ptr<TextureProvider>;
}

