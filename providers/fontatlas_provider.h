
#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/math.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace resource {
    struct FontCharInfo {
        math::vector2f txLT;
        math::vector2f txRB;
        math::vector2f pxSize;
        float advance, lsb, voffset;
        std::uint32_t u16ch;
    };

    class FontAtlasProvider {
    public:
        static std::shared_ptr<FontAtlasProvider> instance(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            ByteDataPtr &&ttfData,
            std::size_t ttfLen
        );
        
    public:
        // Returns width of text in pixels
        //
        virtual auto getTextWidth(const char *text, std::uint8_t fontSize) const -> math::vector2f = 0;

        // Rasterize font to atlases according to @text and @size
        //
        virtual void prepareFontAtlas(const char *text, std::uint8_t fontSize, std::uint8_t blur, util::callback<void()> &&completion) = 0;
        
        // Returns rasterized text with @text and @size
        // @return  - FontAtlasInfo with coordinates and textures
        //
        virtual auto getTextFontAtlas(const char *text, std::uint8_t fontSize, std::uint8_t blur, std::vector<resource::FontCharInfo> &outInfo) -> const foundation::RenderTexturePtr & = 0;
        
        // Provider tracks resources life time and tries to free them
        //
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~FontAtlasProvider() = default;
    };
    
    using FontAtlasProviderPtr = std::shared_ptr<FontAtlasProvider>;
}

