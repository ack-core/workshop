
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
            util::callback<void(const math::vector2f &direction)> onChange;

            static auto make(StageInterface &stage, const std::shared_ptr<StageInterface::Element> &parent, JoystickParams &&params) -> std::shared_ptr<StageInterface::Element> {
                std::shared_ptr<StageInterface::Image> bg = nullptr;
                const resource::ResourceProviderPtr res = stage.getResourceProvider();
                float maxOffset = params.maxThumbOffset;
                
                if (const resource::TextureInfo *info = res->getTextureInfo(params.textureBackground)) {
                    bg = stage.addImage(nullptr, StageInterface::ImageParams {
                        .anchorTarget = params.anchorTarget,
                        .anchorH = params.anchorH,
                        .anchorV = params.anchorV,
                        .anchorOffset = params.anchorOffset,
                        .texture = params.textureBackground,
                        .activeAreaOffset = 0.5f * info->width,
                        .activeAreaRadius = 0.75f * info->width
                    });
                    auto pivot = stage.addPivot(bg, StageInterface::PivotParams {
                        .anchorH = HorizontalAnchor::CENTER,
                        .anchorV = VerticalAnchor::MIDDLE,
                    });
                    auto thumb = stage.addImage(pivot, StageInterface::ImageParams {
                        .anchorH = HorizontalAnchor::CENTER,
                        .anchorV = VerticalAnchor::MIDDLE,
                        .texture = params.textureThumb,
                    });
                    
                    std::weak_ptr<StageInterface::Image> weakBg = bg;
                    std::weak_ptr<StageInterface::Pivot> weakPivot = pivot;
                    
                    bg->setActionHandler([onChange = std::move(params.onChange), maxOffset, weakBg, weakPivot](Action action, float x, float y) {
                        if (auto bg = weakBg.lock()) {
                            if (auto pivot = weakPivot.lock()) {
                                if (action == Action::RELEASE) {
                                    pivot->setScreenPosition(bg->getPosition() + 0.5f * bg->getSize());
                                    if (onChange) {
                                        onChange(math::vector2f(0, 0));
                                    }
                                }
                                else {
                                    const math::vector2f center = bg->getPosition() + 0.5f * bg->getSize();
                                    math::vector2f dir = (math::vector2f(x, y) - center);
                                    
                                    if (dir.length() > maxOffset) {
                                        dir = dir.normalized(maxOffset);
                                    }
                                    
                                    pivot->setScreenPosition(center + dir);
                                    if (onChange) {
                                        onChange(math::vector2f(dir.x, -dir.y) / maxOffset);
                                    }
                                }
                            }
                        }
                    });
                }
                
                return bg;
            }
        };

        //---

        struct Button9SliceParams {
            const std::shared_ptr<StageInterface::Element> anchorTarget;
            const HorizontalAnchor anchorH = HorizontalAnchor::LEFT;
            const VerticalAnchor anchorV = VerticalAnchor::TOP;
            const math::vector2f anchorOffset = math::vector2f(0, 0);
            const math::vector2f size = math::vector2f(100.0f, 100.0f);
            const char *textureBase = "";
            const char *textureOver = "";
            const char *texturePress = "";
            const math::vector3f sliceArgs = 0.0f;
            const float activeAreaOffset = 0.0f;
            const float activeAreaRadius = 0.0f;
            util::callback<void()> onPress;
            
            static auto make(StageInterface &stage, const std::shared_ptr<StageInterface::Element> &parent, Button9SliceParams &&params) -> std::shared_ptr<StageInterface::Element> {
                const resource::ResourceProviderPtr res = stage.getResourceProvider();
                std::shared_ptr<StageInterface::Img9Slice> base = stage.addImg9Slice(nullptr, ui::StageInterface::Img9SliceParams {
                    .anchorH = params.anchorH,
                    .anchorV = params.anchorV,
                    .anchorOffset = params.anchorOffset,
                    .size = params.size,
                    .texture = params.textureBase,
                    .sliceArgs = params.sliceArgs
                });
                
                bool texturesFound = false;
                if (const resource::TextureInfo *baseInfo = res->getTextureInfo(params.textureBase)) {
                    if (const resource::TextureInfo *overInfo = res->getTextureInfo(params.textureOver)) {
                        if (const resource::TextureInfo *pressInfo = res->getTextureInfo(params.texturePress)) {
                            texturesFound = true;
                        }
                    }
                }

                if (texturesFound) {
                    std::weak_ptr<StageInterface::Img9Slice> weakBase = base;
                    std::string overPath = std::string(params.textureOver);
                    std::string pressPath = std::string(params.texturePress);

                    auto baseLoaded = [weakBase, res, overPath, pressPath, onPress = std::move(params.onPress), sliceArgs = params.sliceArgs](const foundation::RenderTexturePtr &txBase) mutable {
                        auto overLoaded = [weakBase, res, overPath, pressPath, onPress = std::move(onPress), txBase, sliceArgs](const foundation::RenderTexturePtr &txOver) mutable {
                            auto pressLoaded = [weakBase, txBase, txOver, onPress = std::move(onPress), sliceArgs](const foundation::RenderTexturePtr &txPress) mutable {
                                if (auto base = weakBase.lock()) {
                                    base->setActionHandler([onPress = std::move(onPress), weakBase, txBase, txOver, txPress, sliceArgs](Action action, float x, float y) {
                                        if (auto base = weakBase.lock()) {
                                            if (action == Action::PRESS) {
                                                base->setTexture(txPress, sliceArgs);
                                            }
                                            if (action == Action::RELEASE) {
                                                base->setTexture(txBase, sliceArgs);
                                                if (onPress) {
                                                    onPress();
                                                }
                                            }
                                        }
                                    });
                                }
                            };

                            res->getOrLoadTexture(pressPath.data(), std::move(pressLoaded));
                        };
                        res->getOrLoadTexture(overPath.data(), std::move(overLoaded));
                    };
                    res->getOrLoadTexture(params.textureBase, std::move(baseLoaded));
                }
                
                return base;
            }
        };
    }
}

