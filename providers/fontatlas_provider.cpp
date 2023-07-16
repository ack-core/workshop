
#include "fontatlas_provider.h"
#include "thirdparty/stb_mini_ttf/stb_mini_ttf.h"

#include <list>
#include <unordered_map>
#include <mutex>
#include <bit>

namespace {
    std::uint32_t utf8ToUTF16(const char *utf8Char, std::uint32_t &utf8Len) {
        const std::uint8_t *src = (const std::uint8_t *)utf8Char;
        std::uint32_t length = 1;
        std::uint32_t result = *src++;

        if ((result & 0x80) != 0) {
            if((result & 0xe0) == 0xc0) {
                result = (result & 0x1f) << 6;
                result |= (*src++ & 0x3f);
                length++;
            }
            else if ((result & 0xf0) == 0xe0) {
                result = (result & 0x0f) << 12;
                result |= (*src++ & 0x3f) << 6;
                result |= (*src++ & 0x3f);
                length++;
            }
            else if ((result & 0xf8) == 0xf0) {
                result = (result & 0x07) << 18;
                result |= (*src++ & 0x3f) << 12;
                result |= (*src++ & 0x3f) << 6;
                result |= (*src++ & 0x3f);
                length++;
            }
        }

        utf8Len = length;
        return result < 0xffff ? result : 0x25A1;
    }
}

namespace resource {
    class FontAtlasProviderImpl : public std::enable_shared_from_this<FontAtlasProviderImpl>, public FontAtlasProvider {
    public:
        FontAtlasProviderImpl(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering, std::unique_ptr<std::uint8_t[]> &&ttfData, std::uint32_t ttfLen);
        ~FontAtlasProviderImpl() override;
        
        float getTextWidth(const char *text, std::uint8_t size) const override;
        void getTextFontAtlas(const char *text, std::uint8_t size, util::callback<void(std::vector<FontCharInfo> &&)> &&completion) override;
        
    private:
        std::uint32_t _calculateAtlasSize(std::uint32_t atlasStart, std::uint32_t fontSize);
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        const std::shared_ptr<foundation::RenderingInterface> _rendering;
        
        const std::unique_ptr<std::uint8_t[]> _ttfData;
        const std::uint32_t _ttfLen;
        stbtt_fontinfo _ttfInfo;
        
        struct {
            std::uint32_t glyphIndex;
        }
        _missingCharacter;
        
        struct FontAtlas {
            struct CharInfo {
                math::vector2f txLT;
                math::vector2f txRB;
                math::vector2f pxSize;
                float advance, lsb, voffset;
            };
            
            std::unique_ptr<CharInfo[]> chars;
            foundation::RenderTexturePtr texture = nullptr;
        };
        struct QueueEntry {
            std::string text;
            std::uint8_t size;
            util::callback<void(std::vector<FontCharInfo> &&)> callback;
        };
        
        std::unordered_map<std::uint32_t, FontAtlas> _atlases;
        std::list<QueueEntry> _callsQueue;
        
        bool _initialized;
        bool _asyncInProgress;
    };
    
    FontAtlasProviderImpl::FontAtlasProviderImpl(
        const std::shared_ptr<foundation::PlatformInterface> &platform,
        const foundation::RenderingInterfacePtr &rendering,
        std::unique_ptr<std::uint8_t[]> &&ttfData,
        std::uint32_t ttfLen
    )
    : _platform(platform)
    , _rendering(rendering)
    , _ttfData(std::move(ttfData))
    , _ttfLen(ttfLen)
    , _initialized(false)
    , _asyncInProgress(false)
    {
        if (stbtt_InitFont(&_ttfInfo, _ttfData.get(), 0) != 0) {
            _missingCharacter.glyphIndex = stbtt_FindGlyphIndex(&_ttfInfo, 0xFFFD);
            
            if (_missingCharacter.glyphIndex != 0) {
                _initialized = true;
            }
            else {
                _platform->logError("[FontAtlasProviderImpl::FontAtlasProviderImpl] ttf doesn't contain 'missing symbol' glyph\n");
            }
        }
        else {
            _platform->logError("[FontAtlasProviderImpl::FontAtlasProviderImpl] ttf is not valid true type file\n");
        }
    }
    
    FontAtlasProviderImpl::~FontAtlasProviderImpl() {
    
    }
    
