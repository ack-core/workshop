
// TODO: dpi-scaling, sub-tree full transform
// + scale inheritance

#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/math.h"
#include "foundation/util.h"

#include "providers/resource_provider.h"
#include "providers/fontatlas_provider.h"

#include <memory>
#include <functional>

namespace ui {
    enum class Action {
        PRESS,
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
    enum class TextAlign {
        LEFT,
        CENTER,
        RIGHT,
    };

    class StageInterface {
    public:
        static std::shared_ptr<StageInterface> instance(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            const resource::ResourceProviderPtr &resourceProvider,
            const resource::FontAtlasProviderPtr &fontAtlasProvider
        );
        
    public:
        struct Element {
            virtual auto getPosition() const -> const math::vector2f & = 0;
            virtual auto getSize() const -> const math::vector2f & = 0;
            virtual ~Element() = default;
        };
        struct Interactor : public virtual Element {
            virtual void setActionHandler(util::callback<void(ui::Action action, float x, float y)> &&handler) = 0;
            virtual ~Interactor() = default;
        };
        struct Pivot : public virtual Element {
            virtual void setScreenPosition(const math::vector2f &position) = 0;
            virtual void setWorldPosition(const math::vector3f &position) = 0;
        };
        struct Image : public virtual Interactor {
            virtual ~Image() = default;
            virtual void setTexture(const char *texturePath) = 0;
            virtual void setTexture(const foundation::RenderTexturePtr &texture) = 0;
        };
        struct Img9Slice : public virtual Interactor {
            virtual ~Img9Slice() = default;
            virtual void setTexture(const char *texturePath, const math::vector3f &sliceArgs) = 0;
        };
        struct TextLine : public virtual Element {
            virtual ~TextLine() = default;
            virtual void setText(const char *utf8text) = 0;
            virtual foundation::RenderTexturePtr getTexture() = 0;
        };
        
    public:
        struct PivotParams {
            const std::shared_ptr<StageInterface::Element> anchorTarget;
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
            const math::vector2f anchorOffset = math::vector2f(0, 0);
        };
        struct CropParams {
            const std::shared_ptr<StageInterface::Element> anchorTarget;
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
            const math::vector2f anchorOffset = math::vector2f(0, 0);
            const math::vector2f size = math::vector2f(100.0f, 100.0f);
        };
        struct ImageParams {
            const std::shared_ptr<StageInterface::Element> anchorTarget;
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
            const math::vector2f anchorOffset = math::vector2f(0, 0);
            const char *texture = "";
            const float activeAreaOffset = 0.0f;
            const float activeAreaRadius = 0.0f;
        };
        struct Img9SliceParams {
            const std::shared_ptr<StageInterface::Element> anchorTarget;
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
            const math::vector2f anchorOffset = math::vector2f(0, 0);
            const math::vector2f size = math::vector2f(100.0f, 100.0f);
            const char *texture = "";
            const math::vector3f sliceArgs = 0.0f;
            const float activeAreaOffset = 0.0f;
            const float activeAreaRadius = 0.0f;
        };
        struct TextLineParams {
            const std::shared_ptr<StageInterface::Element> anchorTarget;
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
            const math::vector2f anchorOffset = math::vector2f(0, 0);
            const std::uint8_t fontSize = 20;
            const math::color fontColor = math::color(1.0f, 1.0f, 1.0f, 1.0f);
            const math::color shadowColor = math::color(0.0f, 0.0f, 0.0f, 0.0f);
            const math::vector2f shadowOffset = {0, 0};
            const std::uint8_t shadowBlur = 0;
        };
        struct TextBlockParams {
            const std::shared_ptr<StageInterface::Element> anchorTarget;
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
            const math::vector2f anchorOffset = math::vector2f(0, 0);
            const math::vector2f size = math::vector2f(100.0f, 100.0f);
            const std::uint8_t fontSize;
            const TextAlign textAlign = TextAlign::LEFT;
        };
        
    public:
        virtual auto addPivot(const std::shared_ptr<Element> &parent, ui::StageInterface::PivotParams &&params) -> std::shared_ptr<Pivot> = 0;
        virtual auto addImage(const std::shared_ptr<Element> &parent, ui::StageInterface::ImageParams &&params) -> std::shared_ptr<Image> = 0;
        virtual auto addImg9Slice(const std::shared_ptr<Element> &parent, ui::StageInterface::Img9SliceParams &&params) -> std::shared_ptr<Img9Slice> = 0;
        virtual auto addTextLine(const std::shared_ptr<Element> &parent, ui::StageInterface::TextLineParams &&params) -> std::shared_ptr<TextLine> = 0;
        
        virtual void clear() = 0;
        virtual void updateAndDraw(float dtSec) = 0;
        
    public:
        virtual ~StageInterface() = default;
    };
    
    using StageInterfacePtr = std::shared_ptr<StageInterface>;
    using ElementPtr = std::shared_ptr<StageInterface::Element>;
    using PivotPtr = std::shared_ptr<StageInterface::Pivot>;
    using ImagePtr = std::shared_ptr<StageInterface::Image>;
}

