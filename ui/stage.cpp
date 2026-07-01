
#include "stage.h"
#include "foundation/layouts.h"

#include <list>
#include <memory>

namespace ui {
    struct DrawingInstance {
        math::vector4f positionAndSize;
        math::vector4f uvCoords;
        math::color color;
        math::vector4f args; //[is R component only, 0, 0, 0]
    };

    class StageFacility {
    public:
        virtual const foundation::PlatformInterfacePtr &getPlatform() const = 0;
        virtual const foundation::RenderingInterfacePtr &getRendering() const = 0;
        virtual const resource::ResourceProviderPtr &getResourceProvider() const = 0;
        virtual const resource::FontAtlasProviderPtr &getFontAtlasProvider() const = 0;
        virtual ~StageFacility() = default;
    };
}

//---

namespace ui {
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
        virtual void draw() {
            for (auto &item : _attachedElements) {
                item->draw();
            }
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
        InteractorImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent) : ElementImpl(facility, parent) {}
        ~InteractorImpl() override {}
        
        void setActiveArea(float offset, float radius) {
            _activeAreaOffset = offset;
            _activeAreaRadius = radius;
        }
        void setActionHandler(util::callback<void(ui::Action action, float x, float y)> &&handler) override {
            _handler = std::move(handler);
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

            if (isInArea && action == ui::Action::PRESS) {
                _pointerId = id;
                _currentAction = action;

                if (_handler) {
                    _handler(action, x, y);
                    return true;
                }
            }
            if (action == ui::Action::MOVE && _pointerId != foundation::INVALID_POINTER_ID) {
                _currentAction = action;
                
                if (_handler) {
                    _handler(action, x, y);
                    return true;
                }
            }
            if (action == ui::Action::RELEASE && _pointerId != foundation::INVALID_POINTER_ID) {
                _pointerId = foundation::INVALID_POINTER_ID;
                _currentAction = action;

                if (_handler) {
                    _handler(action, x, y);
                    return true;
                }
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
        
        util::callback<void(ui::Action action, float x, float y)> _handler;
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
        ImageImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent, const math::vector2f &size) : InteractorImpl(facility, parent) {
            _size = size;
        }
        ~ImageImpl() override {}
        
    public:
        void setTexture(const char *texturePath) override {
            std::string path = texturePath;
            _facility.getResourceProvider()->getOrLoadTexture(texturePath, [weak = weak_from_this(), path](const foundation::RenderTexturePtr &texture) {
                if (std::shared_ptr<ImageImpl> self = weak.lock()) {
                    self->_size.x = texture->getWidth();
                    self->_size.y = texture->getHeight();
                    self->_texture = texture;
                }
            });
        }
        void setTexture(const foundation::RenderTexturePtr &texture) override {
            _size.x = texture->getWidth();
            _size.y = texture->getHeight();
            _texture = texture;
        }
        void draw() override {
            const foundation::RenderingInterfacePtr &rendering = _facility.getRendering();
            
            if (_texture) {
                DrawingInstance instance;
                instance.positionAndSize = math::vector4f(_globalPosition, _size);
                instance.uvCoords = math::vector4f(0, 0, 1, 1);
                instance.color = math::vector4f(1.0f, 1.0f, 1.0f, 1.0f);
                instance.args = math::vector4f(0.0f, 0.0f, 0.0f, 0.0f);
                
                rendering->applyTextures({{_texture, foundation::SamplerType::NEAREST}});
                rendering->draw(&instance, 1);
                
                for (auto &item : _attachedElements) {
                    item->draw();
                }
            }
        }
        
    private:
        foundation::RenderTexturePtr _texture;
    };
}

//---

namespace ui {
    class Img9SliceImpl : public InteractorImpl, public std::enable_shared_from_this<Img9SliceImpl>, public StageInterface::Img9Slice {
    public:
        Img9SliceImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent, const math::vector2f &size) : InteractorImpl(facility, parent) {
            _size = size;
        }
        ~Img9SliceImpl() override {}
        
