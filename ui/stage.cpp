
#include "stage.h"

#include "stage_base.h"
#include "stage_pivot.h"
#include "stage_image.h"
#include "stage_text.h"

#include <list>
#include <memory>

namespace ui {
    class StageInterfaceImpl : public StageInterface, public StageFacility {
    public:
        StageInterfaceImpl(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            const resource::TextureProviderPtr &textureProvider,
            const resource::FontAtlasProviderPtr &fontAtlasProvider
        );
        ~StageInterfaceImpl() override;
        
    public:
        const foundation::PlatformInterfacePtr &getPlatform() const override { return _platform; }
        const foundation::RenderingInterfacePtr &getRendering() const override { return _rendering; }
        const resource::TextureProviderPtr &getTextureProvider() const override { return _textureProvider; }
        const resource::FontAtlasProviderPtr &getFontAtlasProvider() const override { return _fontAtlasProvider; }
        
    public:
        auto addPivot(const std::shared_ptr<Element> &parent, PivotParams &&params) -> std::shared_ptr<Pivot> override;
        auto addImage(const std::shared_ptr<Element> &parent, ImageParams &&params) -> std::shared_ptr<Image> override;
        auto addText(const std::shared_ptr<Element> &parent, TextParams &&params) -> std::shared_ptr<Text> override;
        auto addJoystick(const std::shared_ptr<Element> &parent, JoystickParams &&params) -> std::shared_ptr<StageInterface::Element> override;
        auto addStepper(const std::shared_ptr<Element> &parent, StepperParams &&params) -> std::shared_ptr<StageInterface::Element> override;
        void clear() override;
        
        void updateAndDraw(float dtSec) override;
        
    private:
        const foundation::PlatformInterfacePtr _platform;
        const foundation::RenderingInterfacePtr _rendering;
        const resource::TextureProviderPtr _textureProvider;
        const resource::FontAtlasProviderPtr _fontAtlasProvider;
        
        foundation::EventHandlerToken _touchEventsToken;
        ShaderConstants _shaderConstants;
        foundation::RenderShaderPtr _uiShader;
        std::list<std::shared_ptr<ElementImpl>> _topLevelElements;
    };
    
    std::shared_ptr<StageInterface> StageInterface::instance(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const resource::TextureProviderPtr &textureProvider,
        const resource::FontAtlasProviderPtr &fontAtlasProvider
    ) {
        return std::make_shared<StageInterfaceImpl>(platform, rendering, textureProvider, fontAtlasProvider);
    }
}

namespace {
    const char *g_uiShaderSrc = R"(
        const {
            position_size : float4
            texcoord : float4
            color : float4
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
            float4 texcolor = _tex2nearest(0, input_texcoord);
            output_color[0] = _lerp(texcolor, float4(1.0, 1.0, 1.0, texcolor.r) * const_color, const_flags.r);
        }
    )";
}

