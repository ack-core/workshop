
// TODO: sub-tree full transform
// + scale inheritance
// + ImageSector

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

    // Interface provides 2D-ui as a tree of elements. Coordinates are in points (pixels / screen scale)
    //
    class StageInterface {
    public:
        static std::shared_ptr<StageInterface> instance(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            const resource::ResourceProviderPtr &resourceProvider,
            const resource::FontAtlasProviderPtr &fontAtlasProvider
        );

        struct StatInfo {
            std::size_t totalElementsCount = 0;
            std::size_t topLevelElementsCount = 0;
            std::size_t pivotsCount = 0;
            std::size_t imagesCount = 0;
            std::size_t imgCustomsCount = 0;
            std::size_t img9SlicesCount = 0;
            std::size_t textLinesCount = 0;
            std::size_t textBlocksCount = 0;
        };

    public:
        struct Element {
            virtual auto getPosition() const -> const math::vector2f & = 0;
            virtual auto getSize() const -> const math::vector2f & = 0;
            virtual void setAnchorOffset(const math::vector2f &offset) = 0;
            virtual ~Element() = default;
        };
        struct Interactor : public virtual Element {
            virtual void setActionHandler(util::callback<void(ui::Action action, float x, float y)> &&handler) = 0;
            virtual void setStateHandler(util::callback<void(bool locked)> &&handler) = 0;
            virtual void setLocked(bool locked) = 0;
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
            virtual void setSize(const math::vector2f &size) = 0;
        };
        struct ImgCustom : public virtual Element {
            virtual ~ImgCustom() = default;
            virtual void setTexture(const char *texturePath) = 0;
            virtual void setTexture(const foundation::RenderTexturePtr &texture) = 0;
            virtual void setGeometry(foundation::RenderTopology topology, const void *data, std::uint32_t vcnt, const std::uint32_t *indexes = nullptr, std::uint32_t icnt = 0) = 0;
            virtual void setSize(const math::vector2f &size) = 0;
            virtual void setDrawHandler(util::callback<void(StageInterface::ImgCustom &)> &&handler) = 0;
        };
        struct Img9Slice : public virtual Interactor {
            virtual ~Img9Slice() = default;
            virtual void setTexture(const char *texturePath, const math::vector3f &sliceArgs) = 0;
            virtual void setTexture(const foundation::RenderTexturePtr &texture, const math::vector3f &sliceArgs) = 0;
            virtual void setSize(const math::vector2f &size) = 0;
        };
        struct TextLine : public virtual Element {
            virtual ~TextLine() = default;
            virtual void setText(const char *utf8text) = 0;
            virtual foundation::RenderTexturePtr getTexture() = 0;
        };
        struct TextBlock : public virtual Element {
            virtual ~TextBlock() = default;
            virtual void setText(const char *utf8text) = 0;
        };
        
    public:
        // Anchors
        // Element is aligned to @anchorTarget (sibling) or to parent if target is nullptr (screen for top level)
        // LEFT/CENTER/RIGHT, TOP/MIDDLE/BOTTOM - inside the target. *SIDE - outside, next to the target edge
        // @anchorOffset moves element away from the aligned edge
        //
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
            const float activeAreaOffset = 0.0f;    // padding of touchable rect inside element bounds
            const float activeAreaRadius = 0.0f;    // extra touchable distance around touchable rect
        };
        struct ImgCustomParams {
            const std::shared_ptr<StageInterface::Element> anchorTarget;
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
            const math::vector2f anchorOffset = math::vector2f(0, 0);
            const math::vector2f size = math::vector2f(100.0f, 100.0f);
            const char* texture = "";
            const foundation::InputLayout &layout;  // vertex layout of custom geometry
            const char *shaderSource = "";          // see RenderingInterface::createShader
        };
        struct Img9SliceParams {
            const std::shared_ptr<StageInterface::Element> anchorTarget;
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
            const math::vector2f anchorOffset = math::vector2f(0, 0);
            const math::vector2f size = math::vector2f(100.0f, 100.0f);
            const char *texture = "";
            const math::vector3f sliceArgs = math::vector3f(0, 0, 0);
            const float activeAreaOffset = 0.0f;
            const float activeAreaRadius = 0.0f;
            const bool fill = true;                 // draw center part or not
        };
        struct TextLineParams {
            const std::shared_ptr<StageInterface::Element> anchorTarget;
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
            const math::vector2f anchorOffset = math::vector2f(0, 0);
            const std::uint8_t fontSize = 10;
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
            const TextAlign textAlign = TextAlign::LEFT;
            const std::uint8_t fontSize = 10;
            const math::color fontColor = math::color(1.0f, 1.0f, 1.0f, 1.0f);
            const math::color shadowColor = math::color(0.0f, 0.0f, 0.0f, 0.0f);
            const math::vector2f shadowOffset = {0, 0};
            const std::uint8_t shadowBlur = 0;
        };
        
    public:
        virtual auto getPlatform() const -> const foundation::PlatformInterfacePtr & = 0;
        virtual auto getResourceProvider() const -> const resource::ResourceProviderPtr & = 0;
        virtual auto getStatistics() const -> const StatInfo & = 0;

        // Get element added with name
        // @return - element or nullptr if there is no such element
        //
        virtual auto getNamedElement(const std::string &name) -> std::shared_ptr<Element> = 0;

        // Invisible element to group others. Can be positioned by anchor, in screen or in world
        // @name          - optional name to find element with getNamedElement
        // @parent        - parent element or nullptr. Top level element exists while returned pointer is held, child exists while parent exists
        // @params        - parameters (individual for each type)
        //
        virtual auto addPivot(const std::optional<std::string> &name, const std::shared_ptr<Element> &parent, ui::StageInterface::PivotParams &&params) -> std::shared_ptr<Pivot> = 0;

        // Image with texture size
        // @return        - element or nullptr if texture doesn't exist
        //
        virtual auto addImage(const std::optional<std::string> &name, const std::shared_ptr<Element> &parent, ui::StageInterface::ImageParams &&params) -> std::shared_ptr<Image> = 0;

        // Image drawn with custom shader and geometry
        // @return        - element or nullptr if shader or texture is invalid
        //
        virtual auto addImgCustom(const std::optional<std::string> &name, const std::shared_ptr<Element> &parent, ui::StageInterface::ImgCustomParams &&params) -> std::shared_ptr<ImgCustom> = 0;

        // Resizable image from 9-slice texture
        // @return        - element or nullptr if texture doesn't exist
        //
        virtual auto addImg9Slice(const std::optional<std::string> &name, const std::shared_ptr<Element> &parent, ui::StageInterface::Img9SliceParams &&params) -> std::shared_ptr<Img9Slice> = 0;

        // Single line text sized by its content
        //
        virtual auto addTextLine(const std::optional<std::string> &name, const std::shared_ptr<Element> &parent, ui::StageInterface::TextLineParams &&params) -> std::shared_ptr<TextLine> = 0;

        // Multiline text wrapped into @size and aligned by @textAlign
        //
        virtual auto addTextBlock(const std::optional<std::string> &name, const std::shared_ptr<Element> &parent, ui::StageInterface::TextBlockParams &&params) -> std::shared_ptr<TextBlock> = 0;

        // Add user-defined element. T must provide static T::make(stage, name, parent, params)
        //
        template<typename T> auto addExtensionElement(const std::optional<std::string> &name, const std::shared_ptr<Element> &parent, T &&params) {
            return T::make(*this, name, parent, std::move(params));
        }

        // Remove all elements
        //
        virtual void clear() = 0;

        // Remove released elements, update positions and draw
        //
        virtual void updateAndDraw(float dtSec) = 0;
        
    public:
        virtual ~StageInterface() = default;
    };
    
    using StageInterfacePtr = std::shared_ptr<StageInterface>;
    using ElementPtr = std::shared_ptr<StageInterface::Element>;
    using InteractorPtr = std::shared_ptr<StageInterface::Interactor>;
    using PivotPtr = std::shared_ptr<StageInterface::Pivot>;
    using ImagePtr = std::shared_ptr<StageInterface::Image>;
    using Img9SlicePtr = std::shared_ptr<StageInterface::Img9Slice>;
    using TextLinePtr = std::shared_ptr<StageInterface::TextLine>;
    using TextBlockPtr = std::shared_ptr<StageInterface::TextBlock>;
}