    public:
        void setTexture(const char *texturePath, const math::vector3f &sliceArgs) override {
            std::string path = texturePath;
            _facility.getResourceProvider()->getOrLoadTexture(texturePath, [weak = weak_from_this(), path, sliceArgs](const foundation::RenderTexturePtr &texture) {
                if (std::shared_ptr<Img9SliceImpl> self = weak.lock()) {
                    self->_texture = texture;
                    self->_sliceArgs = sliceArgs;
                }
            });
        }
        void draw() override {
            const foundation::RenderingInterfacePtr &rendering = _facility.getRendering();
            
            if (_texture) {
                const int EGDE_REPEAT_MAX = 10;
                const int INSTANCES_MAX = EGDE_REPEAT_MAX * 4 + 5;
                DrawingInstance instances[INSTANCES_MAX] = {0};
                
                const math::vector2f rbSliceStart = math::vector2f(_size.x - _sliceArgs.z, _size.y - _sliceArgs.z);
                const math::vector2f txSliceSize = _sliceArgs.z / math::vector2f(_texture->getWidth(), _texture->getHeight());

                math::vector2f edgeLeft = math::vector2f(_size.x - 2.0f * _sliceArgs.z, _size.y - 2.0f * _sliceArgs.z);
                math::vector2f edgeOffset = math::vector2f(_sliceArgs.z, _sliceArgs.z);

                instances[0].positionAndSize = math::vector4f(_globalPosition, math::vector2f(_sliceArgs.z, _sliceArgs.z));
                instances[0].uvCoords = math::vector4f(0, 0, txSliceSize.x, txSliceSize.y);
                instances[1].positionAndSize = math::vector4f(_globalPosition + math::vector2f(rbSliceStart.x, 0), math::vector2f(_sliceArgs.z, _sliceArgs.z));
                instances[1].uvCoords = math::vector4f(txSliceSize.x, 0, txSliceSize.x + txSliceSize.x, txSliceSize.y);
                instances[2].positionAndSize = math::vector4f(_globalPosition + math::vector2f(0, rbSliceStart.y), math::vector2f(_sliceArgs.z, _sliceArgs.z));
                instances[2].uvCoords = math::vector4f(0, txSliceSize.y, txSliceSize.x, txSliceSize.y + txSliceSize.y);
                instances[3].positionAndSize = math::vector4f(_globalPosition + math::vector2f(rbSliceStart.x, rbSliceStart.y), math::vector2f(_sliceArgs.z, _sliceArgs.z));
                instances[3].uvCoords = math::vector4f(txSliceSize.x, txSliceSize.y, txSliceSize.x + txSliceSize.x, txSliceSize.y + txSliceSize.y);
                instances[4].positionAndSize = math::vector4f(_globalPosition + math::vector2f(_sliceArgs.z, _sliceArgs.z), edgeLeft);
                instances[4].uvCoords = math::vector4f(2.0f * txSliceSize.x, 2.0f * txSliceSize.y, 3.0f * txSliceSize.x, 3.0f * txSliceSize.y);
                int index = 5;
                
                for (int i = 0; i < EGDE_REPEAT_MAX && edgeLeft.x > 0.0f; i++) {
                    const float repWidth = std::min(edgeLeft.x, _sliceArgs.x);
                    instances[index].positionAndSize = math::vector4f(_globalPosition + math::vector2f(edgeOffset.x, 0), math::vector2f(repWidth, _sliceArgs.z));
                    instances[index++].uvCoords = math::vector4f(2.0f * txSliceSize.x, 0, 2.0f * txSliceSize.x + (repWidth / _texture->getWidth()), txSliceSize.y);
                    instances[index].positionAndSize = math::vector4f(_globalPosition + math::vector2f(edgeOffset.x, rbSliceStart.y), math::vector2f(repWidth, _sliceArgs.z));
                    instances[index++].uvCoords = math::vector4f(2.0f * txSliceSize.x, txSliceSize.y, 2.0f * txSliceSize.x + (repWidth / _texture->getWidth()), 2.0f * txSliceSize.y);
                    edgeLeft.x -= _sliceArgs.x;
                    edgeOffset.x += _sliceArgs.x;
                }
                for (int i = 0; i < EGDE_REPEAT_MAX && edgeLeft.y > 0.0f; i++) {
                    const float repHeight = std::min(edgeLeft.y, _sliceArgs.y);
                    instances[index].positionAndSize = math::vector4f(_globalPosition + math::vector2f(0, edgeOffset.y), math::vector2f(_sliceArgs.z, repHeight));
                    instances[index++].uvCoords = math::vector4f(0, 2.0f * txSliceSize.y, txSliceSize.x, 2.0f * txSliceSize.y + (repHeight / _texture->getHeight()));
                    instances[index].positionAndSize = math::vector4f(_globalPosition + math::vector2f(rbSliceStart.x, edgeOffset.y), math::vector2f(_sliceArgs.z, repHeight));
                    instances[index++].uvCoords = math::vector4f(txSliceSize.x, 2.0f * txSliceSize.y, 2.0f * txSliceSize.x, 2.0f * txSliceSize.y + (repHeight / _texture->getHeight()));
                    edgeLeft.y -= _sliceArgs.y;
                    edgeOffset.y += _sliceArgs.y;
                }
                for (int i = 0; i < index; i++) {
                    instances[i].color = math::color(1.0f, 1.0f, 1.0f, 1.0f);
                    instances[i].args = math::vector4f(0.0f, 0.0f, 0.0f, 0.0f);
                }
                
                rendering->applyTextures({{_texture, foundation::SamplerType::NEAREST}});
                rendering->draw(instances, index);
                
                for (auto &item : _attachedElements) {
                    item->draw();
                }
            }
        }
        