namespace ui {
    StageInterfaceImpl::StageInterfaceImpl(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const resource::TextureProviderPtr &textureProvider,
        const resource::FontAtlasProviderPtr &fontAtlasProvider
    )
    : _platform(platform)
    , _rendering(rendering)
    , _textureProvider(textureProvider)
    , _fontAtlasProvider(fontAtlasProvider)
    , _touchEventsToken(nullptr)
    {
        _shaderConstants = ShaderConstants {};
        _uiShader = _rendering->createShader("stage_element", g_uiShaderSrc, {
            {"ID", foundation::RenderShaderInputFormat::ID}
        });
        
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
        
        if (const resource::TextureInfo *info = _textureProvider->getTextureInfo(params.textureBase)) {
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
    
    std::shared_ptr<StageInterface::Text> StageInterfaceImpl::addText(const std::shared_ptr<Element> &parent, TextParams &&params) {
        std::shared_ptr<TextImpl> result = std::make_shared<TextImpl>(*this, parent, params.fontSize);
        result->setAnchor(params.anchorTarget, params.anchorH, params.anchorV, params.anchorOffset.x, params.anchorOffset.y);
        result->setShadow(params.shadowEnabled, params.shadowOffset);
        result->setColor(params.rgba);
        
        if (parent == nullptr) {
            _topLevelElements.emplace_back(result);
        }
        else {
            std::dynamic_pointer_cast<ElementImpl>(parent)->attachElement(result);
        }
        
        return result;
    }
    
    std::shared_ptr<StageInterface::Element> StageInterfaceImpl::addJoystick(const std::shared_ptr<Element> &parent, JoystickParams &&params) {
        std::shared_ptr<StageInterface::Image> bg;
        float maxOffset = params.maxThumbOffset;
        
        if (const resource::TextureInfo *info = _textureProvider->getTextureInfo(params.textureBackground)) {
            bg = addImage(nullptr, ui::StageInterface::ImageParams {
                .activeAreaOffset = 0.5f * info->width,
                .activeAreaRadius = 0.7f * info->width,
                .anchorTarget = params.anchorTarget,
                .anchorOffset = params.anchorOffset,
                .anchorH = params.anchorH,
                .anchorV = params.anchorV,
                .capturePointer = true,
                .textureBase = params.textureBackground,
            });
            auto pivot = addPivot(bg, PivotParams {
                .anchorH = HorizontalAnchor::CENTER,
                .anchorV = VerticalAnchor::MIDDLE,
            });
            auto thumb = addImage(pivot, ui::StageInterface::ImageParams {
                .anchorH = HorizontalAnchor::CENTER,
                .anchorV = VerticalAnchor::MIDDLE,
                .textureBase = params.textureThumb,
            });
            
            auto handler = std::make_shared<util::callback<void(const math::vector2f &direction)>>(std::move(params.handler));
            std::weak_ptr<StageInterface::Image> weakBg = bg;
            std::weak_ptr<StageInterface::Pivot> weakPivot = pivot;
            
            auto action = [handler, maxOffset](const std::shared_ptr<StageInterface::Image> &bg, const std::shared_ptr<StageInterface::Pivot> &pivot, float x, float y, bool reset) {
                if (bg && pivot) {
                    if (reset) {
                        pivot->setScreenPosition(bg->getPosition() + 0.5f * bg->getSize());
                        handler->operator()(math::vector2f(0, 0));
                    }
                    else {
                        const math::vector2f center = bg->getPosition() + 0.5f * bg->getSize();
                        math::vector2f dir = (math::vector2f(x, y) - center);
                        
                        if (dir.length() > maxOffset) {
                            dir = dir.normalized(maxOffset);
                        }
                        
                        pivot->setScreenPosition(center + dir);
                        handler->operator()(math::vector2f(dir.x, -dir.y) / maxOffset);
                    }
                }
            };
            
            bg->setActionHandler(Action::CAPTURE, [action, weakBg, weakPivot](float x, float y) {
                action(weakBg.lock(), weakPivot.lock(), x, y, false);
            });
            bg->setActionHandler(Action::MOVE, [action, weakBg, weakPivot](float x, float y) {
                action(weakBg.lock(), weakPivot.lock(), x, y, false);
            });
            bg->setActionHandler(Action::RELEASE, [action, weakBg, weakPivot](float x, float y) {
                action(weakBg.lock(), weakPivot.lock(), x, y, true);
            });
        }
        else {
            _platform->logError("[StageInterfaceImpl::addJoystick] 'textureBackground' is not existing texture '%s'\n", params.textureBackground);
        }
        
        return bg;
    }
    
    std::shared_ptr<StageInterface::Element> StageInterfaceImpl::addStepper(const std::shared_ptr<Element> &parent, StepperParams &&params) {
        return nullptr;
    }
    
    void StageInterfaceImpl::clear() {
    
    }
    
    void StageInterfaceImpl::updateAndDraw(float dtSec) {
        _rendering->applyState(_uiShader, foundation::RenderPassCommonConfigs::OVERLAY(foundation::BlendType::MIXING));

        for (const auto &topLevelElement : _topLevelElements) {
            topLevelElement->updateCoordinates();
            topLevelElement->draw(_shaderConstants);
        }
    }
    
}
