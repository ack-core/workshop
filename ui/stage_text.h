
#pragma once

namespace ui {
    class TextImpl : public ElementImpl, public std::enable_shared_from_this<TextImpl>, public StageInterface::Text {
    public:
        TextImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent, std::uint8_t fontSize);
        ~TextImpl() override;
        
    public:
        void setText(const char *text) override;
        void setColor(const math::color &rgba) override;
        void setShadow(bool shadowEnabled, const math::vector2f &shadowOffset);
        void draw(ShaderConstants &constStorage) override;
        
    private:
        const std::uint8_t _fontSize;
        
        bool _shadowEnabled;
        math::vector2f _shadowOffset;
        
        std::vector<resource::FontCharInfo> _chars;
        math::color _rgba;
    };
}