    private:
        foundation::RenderTexturePtr _texture;
        math::vector3f _sliceArgs;
    };
}

//---

namespace ui {
    class TextLineImpl : public InteractorImpl, public std::enable_shared_from_this<TextLineImpl>, public StageInterface::TextLine {
    public:
        TextLineImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent) : InteractorImpl(facility, parent) {}
        ~TextLineImpl() override {}
        
    public:
        void setText(const char *utf8text) override {
            _size = _facility.getFontAtlasProvider()->getTextWidth(utf8text, _fontSize);
            _text = utf8text;
            _makeText();
        }
        
        foundation::RenderTexturePtr getTexture() override {
            return _textureWeak.lock();
        }
        
        void draw() override {
            if (_instances.size()) {
                if (auto texture = _textureWeak.lock()) {
                    const foundation::RenderingInterfacePtr &rendering = _facility.getRendering();
                    
                    std::uint32_t instanceCount = 0;
                    _fillInstances(_shadow, _shadowColor, _shadowOffset, instanceCount);
                    _fillInstances(_chars, _fontColor, {}, instanceCount);
                    
                    if (instanceCount) {
                        rendering->applyTextures({{texture, foundation::SamplerType::NEAREST}});
                        rendering->draw(_instances.data(), instanceCount);
                    }
                }
                else {
                    _makeText();
                }
            }
        }
        
        void setFontParameters(const math::color &fontColor, std::uint8_t fontSize, const math::vector2f &shadowOffset, const math::color &shadowColor, std::uint8_t shadowBlur) {
            _fontSize = fontSize;
            _fontColor = fontColor;
            _shadowColor = shadowColor;
            _shadowOffset = shadowOffset;
            _shadowBlur = shadowBlur;
        }
        
    private:
        void _makeText() {
            _instances.clear();

            if (_shadowColor.a > 0.0f) {
                _facility.getFontAtlasProvider()->getTextFontAtlas(_text.data(), _fontSize, _shadowBlur, [weak = weak_from_this()](std::vector<resource::FontCharInfo> &&shadow, const foundation::RenderTexturePtr &) {
                    if (shadow.size()) {
                        if (std::shared_ptr<TextLineImpl> self = weak.lock()) {
                            self->_shadow = std::move(shadow);
                        }
                    }
                });
            }
            _facility.getFontAtlasProvider()->getTextFontAtlas(_text.data(), _fontSize, 0, [weak = weak_from_this()](std::vector<resource::FontCharInfo> &&chars, const foundation::RenderTexturePtr &texture) {
                if (chars.size()) {
                    if (std::shared_ptr<TextLineImpl> self = weak.lock()) {
                        self->_instances.resize(2 * chars.size());
                        self->_chars = std::move(chars);
                        self->_textureWeak = texture;
                    }
                }
            });
        }
        void _fillInstances(const std::vector<resource::FontCharInfo> &src, const math::color &color, const math::vector2f &offset, std::uint32_t &instanceCount) {
            float offsetX = offset.x;
            
            for (auto &ch : src) {
                if (ch.pxSize.x > std::numeric_limits<float>::epsilon()) {
                    _instances[instanceCount].positionAndSize = math::vector4f(_globalPosition.x + offsetX + ch.lsb, _globalPosition.y + ch.voffset + offset.y, ch.pxSize.x, ch.pxSize.y);
                    _instances[instanceCount].uvCoords = math::vector4f(ch.txLT, ch.txRB);
                    _instances[instanceCount].color = color;
                    _instances[instanceCount].args = math::vector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    instanceCount++;
                }
                
                offsetX += ch.advance;
            }
        }

        
    private:
        math::color _fontColor;
        std::uint8_t _fontSize = 0;
        math::color _shadowColor;
        math::vector2f _shadowOffset;
        std::uint8_t _shadowBlur = 0;
        std::string _text;
        std::weak_ptr<foundation::RenderTexture> _textureWeak;
        std::vector<resource::FontCharInfo> _chars;
        std::vector<resource::FontCharInfo> _shadow;
        std::vector<DrawingInstance> _instances;
    };
}


