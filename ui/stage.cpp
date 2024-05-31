
#include "stage.h"

#include <list>
#include <memory>

namespace ui {
    struct ShaderConstants {
        math::vector4f positionAndSize;
        math::vector4f textureCoords;
        math::vector4f flags; // [is R component only, 0, 0, 0]
    };

    class StageFacility {
    public:
        virtual const foundation::PlatformInterfacePtr &getPlatform() const = 0;
        virtual const foundation::RenderingInterfacePtr &getRendering() const = 0;
        virtual const resource::ResourceProviderPtr &getResourceProvider() const = 0;
        virtual ~StageFacility() = default;
    };
}

//---

namespace ui {
    // TODO: make it working without rtti
    class ElementImpl : public virtual StageInterface::Element {
    public:
        ElementImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent) : _facility(facility), _parent(std::dynamic_pointer_cast<ElementImpl>(parent)) {}
        ~ElementImpl() override {}
        
        void setAnchor(const std::shared_ptr<Element> &target, HorizontalAnchor h, VerticalAnchor v, float hOffset, float vOffset) {
            if (target) {
                std::shared_ptr<ElementImpl> targetImpl = std::dynamic_pointer_cast<ElementImpl>(target);
                if (targetImpl->_parent.lock() == _parent.lock()) {
                    if (targetImpl->_anchorTarget.lock().get() != this) {
                        _anchorTarget = targetImpl;
                    }
                    else {
                        _facility.getPlatform()->logError("[ElementImpl::setAnchor] Cyclic anchor is not allowed\n");
                    }
                }
                else {
                    _facility.getPlatform()->logError("[ElementImpl::setAnchor] Target must be in same hierarchy level\n");
                }
            }
            
            _anchorOffsets = math::vector2f(hOffset, vOffset);
            _hAnchor = h;
            _vAnchor = v;
        }
        void attachElement(const std::shared_ptr<ElementImpl> &element) {
            _attachedElements.emplace_back(element);
        }
        const math::vector2f &getPosition() const override {
            return _globalPosition;
        }
        const math::vector2f &getSize() const override {
            return _size;
        }
        virtual void updateCoordinates() {
            math::vector2f lt = math::vector2f(0.0f, 0.0f);
            math::vector2f rb = math::vector2f(_facility.getPlatform()->getScreenWidth(), _facility.getPlatform()->getScreenHeight());
            
            if (auto target = _anchorTarget.lock()) {
                lt = target->_globalPosition;
                rb = target->_globalPosition + target->_size;
            }
            else if (auto parent = _parent.lock()) {
                lt = parent->_globalPosition;
                rb = parent->_globalPosition + parent->_size;
            }
            
            if (_hAnchor == HorizontalAnchor::LEFTSIDE) {
                _globalPosition.x = lt.x - _anchorOffsets.x - _size.x;
            }
            else if (_hAnchor == HorizontalAnchor::LEFT) {
                _globalPosition.x = lt.x + _anchorOffsets.x;
            }
            else if (_hAnchor == HorizontalAnchor::CENTER) {
                _globalPosition.x = lt.x + std::floor(0.5f * (rb.x - lt.x - _size.x)) + _anchorOffsets.x;
            }
            else if (_hAnchor == HorizontalAnchor::RIGHT) {
                _globalPosition.x = rb.x - _size.x - _anchorOffsets.x;
            }
            else if (_hAnchor == HorizontalAnchor::RIGHTSIDE) {
                _globalPosition.x = rb.x + _anchorOffsets.x;
            }
            
            if (_vAnchor == VerticalAnchor::TOPSIDE) {
                _globalPosition.y = lt.y - _anchorOffsets.y - _size.y;
            }
            else if (_vAnchor == VerticalAnchor::TOP) {
                _globalPosition.y = lt.y + _anchorOffsets.y;
            }
            else if (_vAnchor == VerticalAnchor::MIDDLE) {
                _globalPosition.y = lt.y + std::floor(0.5f * (rb.y - lt.y - _size.y)) + _anchorOffsets.y;
            }
            else if (_vAnchor == VerticalAnchor::BOTTOM) {
                _globalPosition.y = rb.y - _size.y - _anchorOffsets.y;
            }
            else if (_vAnchor == VerticalAnchor::BOTTOMSIDE) {
                _globalPosition.y = rb.y + _anchorOffsets.y;
            }
            
            for (auto &item : _attachedElements) {
                item->updateCoordinates();
            }
        }
        virtual bool onInteraction(ui::Action action, std::size_t id, float x, float y) {
            for (auto &item : _attachedElements) {
                if (item->onInteraction(action, id, x, y)) {
                    return true;
                }
            }
            
            return false;
        }
        
