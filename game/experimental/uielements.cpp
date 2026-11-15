
#include "uielements.h"
#include "ui/extensions.h"

namespace game {
    UIElementsContext::UIElementsContext(API &&api) : _api(std::move(api)) {
        _img = _api.ui->addImg9Slice(std::nullopt, nullptr, ui::StageInterface::Img9SliceParams {
            .anchorH = ui::HorizontalAnchor::CENTER,
            .anchorV = ui::VerticalAnchor::TOP,
            .anchorOffset = math::vector2f(0.0f, 50.0f),
            .size = math::vector2f(600.0f, 150.0f),
            .texture = "textures/ui/img9slice",
            .sliceArgs = math::vector3f(104.0f, 104.0f, 12.0f),
            .fill = false
        });
        _button = _api.ui->addExtensionElement(std::nullopt, nullptr, ui::extensions::Button9SliceParams {
            .anchorH = ui::HorizontalAnchor::LEFT,
            .anchorV = ui::VerticalAnchor::TOP,
            .anchorOffset = math::vector2f(50.0f, 50.0f),
            .size = math::vector2f(80.0f, 80.0f),
            .textureBase = "textures/ui/btn_slice_base",
            .textureOver = "textures/ui/btn_slice_press",
            .texturePress = "textures/ui/btn_slice_press",
            .sliceArgs = math::vector3f(96.0f, 96.0f, 16.0f)
        });
        _joystick = _api.ui->addExtensionElement(std::nullopt, nullptr, ui::extensions::JoystickParams {
            .anchorH = ui::HorizontalAnchor::RIGHT,
            .anchorV = ui::VerticalAnchor::BOTTOM,
            .anchorOffset = math::vector2f(50.0f, 50.0f),
            .textureBackground = "textures/ui/joystick_bg_00",
            .textureThumb = "textures/ui/joystick_thumb",
            .maxThumbOffset = 100.0f,
            .onChange = [this](const math::vector2f &direction) {
                //printf("%f %f\n", direction.x, direction.y);
            }
        });
    }
}