//---

namespace ui {
    class StageInterfaceImpl : public StageInterface, public StageFacility {
    public:
        StageInterfaceImpl(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            const resource::ResourceProviderPtr &resourceProvider,
            const resource::FontAtlasProviderPtr &fontAtlasProvider
        );
        ~StageInterfaceImpl() override;
        
    public:
        const foundation::PlatformInterfacePtr &getPlatform() const override { return _platform; }
        const foundation::RenderingInterfacePtr &getRendering() const override { return _rendering; }
        const resource::ResourceProviderPtr &getResourceProvider() const override { return _resourceProvider; }
        const resource::FontAtlasProviderPtr &getFontAtlasProvider() const override { return _fontAtlasProvider; }
        
    public:
        auto addPivot(const std::shared_ptr<Element> &parent, PivotParams &&params) -> std::shared_ptr<Pivot> override;
        auto addImage(const std::shared_ptr<Element> &parent, ImageParams &&params) -> std::shared_ptr<Image> override;
        auto addImg9Slice(const std::shared_ptr<Element> &parent, ui::StageInterface::Img9SliceParams &&params) -> std::shared_ptr<Img9Slice> override;
        auto addTextLine(const std::shared_ptr<Element> &parent, ui::StageInterface::TextLineParams &&params) -> std::shared_ptr<TextLine> override;

        void clear() override;
        void updateAndDraw(float dtSec) override;
        
    private:
        const foundation::PlatformInterfacePtr _platform;
        const foundation::RenderingInterfacePtr _rendering;
        const resource::ResourceProviderPtr _resourceProvider;
        const resource::FontAtlasProviderPtr _fontAtlasProvider;
        
        foundation::EventHandlerToken _touchEventsToken;
        foundation::RenderShaderPtr _uiShader;
        std::list<std::shared_ptr<ElementImpl>> _topLevelElements;
    };
    
    std::shared_ptr<StageInterface> StageInterface::instance(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const resource::ResourceProviderPtr &resourceProvider,
        const resource::FontAtlasProviderPtr &fontAtlasProvider
    )
    {
        return std::make_shared<StageInterfaceImpl>(platform, rendering, resourceProvider, fontAtlasProvider);
    }
}

namespace {
    const char *g_uiShaderSrc = R"(
        inout {
            color : float4
            args : float4
            texcoord : float2
        }
        vssrc {
            float2 screenTransform = float2(2.0, -2.0) / frame_rtBounds.xy;
            float2 vertexCoord = float2(repeat_ID & 0x1, repeat_ID >> 0x1);
            output_texcoord = _lerp(vertex_uv.xy, vertex_uv.zw, vertexCoord);
            vertexCoord = vertexCoord * vertex_position_size.zw + vertex_position_size.xy;
            output_position = float4(vertexCoord.xy * screenTransform + float2(-1.0, 1.0), 0.1, 1); //
            output_color = vertex_color;
            output_args = vertex_args;
        }
        fssrc {
            float4 texcolor = _tex2d(0, input_texcoord);
            output_color[0] = input_color * _lerp(texcolor, float4(1.0, 1.0, 1.0, texcolor.r), input_args.x);
        }
    )";
}