    protected:
        const StageFacility &_facility;
        const std::weak_ptr<ElementImpl> _parent;
        
        math::vector2f _size = math::vector2f(0, 0);
        math::vector2f _globalPosition = math::vector2f(0, 0);
        math::vector2f _anchorOffsets = math::vector2f(0, 0);

        std::list<std::shared_ptr<ElementImpl>> _attachedElements;

    private:
        std::weak_ptr<ElementImpl> _anchorTarget;
        HorizontalAnchor _hAnchor = HorizontalAnchor::LEFT;
        VerticalAnchor _vAnchor = VerticalAnchor::TOP;
    };
}

//---

namespace ui {
    class InteractorImpl : public ElementImpl, public virtual StageInterface::Interactor {
    public:
        InteractorImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent, bool capturePointer) : ElementImpl(facility, parent), _capturePointer(capturePointer) {}
        ~InteractorImpl() override {}
        
        void setActiveArea(float offset, float radius) {
            _activeAreaOffset = offset;
            _activeAreaRadius = radius;
        }
        void setActionHandler(ui::Action action, util::callback<void(float x, float y)> &&handler) override {
            _handlers[int(action)] = std::move(handler);
        }
        bool onInteraction(ui::Action action, std::size_t id, float x, float y) override {
            if (ElementImpl::onInteraction(action, id, x, y)) {
                return true;
            }
            
            bool isInArea = false;
            const math::vector2f lt = math::vector2f(_globalPosition.x + _activeAreaOffset, _globalPosition.y + _activeAreaOffset);
            const math::vector2f rb = math::vector2f(_globalPosition.x + _size.x - _activeAreaOffset, _globalPosition.y + _size.y - _activeAreaOffset);
            
            if (x < lt.x) {
                if (y < lt.y) {
                    isInArea = math::vector2f(x, y).distanceTo(lt) < _activeAreaRadius;
                }
                else if (y > rb.y) {
                    isInArea = math::vector2f(x, y).distanceTo(math::vector2f(lt.x, rb.y)) < _activeAreaRadius;
                }
                else {
                    isInArea = std::fabs(x - lt.x) < _activeAreaRadius;
                }
            }
            else if (x > rb.x) {
                if (y < lt.y) {
                    isInArea = math::vector2f(x, y).distanceTo(math::vector2f(rb.x, lt.y)) < _activeAreaRadius;
                }
                else if (y > rb.y) {
                    isInArea = math::vector2f(x, y).distanceTo(rb) < _activeAreaRadius;
                }
                else {
                    isInArea = std::fabs(x - rb.x) < _activeAreaRadius;
                }
            }
            else {
                if (y < lt.y) {
                    isInArea = std::fabs(y - lt.y) < _activeAreaRadius;
                }
                else if (y > rb.y) {
                    isInArea = std::fabs(y - rb.y) < _activeAreaRadius;
                }
                else {
                    isInArea = true;
                }
            }

            if (isInArea && action == ui::Action::CAPTURE) {
                if (_handlers[int(action)]) {
                    _pointerId = id;
                    _currentAction = action;
                    _handlers[int(action)](x, y);
                    return true;
                }
                if (_capturePointer) {
                    _currentAction = action;
                    _pointerId = id;
                    return true;
                }
            }
            if (action == ui::Action::MOVE && _pointerId != foundation::INVALID_POINTER_ID) {
                _currentAction = action;
                
                if (_handlers[int(action)]) {
                    _handlers[int(action)](x, y);
                }

                return true;
            }
            if (action == ui::Action::RELEASE) {
                if (_handlers[int(action)] && _pointerId != foundation::INVALID_POINTER_ID) {
                    _handlers[int(action)](x, y);
                }

                _pointerId = foundation::INVALID_POINTER_ID;
                _currentAction = action;
            }
            
            return false;
        }
        
    protected:
        bool _capturePointer;
        std::size_t _pointerId = foundation::INVALID_POINTER_ID;
        ui::Action _currentAction = ui::Action::RELEASE;
        
    private:
        float _activeAreaOffset = 0.0f;
        float _activeAreaRadius = 0.0f;
        
        util::callback<void(float x, float y)> _handlers[int(ui::Action::RELEASE) + 1];
    };
}

//---

namespace ui {
    class PivotImpl : public ElementImpl, public StageInterface::Pivot {
    public:
        PivotImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent) : ElementImpl(facility, parent) {}
        ~PivotImpl() override {}
        
