
#include "stage.h"
#include "stage_base.h"
#include "stage_text.h"

namespace ui {
    TextImpl::TextImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent, std::uint8_t fontSize) : ElementImpl(facility, parent), _fontSize(fontSize) {
        _shadowEnabled = false;
        _size.y = fontSize;
    }
    TextImpl::~TextImpl() {
    
    }
    
    void TextImpl::setText(const char *text) {
        _size.x = _facility.getFontAtlasProvider()->getTextWidth(text, _fontSize);
        _facility.getFontAtlasProvider()->getTextFontAtlas(text, _fontSize, [weak = weak_from_this()](std::vector<resource::FontCharInfo> &&chars) {
            if (std::shared_ptr<TextImpl> self = weak.lock()) {
                self->_chars = std::move(chars);
            }
        });
    }
    
    void TextImpl::setColor(const math::color &rgba) {
        _rgba = rgba;
    }
    
    void TextImpl::setShadow(bool shadowEnabled, const math::vector2f &shadowOffset) {
        _shadowEnabled = shadowEnabled;
        _shadowOffset = shadowOffset;
    }
    
    void TextImpl::draw(ShaderConstants &constStorage) {
        const foundation::RenderingInterfacePtr &rendering = _facility.getRendering();
        float offsetX = 0.0f;
        
        if (_shadowEnabled) {
            for (auto &ch : _chars) {
                if (ch.pxSize.x > std::numeric_limits<float>::epsilon()) {
                    constStorage.positionAndSize.x = _globalPosition.x + offsetX + ch.lsb + _shadowOffset.x;
                    constStorage.positionAndSize.y = _globalPosition.y + ch.voffset + _shadowOffset.y;
                    constStorage.positionAndSize.zw = ch.pxSize;
                    constStorage.textureCoords = math::vector4f(ch.txLT, ch.txRB);
                    constStorage.color = math::vector4f(0.0f, 0.0f, 0.0f, 1.0f);
                    constStorage.flags = math::vector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    rendering->applyTextures(&ch.texture, 1);
                    rendering->applyShaderConstants(&constStorage);
                    rendering->drawGeometry(nullptr, 4, foundation::RenderTopology::TRIANGLESTRIP);
                }

                offsetX += ch.advance;
            }
            
            offsetX = 0.0f;
        }
        
        for (auto &ch : _chars) {
            if (ch.pxSize.x > std::numeric_limits<float>::epsilon()) {
                constStorage.positionAndSize.x = _globalPosition.x + offsetX + ch.lsb;
                constStorage.positionAndSize.y = _globalPosition.y + ch.voffset;
                constStorage.positionAndSize.zw = ch.pxSize;
                constStorage.textureCoords = math::vector4f(ch.txLT, ch.txRB);
                constStorage.color = math::vector4f(_rgba.r, _rgba.g, _rgba.b, _rgba.a);
                constStorage.flags = math::vector4f(1.0f, 0.0f, 0.0f, 0.0f);
                rendering->applyTextures(&ch.texture, 1);
                rendering->applyShaderConstants(&constStorage);
                rendering->drawGeometry(nullptr, 4, foundation::RenderTopology::TRIANGLESTRIP);
            }

            offsetX += ch.advance;
        }


        for (auto &item : _attachedElements) {
            item->draw(constStorage);
        }
    }
}
