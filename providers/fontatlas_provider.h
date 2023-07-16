
#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/math.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace resource {
    struct FontCharInfo {
        foundation::RenderTexturePtr texture = nullptr;
        math::vector2f txLT;
        math::vector2f txRB;
        math::vector2f pxSize;
        float advance, lsb, voffset;
    };

    class FontAtlasProvider {
    public:
        static std::shared_ptr<FontAtlasProvider> instance(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            std::unique_ptr<std::uint8_t[]> &&ttfData,
            std::uint32_t ttfLen
        );
        
    public:
        // Returns width of text in pixels
        //
        virtual float getTextWidth(const char *text, std::uint8_t size) const = 0;
        
        // Rasterize font to atlases according to @text and @size
        // @return  - FontAtlasInfo with coordinates and textures
        //
        virtual void getTextFontAtlas(const char *text, std::uint8_t size, util::callback<void(std::vector<FontCharInfo> &&)> &&completion) = 0;
        
    public:
        virtual ~FontAtlasProvider() = default;
    };
    
    using FontAtlasProviderPtr = std::shared_ptr<FontAtlasProvider>;
}