namespace ui {
    StageInterfaceImpl::StageInterfaceImpl(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const resource::ResourceProviderPtr &resourceProvider,
        const resource::FontAtlasProviderPtr &fontAtlasProvider
    )
    : _platform(platform)
    , _rendering(rendering)
    , _resourceProvider(resourceProvider)
    , _fontAtlasProvider(fontAtlasProvider)
    , _touchEventsToken(nullptr)
    {
        _uiShader = _rendering->createShader("stage_element", g_uiShaderSrc, layouts::VTXUIUV);
        _touchEventsToken = _platform->addPointerEventHandler([this](const foundation::PlatformPointerEventArgs &args) {
            ui::Action action = ui::Action::RELEASE;
            
            if (args.type == foundation::PlatformPointerEventArgs::EventType::START) {
                action = ui::Action::PRESS;
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
        
        if (const resource::TextureInfo *info = _resourceProvider->getTextureInfo(params.texture)) {
            result = std::make_shared<ImageImpl>(*this, parent, math::vector2f(info->width, info->height));
            result->setAnchor(params.anchorTarget, params.anchorH, params.anchorV, params.anchorOffset.x, params.anchorOffset.y);
            result->setActiveArea(params.activeAreaOffset, params.activeAreaRadius);
            result->setTexture(params.texture);
                        
            if (parent == nullptr) {
                _topLevelElements.emplace_back(result);
            }
            else {
                std::dynamic_pointer_cast<ElementImpl>(parent)->attachElement(result);
            }
        }
        else {
            _platform->logError("[StageInterfaceImpl::addImage] '%s' is not existing texture\n", params.texture);
        }
        
        return result;
    }

    std::shared_ptr<StageInterfaceImpl::Img9Slice> StageInterfaceImpl::addImg9Slice(const std::shared_ptr<Element> &parent, ui::StageInterface::Img9SliceParams &&params) {
        std::shared_ptr<Img9SliceImpl> result;
        
        if (const resource::TextureInfo *info = _resourceProvider->getTextureInfo(params.texture)) {
            result = std::make_shared<Img9SliceImpl>(*this, parent, params.size);
            result->setAnchor(params.anchorTarget, params.anchorH, params.anchorV, params.anchorOffset.x, params.anchorOffset.y);
            result->setActiveArea(params.activeAreaOffset, params.activeAreaRadius);
            result->setTexture(params.texture, params.sliceArgs);
                        
            if (parent == nullptr) {
                _topLevelElements.emplace_back(result);
            }
            else {
                std::dynamic_pointer_cast<ElementImpl>(parent)->attachElement(result);
            }
        }
        else {
            _platform->logError("[StageInterfaceImpl::addImage] '%s' is not existing texture\n", params.texture);
        }
        
        return result;

    }

    std::shared_ptr<StageInterfaceImpl::TextLine> StageInterfaceImpl::addTextLine(const std::shared_ptr<Element> &parent, ui::StageInterface::TextLineParams &&params) {
        std::shared_ptr<TextLineImpl> result = std::make_shared<TextLineImpl>(*this, parent);
        result->setAnchor(params.anchorTarget, params.anchorH, params.anchorV, params.anchorOffset.x, params.anchorOffset.y);
        result->setFontParameters(params.fontColor, params.fontSize, params.shadowOffset, params.shadowColor, params.shadowBlur);

        if (parent == nullptr) {
            _topLevelElements.emplace_back(result);
        }
        else {
            std::dynamic_pointer_cast<ElementImpl>(parent)->attachElement(result);
        }

        return result;
    }
    
    void StageInterfaceImpl::clear() {
    
    }
    
    void StageInterfaceImpl::updateAndDraw(float dtSec) {
        _rendering->forTarget(nullptr, nullptr, math::color(0.3f, 0.3f, 0.3f, 1.0f), [&](foundation::RenderingInterface &rendering) {
            rendering.applyShader(_uiShader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::MIXING, foundation::DepthBehavior::DISABLED);
            for (const auto &topLevelElement : _topLevelElements) {
                topLevelElement->updateCoordinates();
                topLevelElement->draw();
            }
        });
    }
    
}
