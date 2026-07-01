
#include "fontatlas_provider.h"
#include "thirdparty/stb_mini_ttf/stb_mini_ttf.h"

#include <list>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <bit>

namespace {
    static const int ATLAS_SIZE = 512;
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

    std::uint64_t makeKey(std::uint32_t u16ch, std::uint8_t blur) {
        return (std::uint64_t(blur) << 56) | std::uint64_t(u16ch);
    }
    std::uint32_t getU16Ch(std::uint64_t key) {
        return std::uint32_t(key & 0xffffffff);
    }
    std::uint8_t getBlur(std::uint64_t key) {
        return std::uint8_t(key >> 56);
    }

}

namespace resource {
    class FontAtlasProviderImpl : public std::enable_shared_from_this<FontAtlasProviderImpl>, public FontAtlasProvider {
    public:
        struct FontAtlas {
            struct CharInfo {
                math::vector2f txLT;
                math::vector2f txRB;
                math::vector2f pxSize;
                float advance, lsb, voffset;
                std::uint8_t blur;
            };
            
            // --- used from worker thread ---
            std::unique_ptr<std::uint8_t[]> txdata;
            int offsetX = ATLAS_SPACE;
            int offsetY = ATLAS_SPACE;

            // --- used from main thread ---
            std::unordered_map<std::uint64_t, CharInfo> chars;
            foundation::RenderTexturePtr texture = nullptr;
            std::uint8_t fontSize = 0;
            float baseLine = 0.0f;
        };
        
    public:
        FontAtlasProviderImpl(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering, std::unique_ptr<std::uint8_t[]> &&ttfData, std::size_t ttfLen);
        ~FontAtlasProviderImpl() override;
        
        auto getTextWidth(const char *text, std::uint8_t fontSize) const -> math::vector2f override;
        void getTextFontAtlas(const char *text, std::uint8_t fontSize, std::uint8_t blur, util::callback<void(std::vector<FontCharInfo> &&, const foundation::RenderTexturePtr &)> &&completion) override;
        void update(float dtSec) override;
        
    private:
        FontAtlas *_collectChars(const char *text, std::uint8_t fontSize, std::uint8_t blur, std::vector<FontCharInfo> &readyChars, std::unordered_set<std::uint64_t> &toLoad);
        
    private:
        const std::shared_ptr<foundation::PlatformInterface> _platform;
        const std::shared_ptr<foundation::RenderingInterface> _rendering;
        const std::unique_ptr<std::uint8_t[]> _ttfData;
        const std::size_t _ttfLen;
        
        stbtt_fontinfo _ttfInfo;
        
        struct QueueEntry {
            std::string text;
            std::uint8_t fontSize;
            std::uint8_t blur;
            util::callback<void(std::vector<FontCharInfo> &&, const foundation::RenderTexturePtr &)> callback;
        };
        
        std::list<FontAtlas> _atlases;
        std::list<QueueEntry> _callsQueue;
        std::list<QueueEntry> _postponedQueue;
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
    
    math::vector2f FontAtlasProviderImpl::getTextWidth(const char *text, std::uint8_t size) const {
        const float scale = stbtt_ScaleForMappingEmToPixels(&_ttfInfo, float(size));
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
        
        return math::vector2f(result, float(size));
    }
    
