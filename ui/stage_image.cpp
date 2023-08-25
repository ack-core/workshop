
#include "stage.h"
#include "stage_base.h"
#include "stage_image.h"

namespace ui {
    ImageImpl::ImageImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent, const math::vector2f &size, bool capturePointer) : InteractorImpl(facility, parent, capturePointer) {
        _size = size;
    }
    ImageImpl::~ImageImpl() {
    
    }
    
    void ImageImpl::setBaseTexture(const char *texturePath) {
        std::string path = texturePath;
        _facility.getTextureProvider()->getOrLoadTexture(texturePath, [weak = weak_from_this(), path](const std::unique_ptr<std::uint8_t[]> &data, std::uint32_t w, std::uint32_t h) {
            if (std::shared_ptr<ImageImpl> self = weak.lock()) {
                if (w == int(self->_size.x) && h == int(self->_size.y)) {
                    self->_baseTexture = self->_facility.getRendering()->createTexture(foundation::RenderTextureFormat::RGBA8UN, w, h, {data.get()});
                }
                else {
                    self->_facility.getPlatform()->logError("[ImageImpl::setActionTexture] Texture '%s' doesn't fit image size\n", path.data());
                }
            }
        });
    }
    
    void ImageImpl::setActionTexture(const char *texturePath) {
        std::string path = texturePath;
        _facility.getTextureProvider()->getOrLoadTexture(texturePath, [weak = weak_from_this(), path](const std::unique_ptr<std::uint8_t[]> &data, std::uint32_t w, std::uint32_t h) {
            if (std::shared_ptr<ImageImpl> self = weak.lock()) {
                if (w == int(self->_size.x) && h == int(self->_size.y)) {
                    self->_actionTexture = self->_facility.getRendering()->createTexture(foundation::RenderTextureFormat::RGBA8UN, w, h, {data.get()});
                }
                else {
                    self->_facility.getPlatform()->logError("[ImageImpl::setActionTexture] Texture '%s' doesn't fit image size\n", path.data());
                }
            }
        });
    }
    
    void ImageImpl::draw(ShaderConstants &constStorage) {
        const foundation::RenderingInterfacePtr &rendering = _facility.getRendering();
        
        constStorage.positionAndSize = math::vector4f(_globalPosition, _size);
        constStorage.textureCoords = math::vector4f(0, 0, 1, 1);
        constStorage.color = math::vector4f(1.0f, 1.0f, 1.0f, 1.0f);
        constStorage.flags = math::vector4f(0.0f, 0.0f, 0.0f, 0.0f);
        
        if (_currentAction != ui::Action::RELEASE && _actionTexture) {
            rendering->applyTextures(&_actionTexture, 1);
        }
        else {
            rendering->applyTextures(&_baseTexture, 1);
        }
        
        rendering->applyShaderConstants(&constStorage);
        rendering->drawGeometry(nullptr, 4, foundation::RenderTopology::TRIANGLESTRIP);
        
        for (auto &item : _attachedElements) {
            item->draw(constStorage);
        }
    }
}
