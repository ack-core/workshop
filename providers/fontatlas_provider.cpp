
#include "fontatlas_provider.h"
#include "thirdparty/stb_mini_ttf/stb_mini_ttf.h"

#include <list>
#include <unordered_map>
#include <mutex>
#include <bit>

namespace {
    static const int ATLAS_SIZE = 1024;
    static const int ATLAS_SPACE = 10;

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

    float ceilfloat(float x) {
        return (float)(int)x + (x > (float)(int)x);
    }
    float floorfloat(float x) {
        return (float)(int)x - (x < (float)(int)x);
    }
    float roundfloat(float x) {
        return (float)(int)(x + (x >= 0 ? 0.5f : -0.5f));
    }

}

namespace resource {
    class FontAtlasProviderImpl : public std::enable_shared_from_this<FontAtlasProviderImpl>, public FontAtlasProvider {
    public:
        FontAtlasProviderImpl(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering, std::unique_ptr<std::uint8_t[]> &&ttfData, std::size_t ttfLen);
        ~FontAtlasProviderImpl() override;
        
        float getTextWidth(const char *text, std::uint8_t fontSize) const override;
        void getTextFontAtlas(const char *text, std::uint8_t fontSize, std::uint8_t blur, util::callback<void(std::vector<FontCharInfo> &&)> &&completion) override;
        void update(float dtSec) override;
        
    private:
        std::uint32_t _calculateAtlasSize(std::uint32_t atlasStart, std::uint32_t fontSize);
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        const std::shared_ptr<foundation::RenderingInterface> _rendering;
        const std::unique_ptr<std::uint8_t[]> _ttfData;
        const std::size_t _ttfLen;
        
        stbtt_fontinfo _ttfInfo;
        
        struct FontAtlas {
            struct CharInfo {
                math::vector2f txLT;
                math::vector2f txRB;
                math::vector2f pxSize;
                float advance, lsb, voffset;
            };
            
            // --- used from worker thread ---
            std::unique_ptr<std::uint8_t[]> txdata;
            int offsetX = ATLAS_SPACE;
            int offsetY = ATLAS_SPACE;

            // --- used from main thread ---
            std::unordered_map<std::uint32_t, CharInfo> chars;
            foundation::RenderTexturePtr texture = nullptr;
            std::uint8_t fontSize = 0;
            std::uint8_t blur = 0;
            float baseLine = 0.0f;
        };
        struct QueueEntry {
            std::string text;
            std::uint8_t fontSize;
            std::uint8_t blur;
            util::callback<void(std::vector<FontCharInfo> &&)> callback;
        };
        
        std::list<FontAtlas> _atlases;
        std::list<QueueEntry> _callsQueue;
        bool _asyncInProgress;
    };
    
    FontAtlasProviderImpl::FontAtlasProviderImpl(
        const std::shared_ptr<foundation::PlatformInterface> &platform,
        const foundation::RenderingInterfacePtr &rendering,
        std::unique_ptr<std::uint8_t[]> &&ttfData,
        std::size_t ttfLen
    )
    : _platform(platform)
    , _rendering(rendering)
    , _ttfData(std::move(ttfData))
    , _ttfLen(ttfLen)
    , _asyncInProgress(false)
    {
        if (stbtt_InitFont(&_ttfInfo, _ttfData.get(), 0) == 0) {
            _platform->logError("[FontAtlasProviderImpl::FontAtlasProviderImpl] ttf is not valid true type file\n");
        }
    }
    
    FontAtlasProviderImpl::~FontAtlasProviderImpl() {
    
    }
    
    float FontAtlasProviderImpl::getTextWidth(const char *text, std::uint8_t size) const {
        const float scale = stbtt_ScaleForPixelHeight(&_ttfInfo, float(size));
        float result = 0.0f;
                
        for (const char *src = text; *src; ) {
            std::uint32_t len = 0;
            const std::uint32_t u16ch = utf8ToUTF16(src, len);
            
            const int glyph = stbtt_FindGlyphIndex(&_ttfInfo, u16ch);
            if (glyph != 0) {
                int iadvance, ilsb;
                stbtt_GetGlyphHMetrics(&_ttfInfo, glyph, &iadvance, &ilsb);
                result += ceilfloat(scale * float(iadvance));
            }
            
            src += len;
        }
        
        return result;
    }
    