    void FontAtlasProviderImpl::getTextFontAtlas(const char *text, std::uint8_t fontSize, std::uint8_t blur, util::callback<void(std::vector<FontCharInfo> &&, const foundation::RenderTexturePtr &)> &&completion) {
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
        std::unordered_set<std::uint64_t> toLoad;
        FontAtlas *suitable = _collectChars(text, fontSize, blur, readyChars, toLoad);
        
        if (toLoad.empty()) {
            completion(std::move(readyChars), suitable->texture);
        }
        else {
            _asyncInProgress = true;
            
            struct AsyncContext {
                std::unordered_map<std::uint64_t, FontAtlas::CharInfo> resultChars;
            };
            
            if (suitable == nullptr) {
                const float scale = stbtt_ScaleForMappingEmToPixels(&_ttfInfo, float(fontSize));
                int ascent, descent, lineGap;
                stbtt_GetFontVMetrics(&_ttfInfo, &ascent, &descent, &lineGap);
                
                FontAtlas *newAtlas = &_atlases.emplace_front();
                newAtlas->baseLine = std::roundf(float(ascent) * scale);
                newAtlas->fontSize = fontSize;
                newAtlas->txdata = std::make_unique<std::uint8_t[]>(ATLAS_SIZE * ATLAS_SIZE);
                suitable = newAtlas;
            }
            
            _platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak = weak_from_this(), toLoad = std::move(toLoad), suitable, fontSize, blur](AsyncContext &ctx) {
                if (std::shared_ptr<FontAtlasProviderImpl> self = weak.lock()) {
                    //--- worker thread ---
                    const stbtt_fontinfo *ttf = &self->_ttfInfo;
                    const float scale = stbtt_ScaleForMappingEmToPixels(ttf, float(fontSize));
                    
                    int ascent, descent, lineGap;
                    stbtt_GetFontVMetrics(ttf, &ascent, &descent, &lineGap);
                    int lineHeight = int(float(ascent - descent) * scale);
                    
                    for (std::uint16_t u16ch : toLoad) {
                        int glyph = stbtt_FindGlyphIndex(ttf, u16ch);
                        if (glyph != 0) {
                            int ix0 = 0, ix1 = 0, iy0 = 0, iy1 = 0, iadvance, ilsb;

                            stbtt_GetGlyphBitmapBox(ttf, glyph, scale, scale, &ix0, &iy0, &ix1, &iy1);
                            stbtt_GetGlyphHMetrics(ttf, glyph, &iadvance, &ilsb);
                            
                            ix0 -= blur; ix1 += blur;
                            iy0 -= blur; iy1 += blur;

                            if (suitable->offsetX + ix1 - ix0 + 1 >= ATLAS_SIZE - ATLAS_SPACE) {
                                suitable->offsetX = 10;
                                suitable->offsetY = suitable->offsetY + lineHeight;
                                
                                if (suitable->offsetY >= ATLAS_SIZE - ATLAS_SPACE) { // TODO: atlas growing 4096x32 -> 4096x64 ...
                                    self->_platform->logError("[FontAtlasProviderImpl::getTextFontAtlas] Atlas overflow");
                                    return;
                                }
                            }
                            
                            FontAtlas::CharInfo &chInfo = ctx.resultChars.emplace(makeKey(u16ch, blur), FontAtlas::CharInfo{}).first->second;
                            chInfo.advance = ceilfloat(scale * float(iadvance));
                            chInfo.lsb = floorfloat(scale * float(ilsb));
                            chInfo.pxSize = math::vector2f(ix1 - ix0, iy1 - iy0);
                            chInfo.txLT = math::vector2f(suitable->offsetX - blur, suitable->offsetY + suitable->baseLine + iy0) / float(ATLAS_SIZE);
                            chInfo.txRB = chInfo.txLT + chInfo.pxSize / float(ATLAS_SIZE);
                            chInfo.voffset = suitable->baseLine + float(iy0);
                            chInfo.blur = blur;

                            if (ix1 > ix0 && iy1 > iy0) {
                                int startY = suitable->offsetY + int(ceilfloat(suitable->baseLine)) + iy0;
                                int startX = suitable->offsetX - blur;
                                int offset = (startY + blur) * ATLAS_SIZE + startX;
                                stbtt_MakeGlyphBitmap(ttf, suitable->txdata.get() + offset, ix1 - ix0, iy1 - iy0, ATLAS_SIZE, scale, scale, glyph);
                                
                                for (int b = 0; b < blur; b++) {
                                    for (int bY = startY; bY < startY + iy1 - iy0; bY++) {
                                        for (int bX = startX; bX < startX + ix1 - ix0; bX++) {
                                            std::uint8_t &bpx = *(suitable->txdata.get() + bY * ATLAS_SIZE + bX);
                                            int sum = 0;
                                            sum += *(suitable->txdata.get() + (bY - 1) * ATLAS_SIZE + (bX - 1));
                                            sum += *(suitable->txdata.get() + (bY - 1) * ATLAS_SIZE + (bX + 1));
                                            sum += *(suitable->txdata.get() + (bY + 1) * ATLAS_SIZE + (bX - 1));
                                            sum += *(suitable->txdata.get() + (bY + 1) * ATLAS_SIZE + (bX + 1));
                                            sum += *(suitable->txdata.get() + (bY - 1) * ATLAS_SIZE + (bX + 0));
                                            sum += *(suitable->txdata.get() + (bY + 1) * ATLAS_SIZE + (bX + 0));
                                            sum += *(suitable->txdata.get() + (bY + 0) * ATLAS_SIZE + (bX - 1));
                                            sum += *(suitable->txdata.get() + (bY + 0) * ATLAS_SIZE + (bX + 1));
                                            bpx = std::max(bpx, std::uint8_t(sum >> 3));
                                        }
                                    }
                                }
                                
                                suitable->offsetX += ix1 - ix0 + 1;
                            }
                        }
                        else {
                            self->_platform->logError("[FontAtlasProviderImpl::getTextFontAtlas] Char %d not found in TTF", int(u16ch));
                        }
                    }
                    //--- worker thread ---
                }
            },
            [weak = weak_from_this(), txt = std::string(text), fontSize, blur, suitable, completion = std::move(completion)](AsyncContext &ctx) mutable {
                if (std::shared_ptr<FontAtlasProviderImpl> self = weak.lock()) {
                    self->_asyncInProgress = false;
                    suitable->chars.merge(ctx.resultChars);

                    for (const auto &item : self->_callsQueue) {
                        if (item.fontSize == fontSize) {
                            self->_postponedQueue.emplace_back(QueueEntry{
                                .text = txt,
                                .fontSize = fontSize,
                                .blur = blur,
                                .callback = std::move(completion)
                            });
                            return;
                        }
                    }
                    
                    suitable->texture = self->_rendering->createTexture(foundation::RenderTextureFormat::R8UN, ATLAS_SIZE, ATLAS_SIZE, { suitable->txdata.get() });
                    self->getTextFontAtlas(txt.data(), fontSize, blur, std::move(completion));
                }
            }));
        }
    }

    FontAtlasProviderImpl::FontAtlas *FontAtlasProviderImpl::_collectChars(const char *text, std::uint8_t fontSize, std::uint8_t blur, std::vector<FontCharInfo> &readyChars, std::unordered_set<std::uint64_t> &toLoad) {
        FontAtlas *suitable = nullptr;

        for (auto atlasIt = _atlases.begin(); atlasIt != _atlases.end(); ++atlasIt) {
            if (atlasIt->fontSize == fontSize) {
                suitable = &(*atlasIt);
                break;
            }
        }
        
        for (const char *src = text; *src; ) {
            std::uint32_t len = 0;
            std::uint32_t u16ch = utf8ToUTF16(src, len);

            if (suitable) {
                auto index = suitable->chars.find(makeKey(u16ch, blur));
                if (index != suitable->chars.end()) {
                    readyChars.emplace_back(FontCharInfo {
                        .txLT = index->second.txLT,
                        .txRB = index->second.txRB,
                        .pxSize = index->second.pxSize,
                        .advance = index->second.advance,
                        .lsb = index->second.lsb,
                        .voffset = index->second.voffset
                    });
                }
                else {
                    toLoad.emplace(makeKey(u16ch, blur));
                }
            }
            else {
                toLoad.emplace(makeKey(u16ch, blur));
            }

            src += len;
        }
        
        return suitable;
    }
    
    void FontAtlasProviderImpl::update(float dtSec) {
        while (_asyncInProgress == false && _callsQueue.size()) {
            QueueEntry &entry = _callsQueue.front();
            getTextFontAtlas(entry.text.data(), entry.fontSize, entry.blur, std::move(entry.callback));
            _callsQueue.pop_front();
        }
        if (_asyncInProgress == false && _callsQueue.empty()) {
            while (_postponedQueue.size()) {
                QueueEntry &entry = _postponedQueue.front();
                
                std::vector<FontCharInfo> readyChars;
                std::unordered_set<std::uint64_t> toLoad;
                FontAtlas *suitable = _collectChars(entry.text.data(), entry.fontSize, entry.blur, readyChars, toLoad);
                
                if (suitable && toLoad.empty()) {
                    entry.callback(std::move(readyChars), suitable->texture);
                }
                else {
                    _platform->logError("[FontAtlasProviderImpl::update] Atlas logic error");
                }
                
                _postponedQueue.pop_front();
            }
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
