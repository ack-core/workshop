
#pragma once
#include <functional>

namespace ui {
    class ImageImpl : public InteractorImpl, public std::enable_shared_from_this<ImageImpl>, public virtual StageInterface::Image {
    public:
        ImageImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent, const math::vector2f &size, bool capturePointer);
        ~ImageImpl() override;
        
    public:
        void setBaseTexture(const char *texturePath);
        void setActionTexture(const char *texturePath);
        
        void draw(ShaderConstants &constStorage) override;
        
    private:
        foundation::RenderTexturePtr _baseTexture;
        foundation::RenderTexturePtr _actionTexture;
    };
}

