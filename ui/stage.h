
// TODO: dpi-scaling

#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/math.h"
#include "foundation/util.h"

#include "providers/resource_provider.h"

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
    
    class StageInterface {
    public:
        static std::shared_ptr<StageInterface> instance(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            const resource::ResourceProviderPtr &resourceProvider
        );
        
    public:
        struct Element {
            virtual auto getPosition() const -> const math::vector2f & = 0;
            virtual auto getSize() const -> const math::vector2f & = 0;
            virtual ~Element() = default;
        };
        struct Interactor : public virtual Element {
            virtual void setActionHandler(ui::Action action, util::callback<void(float x, float y)> &&handler) = 0;
            virtual ~Interactor() = default;
        };
        struct Pivot : public virtual Element {
            virtual void setScreenPosition(const math::vector2f &position) = 0;
            virtual void setWorldPosition(const math::vector3f &position) = 0;
        };
        struct Image : public virtual Interactor {
            virtual ~Image() = default;
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
        
    public:
        virtual auto addPivot(const std::shared_ptr<Element> &parent, ui::StageInterface::PivotParams &&params) -> std::shared_ptr<Pivot> = 0;
        virtual auto addImage(const std::shared_ptr<Element> &parent, ui::StageInterface::ImageParams &&params) -> std::shared_ptr<Image> = 0;
        
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