    std::uint32_t FontAtlasProviderImpl::_calculateAtlasSize(std::uint32_t atlasStart, std::uint32_t fontSize) {
        float fs = float(fontSize);
        float glyphScale = stbtt_ScaleForPixelHeight(&_ttfInfo, fs);
        float totalWidth = 0.0f;
        
        for (std::uint32_t chIndex = atlasStart; chIndex < atlasStart + 0xff; chIndex++) {
            int glyph = stbtt_FindGlyphIndex(&_ttfInfo, chIndex);
            if (glyph) {
                int ix0, ix1, iy0, iy1, iadvance, ilsb;
                stbtt_GetGlyphBitmapBox(&_ttfInfo, glyph, glyphScale, glyphScale, &ix0, &iy0, &ix1, &iy1);
                
                if (ix1 > ix0 && iy1 > iy0) {
                    totalWidth += float(ix1 - ix0 + 1);
                }
            }
        }
        
        std::uint32_t atlasSize = std::uint32_t(std::ceilf(std::sqrtf(totalWidth / fs))) * fontSize;
        return std::bit_ceil(atlasSize);
    }
    
    float FontAtlasProviderImpl::getTextWidth(const char *text, std::uint8_t size) const {
        float scale = stbtt_ScaleForPixelHeight(&_ttfInfo, float(size));
        float result = 0.0f;
        
        for (const char *src = text; *src; ) {
            std::uint32_t len = 0;
            std::uint32_t u16ch = utf8ToUTF16(src, len);
            
            int glyph = stbtt_FindGlyphIndex(&_ttfInfo, u16ch);
            if (glyph != 0) {
                int iadvance, ilsb;
                stbtt_GetGlyphHMetrics(&_ttfInfo, glyph, &iadvance, &ilsb);
                result += std::ceilf(scale * float(iadvance));
            }
            
            src += len;
        }
        
        return result;
    }
    
