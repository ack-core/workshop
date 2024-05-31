
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
            const math::vector2f anchorOffset = math::vector2f(0, 0);
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
            const char *textureBackground = "";
            const char *textureThumb = "";
            const float maxThumbOffset = 50.0f;
            util::callback<void(const math::vector2f &direction)> handler;
        };
            
        auto addJoystick(const StageInterfacePtr &stage, const std::shared_ptr<Element> &parent, ui::StageInterface::JoystickParams &&params) -> std::shared_ptr<StageInterface::Element> {
            std::shared_ptr<StageInterface::Image> bg = nullptr;
            float maxOffset = params.maxThumbOffset;
            
            if (const resource::TextureInfo *info = _textureProvider->getTextureInfo(params.textureBackground)) {
                bg = stage->addImage(nullptr, ui::StageInterface::ImageParams {
                    .anchorTarget = params.anchorTarget,
                    .anchorOffset = params.anchorOffset,
                    .anchorH = params.anchorH,
                    .anchorV = params.anchorV,
                    .textureBase = params.textureBackground,
                    .activeAreaOffset = 0.5f * info->width,
                    .activeAreaRadius = 0.75f * info->width,
                    .capturePointer = true,
                });
                auto pivot = stage->addPivot(bg, PivotParams {
                    .anchorH = HorizontalAnchor::CENTER,
                    .anchorV = VerticalAnchor::MIDDLE,
                });
                auto thumb = stage->addImage(pivot, ui::StageInterface::ImageParams {
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
            
            return bg;
        }
    }
}