    public:
        void updateCoordinates() override {
            if (_state == CoordinateState::ANCHOR) {
                ElementImpl::updateCoordinates();
            }
            else {
                if (_state == CoordinateState::SCREEN) {
                    _globalPosition = _screenPosition;
                }
                else if (_state == CoordinateState::WORLD) {
                    math::vector4f tpos = math::vector4f(_worldPosition, 1.0f);
                    
                    tpos = tpos.transformed(_facility.getRendering()->getStdVPMatrix());
                    tpos.x /= tpos.w;
                    tpos.y /= tpos.w;
                    tpos.z /= tpos.w;
                    
                    if (tpos.z > 0.0f) {
                        _globalPosition.x = (tpos.x + 1.0f) * 0.5f * _facility.getPlatform()->getScreenWidth();
                        _globalPosition.y = (1.0f - tpos.y) * 0.5f * _facility.getPlatform()->getScreenHeight();
                    }
                }
                
                for (auto &item : _attachedElements) {
                    item->updateCoordinates();
                }
            }
        }
        void setScreenPosition(const math::vector2f &position) override {
            _state = CoordinateState::SCREEN;
            _screenPosition = position;
        }
        void setWorldPosition(const math::vector3f &position) override {
            _state = CoordinateState::WORLD;
            _worldPosition = position;
        }
        bool onInteraction(ui::Action action, std::size_t id, float x, float y) override {
            return ElementImpl::onInteraction(action, id, x, y);
        }
        
    private:
        enum class CoordinateState {
            ANCHOR = 0,
            SCREEN,
            WORLD
        }
        _state = CoordinateState::ANCHOR;
        
        math::vector2f _screenPosition = {0, 0};
        math::vector3f _worldPosition = {0, 0, 0};
    };
}

//---

namespace ui {
    class ImageImpl : public InteractorImpl, public std::enable_shared_from_this<ImageImpl>, public StageInterface::Image {
    public:
        ImageImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent, const math::vector2f &size, bool capturePointer) : InteractorImpl(facility, parent, capturePointer) {
            _size = size;
        }
        ~ImageImpl() override {}
        
    public:
        void setBaseTexture(const char *texturePath) {
            std::string path = texturePath;
            _facility.getResourceProvider()->getOrLoadTexture(texturePath, [weak = weak_from_this(), path](const foundation::RenderTexturePtr &texture) {
                if (std::shared_ptr<ImageImpl> self = weak.lock()) {
                    if (texture->getWidth() == int(self->_size.x) && texture->getHeight() == int(self->_size.y)) {
                        self->_baseTexture = texture;
                    }
                    else {
                        self->_facility.getPlatform()->logError("[ImageImpl::setActionTexture] Texture '%s' doesn't fit image size\n", path.data());
                    }
                }
            });
        }
        void setActionTexture(const char *texturePath) {
            std::string path = texturePath;
            _facility.getResourceProvider()->getOrLoadTexture(texturePath, [weak = weak_from_this(), path](const foundation::RenderTexturePtr &texture) {
                if (std::shared_ptr<ImageImpl> self = weak.lock()) {
                    if (texture->getWidth() == int(self->_size.x) && texture->getHeight() == int(self->_size.y)) {
                        self->_actionTexture = texture;
                    }
                    else {
                        self->_facility.getPlatform()->logError("[ImageImpl::setActionTexture] Texture '%s' doesn't fit image size\n", path.data());
                    }
                }
            });
        }
        
    private:
        foundation::RenderTexturePtr _baseTexture;
        foundation::RenderTexturePtr _actionTexture;
    };
}

//---

namespace ui {
    class StageInterfaceImpl : public StageInterface, public StageFacility {
    public:
        StageInterfaceImpl(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            const resource::ResourceProviderPtr &resourceProvider
        );
        ~StageInterfaceImpl() override;
        
    public:
        const foundation::PlatformInterfacePtr &getPlatform() const override { return _platform; }
        const foundation::RenderingInterfacePtr &getRendering() const override { return _rendering; }
        const resource::ResourceProviderPtr &getResourceProvider() const override { return _resourceProvider; }
        
    public:
        auto addPivot(const std::shared_ptr<Element> &parent, PivotParams &&params) -> std::shared_ptr<Pivot> override;
        auto addImage(const std::shared_ptr<Element> &parent, ImageParams &&params) -> std::shared_ptr<Image> override;

        void clear() override;
        void updateAndDraw(float dtSec) override;
        
    private:
        const foundation::PlatformInterfacePtr _platform;
        const foundation::RenderingInterfacePtr _rendering;
        const resource::ResourceProviderPtr _resourceProvider;
        
        foundation::EventHandlerToken _touchEventsToken;
        ShaderConstants _shaderConstants;
        foundation::RenderShaderPtr _uiShader;
        std::list<std::shared_ptr<ElementImpl>> _topLevelElements;
    };
    
