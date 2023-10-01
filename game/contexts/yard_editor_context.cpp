
#include "yard_editor_context.h"

namespace game {
    static const float DISTANCE_MULTIPLIER = 0.02f;
    static const float MOVING_TOOL_SCALE = 0.18f;
    
    YardEditorContext::MovingTool::MovingTool(const API &api, float distance) : _api(api) {
        _axisLength = distance * MOVING_TOOL_SCALE;
        _lineset = _api.scene->addLineSet(false, 3);
    }
    
    void YardEditorContext::MovingTool::initialize() {
        _pivotX = _api.ui->addPivot(nullptr, ui::StageInterface::PivotParams {});
        _pivotY = _api.ui->addPivot(nullptr, ui::StageInterface::PivotParams {});
        _pivotZ = _api.ui->addPivot(nullptr, ui::StageInterface::PivotParams {});
    
        _tipX = _api.ui->addImage(_pivotX, ui::StageInterface::ImageParams {
            .anchorH = ui::HorizontalAnchor::CENTER,
            .anchorV = ui::VerticalAnchor::MIDDLE,
            .textureBase = "textures/ui/btn_circle_up",
            .textureAction = "textures/ui/btn_circle_down",
        });
        _tipY = _api.ui->addImage(_pivotY, ui::StageInterface::ImageParams {
            .anchorH = ui::HorizontalAnchor::CENTER,
            .anchorV = ui::VerticalAnchor::MIDDLE,
            .textureBase = "textures/ui/btn_circle_up",
            .textureAction = "textures/ui/btn_circle_down",
        });
        _tipZ = _api.ui->addImage(_pivotZ, ui::StageInterface::ImageParams {
            .anchorH = ui::HorizontalAnchor::CENTER,
            .anchorV = ui::VerticalAnchor::MIDDLE,
            .textureBase = "textures/ui/btn_circle_up",
            .textureAction = "textures/ui/btn_circle_down",
        });
        
        _tipX->setColor({1.0f, 0.0f, 0.0f, 1.0f});
        _tipY->setColor({0.0f, 1.0f, 0.0f, 1.0f});
        _tipZ->setColor({0.0f, 0.0f, 1.0f, 1.0f});
        
        _tipX->setActionHandler(ui::Action::CAPTURE, [this](float x, float y) {
            const math::vector3f wdir = _api.scene->getWorldDirection(math::vector2f(x, y));
            _capturedPosition = _position;
            _capturedPlaneNormal = std::fabs(wdir.y) > std::fabs(wdir.z) ? math::vector3f(0, 1, 0) : math::vector3f(0, 0, 1);
            math::intersectRayPlane(_camPos, wdir, _position, _capturedPlaneNormal, _capturedIntersectPos);
        });
        _tipX->setActionHandler(ui::Action::MOVE, [this](float x, float y) {
            const math::vector3f wdir = _api.scene->getWorldDirection(math::vector2f(x, y));
            math::vector3f intersectPos = {0, 0, 0};
            
            if (math::intersectRayPlane(_camPos, wdir, _position, _capturedPlaneNormal, intersectPos)) {
                _position = _capturedPosition + math::vector3f((intersectPos - _capturedIntersectPos).dot(math::vector3f(1, 0, 0)), 0.0f, 0.0f);
            }
        });
        _tipY->setActionHandler(ui::Action::CAPTURE, [this](float x, float y) {
            const math::vector3f wdir = _api.scene->getWorldDirection(math::vector2f(x, y));
            _capturedPosition = _position;
            _capturedPlaneNormal = std::fabs(wdir.x) > std::fabs(wdir.z) ? math::vector3f(1, 0, 0) : math::vector3f(0, 0, 1);
            math::intersectRayPlane(_camPos, wdir, _position, _capturedPlaneNormal, _capturedIntersectPos);
        });
        _tipY->setActionHandler(ui::Action::MOVE, [this](float x, float y) {
            const math::vector3f wdir = _api.scene->getWorldDirection(math::vector2f(x, y));
            math::vector3f intersectPos = {0, 0, 0};
            
            if (math::intersectRayPlane(_camPos, wdir, _position, _capturedPlaneNormal, intersectPos)) {
                _position = _capturedPosition + math::vector3f(0.0f, (intersectPos - _capturedIntersectPos).dot(math::vector3f(0, 1, 0)), 0.0f);
            }
        });
        _tipZ->setActionHandler(ui::Action::CAPTURE, [this](float x, float y) {
            const math::vector3f wdir = _api.scene->getWorldDirection(math::vector2f(x, y));
            _capturedPosition = _position;
            _capturedPlaneNormal = std::fabs(wdir.x) > std::fabs(wdir.y) ? math::vector3f(1, 0, 0) : math::vector3f(0, 1, 0);
            math::intersectRayPlane(_camPos, wdir, _position, _capturedPlaneNormal, _capturedIntersectPos);
        });
        _tipZ->setActionHandler(ui::Action::MOVE, [this](float x, float y) {
            const math::vector3f wdir = _api.scene->getWorldDirection(math::vector2f(x, y));
            math::vector3f intersectPos = {0, 0, 0};
            
            if (math::intersectRayPlane(_camPos, wdir, _position, _capturedPlaneNormal, intersectPos)) {
                _position = _capturedPosition + math::vector3f(0.0f, 0.0f, (intersectPos - _capturedIntersectPos).dot(math::vector3f(0, 0, 1)));
            }
        });
    }

    YardEditorContext::MovingTool::~MovingTool() {
    
    }
    
