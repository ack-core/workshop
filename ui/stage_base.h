
#pragma once
#include "foundation/math.h"

#include <memory>
#include <list>

namespace ui {
    struct ShaderConstants {
        math::vector4f positionAndSize;
        math::vector4f textureCoords;
        math::vector4f color;
        math::vector4f flags; // [is R component only, 0, 0, 0]
    };

    class StageFacility {
    public:
        virtual const foundation::PlatformInterfacePtr &getPlatform() const = 0;
        virtual const foundation::RenderingInterfacePtr &getRendering() const = 0;
        virtual const resource::TextureProviderPtr &getTextureProvider() const = 0;
        virtual const resource::FontAtlasProviderPtr &getFontAtlasProvider() const = 0;
        virtual ~StageFacility() = default;
    };
    
    class ElementImpl : public virtual StageInterface::Element {
    public:
        ElementImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent);
        ~ElementImpl() override;
        
        void setAnchor(const std::shared_ptr<Element> &target, HorizontalAnchor h, VerticalAnchor v, float hOffset, float vOffset);
        void attachElement(const std::shared_ptr<ElementImpl> &element);
        
        const math::vector2f &getPosition() const override;
        const math::vector2f &getSize() const override;
        
        virtual void updateCoordinates();
        virtual bool onInteraction(ui::Action action, std::size_t id, float x, float y);
        virtual void draw(ShaderConstants &constStorage);
        
    protected:
        const StageFacility &_facility;
        const std::weak_ptr<ElementImpl> _parent;
        
        math::vector2f _size;
        math::vector2f _globalPosition;
        math::vector2f _anchorOffsets;

        std::list<std::shared_ptr<ElementImpl>> _attachedElements;

    private:
        std::weak_ptr<ElementImpl> _anchorTarget;
        HorizontalAnchor _hAnchor;
        VerticalAnchor _vAnchor;
    };
    
    class InteractorImpl : public ElementImpl, public virtual StageInterface::Interactor {
    public:
        InteractorImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent, bool capturePointer);
        ~InteractorImpl() override;
        
        void setActiveArea(float offset, float radius);
        void setActionHandler(ui::Action action, util::callback<void(float x, float y)> &&handler) override;
        bool onInteraction(ui::Action action, std::size_t id, float x, float y) override;
        
    protected:
        bool _capturePointer;
        std::size_t _pointerId;
        ui::Action _currentAction;
        
    private:
        float _activeAreaOffset;
        float _activeAreaRadius;
        
        util::callback<void(float x, float y)> _handlers[int(ui::Action::RELEASE) + 1];
    };
}

