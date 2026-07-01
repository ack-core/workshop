
// TODO: dpi-scaling

#pragma once
#include "ui/stage.h"
#include "foundation/util.h"
#include "foundation/math.h"

#include <memory>
#include <functional>

namespace ui {
    namespace extensions {
        struct JoystickParams {
            const std::shared_ptr<StageInterface::Element> anchorTarget;
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
            const math::vector2f anchorOffset = math::vector2f(0, 0);
            const char *textureBackground = "";
            const char *textureThumb = "";
            const float maxThumbOffset = 50.0f;
            util::callback<void(const math::vector2f &direction)> handler;
        };
            
        auto addJoystick(const StageInterfacePtr &stage, const resource::ResourceProviderPtr &res, const std::shared_ptr<StageInterface::Element> &parent, JoystickParams &&params) -> std::shared_ptr<StageInterface::Element> {
            std::shared_ptr<StageInterface::Image> bg = nullptr;
            float maxOffset = params.maxThumbOffset;
            
            if (const resource::TextureInfo *info = res->getTextureInfo(params.textureBackground)) {
                bg = stage->addImage(nullptr, StageInterface::ImageParams {
                    .anchorTarget = params.anchorTarget,
                    .anchorOffset = params.anchorOffset,
                    .anchorH = params.anchorH,
                    .anchorV = params.anchorV,
                    .texture = params.textureBackground,
                    .activeAreaOffset = 0.5f * info->width,
                    .activeAreaRadius = 0.75f * info->width
                });
                auto pivot = stage->addPivot(bg, StageInterface::PivotParams {
                    .anchorH = HorizontalAnchor::CENTER,
                    .anchorV = VerticalAnchor::MIDDLE,
                });
                auto thumb = stage->addImage(pivot, StageInterface::ImageParams {
                    .anchorH = HorizontalAnchor::CENTER,
                    .anchorV = VerticalAnchor::MIDDLE,
                    .texture = params.textureThumb,
                });
                
                std::weak_ptr<StageInterface::Image> weakBg = bg;
                std::weak_ptr<StageInterface::Pivot> weakPivot = pivot;
                
                bg->setActionHandler([handler = std::move(params.handler), maxOffset, weakBg, weakPivot](Action action, float x, float y) {
                    if (auto bg = weakBg.lock()) {
                        if (auto pivot = weakPivot.lock()) {
                            if (action == Action::RELEASE) {
                                pivot->setScreenPosition(bg->getPosition() + 0.5f * bg->getSize());
                                if (handler) {
                                    handler(math::vector2f(0, 0));
                                }
                            }
                            else {
                                const math::vector2f center = bg->getPosition() + 0.5f * bg->getSize();
                                math::vector2f dir = (math::vector2f(x, y) - center);
                                
                                if (dir.length() > maxOffset) {
                                    dir = dir.normalized(maxOffset);
                                }
                                
                                pivot->setScreenPosition(center + dir);
                                if (handler) {
                                    handler(math::vector2f(dir.x, -dir.y) / maxOffset);
                                }
                            }
                        }
                    }
                });
            }
            
            return bg;
        }
    }
}