    void FontAtlasProviderImpl::getTextFontAtlas(const char *text, std::uint8_t size, util::callback<void(std::vector<FontCharInfo> &&)> &&completion) {
        if (_asyncInProgress) {
            _callsQueue.emplace_back(QueueEntry{
                .text = text,
                .size = size,
                .callback = std::move(completion)
            });
            
            return;
        }
        
        std::vector<std::uint32_t> fontsToLoad;
        std::vector<FontCharInfo> chars;
        
        for (const char *src = text; *src; ) {
            std::uint32_t len = 0;
            std::uint32_t u16ch = utf8ToUTF16(src, len);
            std::uint32_t chIndex = u16ch & 0xff;
            std::uint32_t atlasIndex = (u16ch & 0xffffff00) + size;
            
            auto index = _atlases.find(atlasIndex);
            if (index != _atlases.end()) {
                chars.emplace_back(FontCharInfo {
                    .texture = index->second.texture,
                    .txLT = index->second.chars[chIndex].txLT,
                    .txRB = index->second.chars[chIndex].txRB,
                    .pxSize = index->second.chars[chIndex].pxSize,
                    .advance = index->second.chars[chIndex].advance,
                    .lsb = index->second.chars[chIndex].lsb,
                    .voffset = index->second.chars[chIndex].voffset
                });
            }
            else if (std::find(fontsToLoad.begin(), fontsToLoad.end(), atlasIndex) == fontsToLoad.end()) {
                fontsToLoad.emplace_back(atlasIndex);
            }
            
            src += len;
        }
        
        if (fontsToLoad.empty()) {
            completion(std::move(chars));
        }
        else {
            _asyncInProgress = true;
            
            struct AsyncContext {
                struct Atlas {
                    std::uint32_t index;
                    std::uint32_t textureSize;
                    std::unique_ptr<std::uint8_t[]> textureData;
                    std::unique_ptr<FontAtlas::CharInfo[]> chars;
                    float baseline;
                };

                std::vector<Atlas> atlases;
            };
            
            _platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak = weak_from_this(), fontsToLoad = std::move(fontsToLoad)](AsyncContext &ctx) {
                if (std::shared_ptr<FontAtlasProviderImpl> self = weak.lock()) {
                    for (std::uint32_t i = 0; i < fontsToLoad.size(); i++) {
                        ctx.atlases.emplace_back(AsyncContext::Atlas{ fontsToLoad[i] });
                    }
                    
                    const stbtt_fontinfo *ttf = &self->_ttfInfo;
                    
                    for (AsyncContext::Atlas &atlas : ctx.atlases) {
                        atlas.textureSize = self->_calculateAtlasSize(atlas.index & 0xffffff00, atlas.index & 0xff);
                        atlas.textureData = std::make_unique<std::uint8_t[]>(atlas.textureSize * atlas.textureSize);
                        atlas.chars = std::make_unique<FontAtlas::CharInfo[]>(256);

                        const float scale = stbtt_ScaleForPixelHeight(ttf, float(atlas.index & 0xff));
                        
                        int ascent, descent, lineGap, offsetX = 1, offsetY = 0;
                        stbtt_GetFontVMetrics(ttf, &ascent, &descent, &lineGap);
                        atlas.baseline = std::roundf(float(ascent) * scale);

                        for (std::uint32_t i = 0; i < 0xff; i++) {
                            int glyph = stbtt_FindGlyphIndex(ttf, (atlas.index & 0xffffff00) + i);
                            if (glyph != 0) {
                                int ix0 = 0, ix1 = 0, iy0 = 0, iy1 = 0, iadvance, ilsb;

                                stbtt_GetGlyphBitmapBox(ttf, glyph, scale, scale, &ix0, &iy0, &ix1, &iy1);
                                stbtt_GetGlyphHMetrics(ttf, glyph, &iadvance, &ilsb);

                                if (offsetX + ix1 - ix0 + 1 >= atlas.textureSize) {
                                    offsetX = 1;
                                    offsetY += int(float(ascent - descent) * scale);
                                }

                                atlas.chars[i].advance = std::ceilf(scale * float(iadvance));
                                atlas.chars[i].lsb = std::floorf(scale * float(ilsb));
                                atlas.chars[i].pxSize = math::vector2f(ix1 - ix0, iy1 - iy0);
                                atlas.chars[i].txLT = math::vector2f(offsetX, offsetY + atlas.baseline + iy0) / float(atlas.textureSize);
                                atlas.chars[i].txRB = atlas.chars[i].txLT + atlas.chars[i].pxSize / float(atlas.textureSize);
                                atlas.chars[i].voffset = atlas.baseline + float(iy0);

                                if (ix1 > ix0 && iy1 > iy0) {
                                    int offset = (offsetY + int(std::ceilf(atlas.baseline)) + iy0) * atlas.textureSize + offsetX;
                                    stbtt_MakeGlyphBitmap(ttf, atlas.textureData.get() + offset, ix1 - ix0, iy1 - iy0, atlas.textureSize, scale, scale, glyph);
                                    offsetX += ix1 - ix0 + 1;
                                }
                            }
                        }
                    }
                    
                }
            },
            [weak = weak_from_this(), txt = std::string(text), size, completion = std::move(completion)](AsyncContext &ctx) mutable {
                if (std::shared_ptr<FontAtlasProviderImpl> self = weak.lock()) {
                    for (AsyncContext::Atlas &atlas : ctx.atlases) {
                        FontAtlas &emplaced = self->_atlases.emplace(atlas.index, FontAtlas{}).first->second;
                        emplaced.chars = std::move(atlas.chars);
                        emplaced.texture = self->_rendering->createTexture(foundation::RenderTextureFormat::R8UN, atlas.textureSize, atlas.textureSize, { atlas.textureData.get() });
                    }

                    self->_asyncInProgress = false;
                    self->getTextFontAtlas(txt.data(), size, std::move(completion));
                    
                    if (self->_callsQueue.size()) {
                        QueueEntry &entry = self->_callsQueue.front();
                        
                        self->getTextFontAtlas(entry.text.data(), entry.size, std::move(entry.callback));
                        self->_callsQueue.pop_front();
                    }
                }
            }));
        }
    }
    
    
}

namespace resource {
    std::shared_ptr<FontAtlasProvider> FontAtlasProvider::instance(
        const std::shared_ptr<foundation::PlatformInterface> &platform,
        const foundation::RenderingInterfacePtr &rendering,
        std::unique_ptr<std::uint8_t[]> &&ttfData,
        std::uint32_t ttfLen
    ) {
        return std::make_shared<FontAtlasProviderImpl>(platform, rendering, std::move(ttfData), ttfLen);
    }
}