    void FontAtlasProviderImpl::getTextFontAtlas(const char *text, std::uint8_t fontSize, std::uint8_t blur, util::callback<void(std::vector<FontCharInfo> &&)> &&completion) {
        if (_asyncInProgress) {
            _callsQueue.emplace_back(QueueEntry{
                .text = text,
                .fontSize = fontSize,
                .blur = blur,
                .callback = std::move(completion)
            });
            
            return;
        }
        
        std::vector<FontCharInfo> readyChars;
        std::vector<std::uint32_t> toLoad;
        FontAtlas *suitable = nullptr;

        for (const char *src = text; *src; ) {
            std::uint32_t len = 0;
            std::uint32_t u16ch = utf8ToUTF16(src, len);
            auto atlasIt = _atlases.begin();
            
            for (; atlasIt != _atlases.end(); ++atlasIt) {
                if (atlasIt->fontSize == fontSize && atlasIt->blur == blur) {
                    suitable = &(*atlasIt);
                    auto index = atlasIt->chars.find(u16ch);
                    if (index != atlasIt->chars.end()) {
                        readyChars.emplace_back(FontCharInfo {
                            .texture = atlasIt->texture,
                            .txLT = index->second.txLT,
                            .txRB = index->second.txRB,
                            .pxSize = index->second.pxSize,
                            .advance = index->second.advance,
                            .lsb = index->second.lsb,
                            .voffset = index->second.voffset
                        });
                        break;
                    }
                }
            }
            
            if (atlasIt == _atlases.end()) { // not found
                toLoad.emplace_back(u16ch);
            }
            
            src += len;
        }
        
        if (toLoad.empty()) {
            completion(std::move(readyChars));
        }
        else {
            _asyncInProgress = true;
            
            struct AsyncContext {
//                struct Atlas {
//                    std::uint32_t index;
//                    std::uint32_t textureSize;
//                    std::unique_ptr<std::uint8_t[]> textureData;
//                    std::unique_ptr<FontAtlas::CharInfo[]> chars;
//                    float baseline;
//                };
//
//                std::vector<Atlas> atlases;
                std::unordered_map<std::uint32_t, FontAtlas::CharInfo> resultChars;
            };
            
            if (suitable == nullptr) {
                const float scale = stbtt_ScaleForPixelHeight(&_ttfInfo, float(fontSize));
                int ascent, descent, lineGap;
                stbtt_GetFontVMetrics(&_ttfInfo, &ascent, &descent, &lineGap);
                
                FontAtlas *newAtlas = &_atlases.emplace_front();
                newAtlas->baseLine = std::roundf(float(ascent) * scale);
                newAtlas->fontSize = fontSize;
                newAtlas->blur = blur;
                newAtlas->txdata = std::make_unique<std::uint8_t[]>(ATLAS_SIZE * ATLAS_SIZE);
                suitable = newAtlas;
            }
            
            _platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak = weak_from_this(), toLoad = std::move(toLoad), suitable, fontSize, blur](AsyncContext &ctx) {
                if (std::shared_ptr<FontAtlasProviderImpl> self = weak.lock()) {
                    //--- worker thread ---
                    const stbtt_fontinfo *ttf = &self->_ttfInfo;
                    const float scale = stbtt_ScaleForPixelHeight(ttf, float(fontSize));
                    
                    int ascent, descent, lineGap;
                    stbtt_GetFontVMetrics(ttf, &ascent, &descent, &lineGap);
                    int lineHeight = int(float(ascent - descent) * scale);
                    
                    for (std::uint16_t u16ch : toLoad) {
                        int glyph = stbtt_FindGlyphIndex(ttf, u16ch);
                        if (glyph != 0) {
                            int ix0 = 0, ix1 = 0, iy0 = 0, iy1 = 0, iadvance, ilsb;

                            stbtt_GetGlyphBitmapBox(ttf, glyph, scale, scale, &ix0, &iy0, &ix1, &iy1);
                            stbtt_GetGlyphHMetrics(ttf, glyph, &iadvance, &ilsb);

                            if (suitable->offsetX + ix1 - ix0 + 1 >= ATLAS_SIZE - ATLAS_SPACE) {
                                suitable->offsetX = 10;
                                suitable->offsetY = suitable->offsetY + lineHeight;
                                
                                if (suitable->offsetY >= ATLAS_SIZE - ATLAS_SPACE) {
                                    self->_platform->logError("[FontAtlasProviderImpl::getTextFontAtlas] Atlas overflow");
                                    return;
                                }
                            }
                            
                            FontAtlas::CharInfo &chInfo = ctx.resultChars.emplace(u16ch, FontAtlas::CharInfo{}).first->second;
                            chInfo.advance = ceilfloat(scale * float(iadvance));
                            chInfo.lsb = floorfloat(scale * float(ilsb));
                            chInfo.pxSize = math::vector2f(ix1 - ix0, iy1 - iy0);
                            chInfo.txLT = math::vector2f(suitable->offsetX, suitable->offsetY + suitable->baseLine + iy0) / float(ATLAS_SIZE);
                            chInfo.txRB = chInfo.txLT + chInfo.pxSize / float(ATLAS_SIZE);
                            chInfo.voffset = suitable->baseLine + float(iy0);

                            if (ix1 > ix0 && iy1 > iy0) {
                                int offset = (suitable->offsetY + int(ceilfloat(suitable->baseLine)) + iy0) * ATLAS_SIZE + suitable->offsetX;
                                stbtt_MakeGlyphBitmap(ttf, suitable->txdata.get() + offset, ix1 - ix0, iy1 - iy0, ATLAS_SIZE, scale, scale, glyph);
                                suitable->offsetX += ix1 - ix0 + 1;
                            }
                        }
                        else {
                            self->_platform->logError("[FontAtlasProviderImpl::getTextFontAtlas] Char %d not found in TTF", int(u16ch));
                        }
                    }
                    
                    //--- worker thread ---
//                    for (std::uint32_t i = 0; i < fontsToLoad.size(); i++) {
//                        ctx.atlases.emplace_back(AsyncContext::Atlas{ fontsToLoad[i] });
//                    }
                    
//                    const stbtt_fontinfo *ttf = &self->_ttfInfo;
//
//                    for (AsyncContext::Atlas &atlas : ctx.atlases) {
//                        atlas.textureSize = self->_calculateAtlasSize(atlas.index & 0xffffff00, atlas.index & 0xff);
//                        atlas.textureData = std::make_unique<std::uint8_t[]>(atlas.textureSize * atlas.textureSize);
//                        atlas.chars = std::make_unique<FontAtlas::CharInfo[]>(256);
//
//                        const float scale = stbtt_ScaleForPixelHeight(ttf, float(atlas.index & 0xff));
//
//                        int ascent, descent, lineGap, offsetX = 1, offsetY = 0;
//                        stbtt_GetFontVMetrics(ttf, &ascent, &descent, &lineGap);
//                        atlas.baseline = std::roundf(float(ascent) * scale);
//
//                        for (std::uint32_t i = 0; i < 0xff; i++) {
//                            int glyph = stbtt_FindGlyphIndex(ttf, (atlas.index & 0xffffff00) + i);
//                            if (glyph != 0) {
//                                int ix0 = 0, ix1 = 0, iy0 = 0, iy1 = 0, iadvance, ilsb;
//
//                                stbtt_GetGlyphBitmapBox(ttf, glyph, scale, scale, &ix0, &iy0, &ix1, &iy1);
//                                stbtt_GetGlyphHMetrics(ttf, glyph, &iadvance, &ilsb);
//
//                                if (offsetX + ix1 - ix0 + 1 >= atlas.textureSize) {
//                                    offsetX = 1;
//                                    offsetY += int(float(ascent - descent) * scale);
//                                }
//
//                                atlas.chars[i].advance = std::ceilf(scale * float(iadvance));
//                                atlas.chars[i].lsb = std::floorf(scale * float(ilsb));
//                                atlas.chars[i].pxSize = math::vector2f(ix1 - ix0, iy1 - iy0);
//                                atlas.chars[i].txLT = math::vector2f(offsetX, offsetY + atlas.baseline + iy0) / float(atlas.textureSize);
//                                atlas.chars[i].txRB = atlas.chars[i].txLT + atlas.chars[i].pxSize / float(atlas.textureSize);
//                                atlas.chars[i].voffset = atlas.baseline + float(iy0);
//
//                                if (ix1 > ix0 && iy1 > iy0) {
//                                    int offset = (offsetY + int(std::ceilf(atlas.baseline)) + iy0) * atlas.textureSize + offsetX;
//                                    stbtt_MakeGlyphBitmap(ttf, atlas.textureData.get() + offset, ix1 - ix0, iy1 - iy0, atlas.textureSize, scale, scale, glyph);
//                                    offsetX += ix1 - ix0 + 1;
//                                }
//                            }
//                        }
//                    }
                    
                }
            },
            [weak = weak_from_this(), txt = std::string(text), fontSize, blur, suitable, completion = std::move(completion)](AsyncContext &ctx) mutable {
                if (std::shared_ptr<FontAtlasProviderImpl> self = weak.lock()) {
                    suitable->chars.merge(ctx.resultChars);
                    suitable->texture = self->_rendering->createTexture(foundation::RenderTextureFormat::R8UN, ATLAS_SIZE, ATLAS_SIZE, { suitable->txdata.get() });

                    self->_asyncInProgress = false;
                    self->getTextFontAtlas(txt.data(), fontSize, blur, std::move(completion));
                }
            }));
        }
    }
    
    void FontAtlasProviderImpl::update(float dtSec) {
        while (_asyncInProgress == false && _callsQueue.size()) {
            QueueEntry &entry = _callsQueue.front();
            getTextFontAtlas(entry.text.data(), entry.fontSize, entry.blur, std::move(entry.callback));
            _callsQueue.pop_front();
        }
    }
}

namespace resource {
    std::shared_ptr<FontAtlasProvider> FontAtlasProvider::instance(
        const std::shared_ptr<foundation::PlatformInterface> &platform,
        const foundation::RenderingInterfacePtr &rendering,
        std::unique_ptr<std::uint8_t[]> &&ttfData,
        std::size_t ttfLen
    ) {
        return std::make_shared<FontAtlasProviderImpl>(platform, rendering, std::move(ttfData), ttfLen);
    }
}
