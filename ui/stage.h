
// TODO: dpi-scaling

#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/math.h"

#include "providers/texture_provider.h"
#include "providers/fontatlas_provider.h"

#include <memory>
#include <functional>

namespace ui {
    enum class Action {
        CAPTURE,
        MOVE,
        RELEASE
    };
    enum class VerticalAnchor {
        TOPSIDE,
        TOP,
        MIDDLE,
        BOTTOM,
        BOTTOMSIDE
    };
    enum class HorizontalAnchor {
        LEFTSIDE,
        LEFT,
        CENTER,
        RIGHT,
        RIGHTSIDE
    };
    
    class StageInterface {
    public:
        static std::shared_ptr<StageInterface> instance(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            const resource::TextureProviderPtr &textureProvider,
            const resource::FontAtlasProviderPtr &fontAtlasProvider
        );
        
    public:
        struct Element {
            virtual auto getPosition() const -> const math::vector2f & = 0;
            virtual auto getSize() const -> const math::vector2f & = 0;
            virtual ~Element() = default;
        };
        struct Interactor : public virtual Element {
            virtual void setActionHandler(Action action, util::callback<void(float x, float y)> &&handler) = 0;
            virtual ~Interactor() = default;
        };
        struct Pivot : public virtual Element {
            virtual void setScreenPosition(const math::vector2f &position) = 0;
            virtual void setWorldPosition(const math::vector3f &position) = 0;
        };
        struct Image : public virtual Interactor {
            virtual void setColor(const math::color &rgba) = 0;
            virtual ~Image() = default;
        };
        struct Text : public virtual Element {
            virtual void setText(const char *text) = 0;
            virtual void setColor(const math::color &rgba) = 0;
            virtual ~Text() = default;
        };
        
    public:
        struct PivotParams {
            const std::shared_ptr<StageInterface::Element> anchorTarget;
            const math::vector2f anchorOffset = math::vector2f(0, 0);
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
        };
        struct ImageParams {
            const std::shared_ptr<StageInterface::Element> anchorTarget;
            const math::vector2f anchorOffset = math::vector2f(0, 0);
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
            const char *textureBase = "";
            const char *textureAction = "";
            const float activeAreaOffset = 0.0f;
            const float activeAreaRadius = 0.0f;
            const bool capturePointer = false;
        };
        struct TextParams {
            const std::shared_ptr<StageInterface::Element> anchorTarget;
            const math::vector2f anchorOffset = math::vector2f(0, 0);
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
            const math::vector2f shadowOffset = math::vector2f(1.0f, 1.0f);
            const math::color rgba = math::color(1.0f, 1.0f, 1.0f, 1.0f);
            const std::uint8_t fontSize = 10;
            const bool shadowEnabled = false;
        };
        struct JoystickParams {
            const std::shared_ptr<StageInterface::Element> anchorTarget;
            const math::vector2f anchorOffset = math::vector2f(0, 0);
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
            const char *textureBackground = "";
            const char *textureThumb = "";
            const float maxThumbOffset = 50.0f;
            util::callback<void(const math::vector2f &direction)> handler;
        };
        struct StepperParams {
            const std::shared_ptr<StageInterface::Element> anchorTarget;
            const math::vector2f anchorOffset = math::vector2f(0, 0);
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
            const char *textureLeftBase = "";
            const char *textureLeftAction = "";
            const char *textureRightBase = "";
            const char *textureRightAction = "";
            util::callback<void(float value)> handler;
        };
        
    public:
        virtual auto addPivot(const std::shared_ptr<Element> &parent, PivotParams &&params) -> std::shared_ptr<Pivot> = 0;
        virtual auto addImage(const std::shared_ptr<Element> &parent, ImageParams &&params) -> std::shared_ptr<Image> = 0;
        virtual auto addText(const std::shared_ptr<Element> &parent, TextParams &&params) -> std::shared_ptr<Text> = 0;
        
        virtual auto addJoystick(const std::shared_ptr<Element> &parent, JoystickParams &&params) -> std::shared_ptr<StageInterface::Element> = 0;
        virtual auto addStepper(const std::shared_ptr<Element> &parent, StepperParams &&params) -> std::shared_ptr<StageInterface::Element> = 0;
        
        virtual void clear() = 0;
        virtual void updateAndDraw(float dtSec) = 0;
        
    public:
        virtual ~StageInterface() = default;
    };
    
    using StageInterfacePtr = std::shared_ptr<StageInterface>;
    using ElementPtr = std::shared_ptr<StageInterface::Element>;
    using PivotPtr = std::shared_ptr<StageInterface::Pivot>;
    using ImagePtr = std::shared_ptr<StageInterface::Image>;
    using TextPtr = std::shared_ptr<StageInterface::Text>;
}

