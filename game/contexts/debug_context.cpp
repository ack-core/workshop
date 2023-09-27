
#include "debug_context.h"

namespace game {
    template <> std::unique_ptr<Context> makeContext<DebugContext>(API &&api) {
        return std::make_unique<DebugContext>(std::move(api));
    }
    
    DebugContext::DebugContext(API &&api) : _api(std::move(api)) {
        _api.yard->setCameraLookAt(_center + _orbit, _center);
        _touchEventHandlerToken = _api.platform->addPointerEventHandler(
            [this](const foundation::PlatformPointerEventArgs &args) -> bool {
                if (args.type == foundation::PlatformPointerEventArgs::EventType::START) {
                    _pointerId = args.pointerID;
                    _lockedCoordinates = { args.coordinateX, args.coordinateY };
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::MOVE) {
                    if (_pointerId != foundation::INVALID_POINTER_ID) {
                        float dx = args.coordinateX - _lockedCoordinates.x;
                        float dy = args.coordinateY - _lockedCoordinates.y;
                        
                        _orbit.xz = _orbit.xz.rotated(dx / 100.0f);
                        
                        math::vector3f right = math::vector3f(0, 1, 0).cross(_orbit);
                        math::vector3f rotatedOrbit = _orbit.rotated(right, dy / 100.0f);
                        
                        if (fabs(math::vector3f(0, 1, 0).dot(rotatedOrbit.normalized())) < 0.96f) {
                            _orbit = rotatedOrbit;
                        }
                        
                        _api.yard->setCameraLookAt(_center + _orbit, _center);
                        _lockedCoordinates = { args.coordinateX, args.coordinateY };
                    }
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::FINISH) {
                    _pointerId = foundation::INVALID_POINTER_ID;
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::CANCEL) {
                    _pointerId = foundation::INVALID_POINTER_ID;
                }
                
                return true;
            }
        );
        
        _api.yard->loadYard("debug", [this](bool loaded){
            printf("%s\n", loaded ? "loaded" : "not loaded");
        });
        
        auto panel = _api.ui->addImage(nullptr, ui::StageInterface::ImageParams {
            .anchorOffset = math::vector2f(50.0f, 50.0f),
            .anchorH = ui::HorizontalAnchor::RIGHT,
            .anchorV = ui::VerticalAnchor::BOTTOM,
            .capturePointer = true,
            .textureBase = "textures/ui/panel",
        });
        auto btn1 = _api.ui->addImage(panel, ui::StageInterface::ImageParams {
            .anchorOffset = math::vector2f(30.0f, 50.0f),
            .anchorH = ui::HorizontalAnchor::RIGHT,
            .anchorV = ui::VerticalAnchor::TOP,
            .textureBase = "textures/ui/button_up",
            .textureAction = "textures/ui/button_down",
        });
        auto btn2 = _api.ui->addImage(panel, ui::StageInterface::ImageParams {
            .anchorTarget = btn1,
            .anchorOffset = math::vector2f(0.0f, 10.0f),
            .anchorH = ui::HorizontalAnchor::CENTER,
            .anchorV = ui::VerticalAnchor::BOTTOMSIDE,
            .textureBase = "textures/ui/button_up",
            .textureAction = "textures/ui/button_down",
        });
        auto txt1 = _api.ui->addText(nullptr, ui::StageInterface::TextParams {
            .anchorTarget = nullptr,
            .anchorOffset = math::vector2f(0.0f, 0.0f),
            .anchorH = ui::HorizontalAnchor::LEFT,
            .anchorV = ui::VerticalAnchor::MIDDLE,
            .fontSize = 36,
            .shadowEnabled = true,
            .shadowOffset = math::vector2f(2.0f, 2.0f)
        });

        btn1->setActionHandler(ui::Action::CAPTURE, [](float, float) {

        });
        btn2->setActionHandler(ui::Action::CAPTURE, [](float, float) {

        });
        txt1->setText("Gætirðu skrifað þetta upp?"); //

        auto pivot1 = _api.ui->addPivot(nullptr, {});
        pivot1->setWorldPosition(math::vector3f(0, 10, 0));

        auto btn3 = _api.ui->addImage(pivot1, ui::StageInterface::ImageParams {
            .anchorOffset = math::vector2f(0.0f, 0.0f),
            .anchorH = ui::HorizontalAnchor::CENTER,
            .anchorV = ui::VerticalAnchor::BOTTOM,
            .textureBase = "textures/ui/button_up",
            .textureAction = "textures/ui/button_down",
        });
        auto txt2 = _api.ui->addText(btn3, ui::StageInterface::TextParams {
            .anchorOffset = math::vector2f(0.0f, -2.0f),
            .anchorH = ui::HorizontalAnchor::CENTER,
            .anchorV = ui::VerticalAnchor::MIDDLE,
            .shadowEnabled = true,
            .shadowOffset = math::vector2f(1.0f, 2.0f),
            .fontSize = 26
        });
        txt2->setText("Play!");

        btn3->setActionHandler(ui::Action::CAPTURE, [](float, float) {

        });

        auto joystick = _api.ui->addJoystick(nullptr, ui::StageInterface::JoystickParams {
            .anchorOffset = math::vector2f(50.0f, 50.0f),
            .anchorH = ui::HorizontalAnchor::LEFT,
            .anchorV = ui::VerticalAnchor::BOTTOM,
            .textureBackground = "textures/ui/joystick_bg",
            .textureThumb = "textures/ui/joystick_thumb",
            .maxThumbOffset = 80.0f,
            .handler = [](const math::vector2f &direction) {
                printf("--->>> %2.2f %2.2f\n", direction.x, direction.y);
            }
        });
        
        _api.yard->setCameraLookAt(_center + _orbit, _center);
        

        
    }
    
    DebugContext::~DebugContext() {
        _api.platform->removeEventHandler(_touchEventHandlerToken);
    }
    
    void DebugContext::update(float dtSec) {
    
    }
}
