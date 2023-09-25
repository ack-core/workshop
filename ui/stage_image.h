
#pragma once
#include <functional>

namespace ui {
    class ImageImpl : public InteractorImpl, public std::enable_shared_from_this<ImageImpl>, public StageInterface::Image {
    public:
        ImageImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent, const math::vector2f &size, bool capturePointer);
        ~ImageImpl() override;
        
        void setColor(const math::color &rgba) override;

    public:
        void setBaseTexture(const char *texturePath);
        void setActionTexture(const char *texturePath);
        
        void draw(ShaderConstants &constStorage) override;
        
    private:
        math::color _rgba = math::color(1.0f, 1.0f, 1.0f, 1.0f);
        foundation::RenderTexturePtr _baseTexture;
        foundation::RenderTexturePtr _actionTexture;
    };
}