    std::shared_ptr<StageInterface> StageInterface::instance(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const resource::ResourceProviderPtr &resourceProvider
    ) {
        return std::make_shared<StageInterfaceImpl>(platform, rendering, resourceProvider);
    }
}

namespace {
    const char *g_uiShaderSrc = R"(
        const {
            position_size : float4
            texcoord : float4
            flags : float4
        }
        inout {
            texcoord : float2
        }
        vssrc {
            float2 screenTransform = float2(2.0, -2.0) / frame_rtBounds.xy;
            float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
            output_texcoord = _lerp(const_texcoord.xy, const_texcoord.zw, vertexCoord);
            vertexCoord = vertexCoord * const_position_size.zw + const_position_size.xy;
            output_position = float4(vertexCoord.xy * screenTransform + float2(-1.0, 1.0), 0.1, 1); //
        }
        fssrc {
            float4 texcolor = _tex2d(0, input_texcoord);
            output_color[0] = _lerp(texcolor, float4(1.0, 1.0, 1.0, texcolor.r), const_flags.r);
        }
    )";
}

namespace ui {
    StageInterfaceImpl::StageInterfaceImpl(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const resource::ResourceProviderPtr &resourceProvider
    )
    : _platform(platform)
    , _rendering(rendering)
    , _resourceProvider(resourceProvider)
    , _touchEventsToken(nullptr)
    {
        _shaderConstants = ShaderConstants {};
//        _uiShader = _rendering->createShader("stage_element", g_uiShaderSrc, foundation::InputLayout {
//            .repeat = 4
//        });
        
        _touchEventsToken = _platform->addPointerEventHandler([this](const foundation::PlatformPointerEventArgs &args) {
            ui::Action action = ui::Action::RELEASE;
            
            if (args.type == foundation::PlatformPointerEventArgs::EventType::START) {
                action = ui::Action::CAPTURE;
            }
            if (args.type == foundation::PlatformPointerEventArgs::EventType::MOVE) {
                action = ui::Action::MOVE;
            }
            
            for (const auto &topLevelElement : _topLevelElements) {
                if (topLevelElement->onInteraction(action, args.pointerID, args.coordinateX, args.coordinateY)) {
                    return true;
                }
            }
            
            return false;
        });
    }
    
    StageInterfaceImpl::~StageInterfaceImpl() {
        _platform->removeEventHandler(_touchEventsToken);
    }
    
    std::shared_ptr<StageInterface::Pivot> StageInterfaceImpl::addPivot(const std::shared_ptr<Element> &parent, PivotParams &&params) {
        std::shared_ptr<PivotImpl> result = std::make_shared<PivotImpl>(*this, parent);
        result->setAnchor(params.anchorTarget, params.anchorH, params.anchorV, params.anchorOffset.x, params.anchorOffset.y);
        
        if (parent == nullptr) {
            _topLevelElements.emplace_back(result);
        }
        else {
            std::dynamic_pointer_cast<ElementImpl>(parent)->attachElement(result);
        }
        
        return result;
    }
    
    std::shared_ptr<StageInterface::Image> StageInterfaceImpl::addImage(const std::shared_ptr<Element> &parent, ImageParams &&params) {
        std::shared_ptr<ImageImpl> result;
        
        if (const resource::TextureInfo *info = _resourceProvider->getTextureInfo(params.textureBase)) {
            result = std::make_shared<ImageImpl>(*this, parent, math::vector2f(info->width, info->height), params.capturePointer);
            result->setAnchor(params.anchorTarget, params.anchorH, params.anchorV, params.anchorOffset.x, params.anchorOffset.y);
            result->setActiveArea(params.activeAreaOffset, params.activeAreaRadius);
            result->setBaseTexture(params.textureBase);
            
            if (params.textureAction && params.textureAction[0]) {
                result->setActionTexture(params.textureAction);
            }
            
            if (parent == nullptr) {
                _topLevelElements.emplace_back(result);
            }
            else {
                std::dynamic_pointer_cast<ElementImpl>(parent)->attachElement(result);
            }
        }
        else {
            _platform->logError("[StageInterfaceImpl::addImage] 'textureBase' is not existing texture '%s'\n", params.textureBase);
        }
        
        return result;
    }
    
    void StageInterfaceImpl::clear() {
    
    }
    
    void StageInterfaceImpl::updateAndDraw(float dtSec) {
//        _rendering->forTarget(nullptr, nullptr, std::nullopt, [&](foundation::RenderingInterface &rendering) {
//            rendering.applyShader(_uiShader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::MIXING, foundation::DepthBehavior::DISABLED);
//            for (const auto &topLevelElement : _topLevelElements) {
//                topLevelElement->updateCoordinates();
//
//            }
//        });
    }
    
}