    void YardEditorContext::MovingTool::MovingTool::update(const math::vector3f &camPos, float distance) {
        _camPos = camPos;
        _axisLength = distance * MOVING_TOOL_SCALE;
        
        _lineset->setPosition(_position);
        _lineset->setLine(0, {}, {_axisLength, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f});
        _lineset->setLine(1, {}, {0.0f, _axisLength, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f});
        _lineset->setLine(2, {}, {0.0f, 0.0f, _axisLength}, {0.0f, 0.0f, 1.0f, 1.0f});

        _pivotX->setWorldPosition(_position + math::vector3f(_axisLength, 0.0f, 0.0f));
        _pivotY->setWorldPosition(_position + math::vector3f(0.0f, _axisLength, 0.0f));
        _pivotZ->setWorldPosition(_position + math::vector3f(0.0f, 0.0f, _axisLength));
    }
    
    //---
    
    template <> std::unique_ptr<Context> makeContext<YardEditorContext>(API &&api) {
        return std::make_unique<YardEditorContext>(std::move(api));
    }
    
    YardEditorContext::YardEditorContext(API &&api) : _api(std::move(api)), _movingTool(_api, _orbit.length()) {
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
                        math::vector3f right = math::vector3f(0, 1, 0).cross(_orbit).normalized();
                        math::vector3f rotatedOrbit = _orbit.rotated(right, dy / 100.0f);

                        if (fabs(math::vector3f(0, 1, 0).dot(rotatedOrbit.normalized())) < 0.96f) {
                            _orbit = rotatedOrbit;
                        }

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
        
        _joystickElement = _api.ui->addJoystick(nullptr, ui::StageInterface::JoystickParams {
            .anchorOffset = math::vector2f(0.0f, 0.0f),
            .anchorH = ui::HorizontalAnchor::RIGHT,
            .anchorV = ui::VerticalAnchor::BOTTOM,
            .textureBackground = "textures/ui/joystick_bg_00",
            .textureThumb = "textures/ui/joystick_st_00",
            .maxThumbOffset = 80.0f,
            .handler = [this](const math::vector2f &direction) {
                _joystickMovement = -1.0f * _orbit.xz.normalized() * direction.y + math::vector2f(_orbit.z, -_orbit.x).normalized() * direction.x;
            }
        });
        _zoomOut = _api.ui->addImage(nullptr, ui::StageInterface::ImageParams {
            .anchorTarget = _joystickElement,
            .anchorOffset = math::vector2f(0.0f, 30.0f),
            .anchorH = ui::HorizontalAnchor::RIGHT,
            .anchorV = ui::VerticalAnchor::TOPSIDE,
            .textureBase = "textures/ui/btn_square_up",
            .textureAction = "textures/ui/btn_square_down",
        });
        _zoomIn = _api.ui->addImage(nullptr, ui::StageInterface::ImageParams {
            .anchorTarget = _zoomOut,
            .anchorOffset = math::vector2f(0.0f, 10.0f),
            .anchorH = ui::HorizontalAnchor::RIGHT,
            .anchorV = ui::VerticalAnchor::TOPSIDE,
            .textureBase = "textures/ui/btn_square_up",
            .textureAction = "textures/ui/btn_square_down",
        });
        
        _zoomIn->setActionHandler(ui::Action::CAPTURE, [this](float, float) {
            _distanceMultiplier = -DISTANCE_MULTIPLIER;
        });
        _zoomIn->setActionHandler(ui::Action::RELEASE, [this](float, float) {
            _distanceMultiplier = 0.0f;
        });
        _zoomOut->setActionHandler(ui::Action::CAPTURE, [this](float, float) {
            _distanceMultiplier = DISTANCE_MULTIPLIER;
        });
        _zoomOut->setActionHandler(ui::Action::RELEASE, [this](float, float) {
            _distanceMultiplier = 0.0f;
        });

        _movingTool.initialize();
        _quad = _api.scene->addLineSet(false, 8);
        _quad->addLine({-16.0f, 0.0f, -16.0f}, {16.0f, 0.0f, -16.0f}, {0.25f, 0.25f, 0.25f, 1.0f});
        _quad->addLine({-16.0f, 0.0f, -16.0f}, {-16.0f, 0.0f, 16.0f}, {0.25f, 0.25f, 0.25f, 1.0f});
        _quad->addLine({16.0f, 0.0f, 16.0f}, {16.0f, 0.0f, -16.0f}, {0.25f, 0.25f, 0.25f, 1.0f});
        _quad->addLine({16.0f, 0.0f, 16.0f}, {-16.0f, 0.0f, 16.0f}, {0.25f, 0.25f, 0.25f, 1.0f});
        _quad->addLine({0.0f, 0.0f, 0.0f}, {32.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f});
        _quad->addLine({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 32.0f}, {0.5f, 0.5f, 0.5f, 1.0f});
        _quad->addLine({0.0f, 0.0f, 0.0f}, {-32.0f, 0.0f, 0.0f}, {0.25f, 0.25f, 0.25f, 1.0f});
        _quad->addLine({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -32.0f}, {0.25f, 0.25f, 0.25f, 1.0f});
        
        _api.yard->setCameraLookAt(_center + _orbit, _center);
    }
    
    YardEditorContext::~YardEditorContext() {
        _api.platform->removeEventHandler(_touchEventHandlerToken);
    }
    
    void YardEditorContext::update(float dtSec) {
        _center.xz = _center.xz + _joystickMovement * 100.0f * dtSec;
        
        if (_orbit.length() > 40.0f && _distanceMultiplier < 0.0f) {
            _orbit = _orbit * (1.0f + _distanceMultiplier);
        }
        if (_orbit.length() < 1000.0f && _distanceMultiplier > 0.0f) {
            _orbit = _orbit * (1.0f + _distanceMultiplier);
        }

        _api.yard->setCameraLookAt(_center + _orbit, _center);
        _quad->setPosition(_center);
        _movingTool.update(_center + _orbit, _orbit.length());
    }
}
