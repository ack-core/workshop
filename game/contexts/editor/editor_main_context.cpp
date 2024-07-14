
#include "editor_main_context.h"

namespace game {
    namespace {
        const float MOVING_TOOL_SIZE = 5.0f;
        const math::vector3f lineDirs[] = {
            {1, 0, 0}, {0, 1, 0}, {0, 0, 1}
        };
        const math::vector4f lineColors[] = {
            {1, 0, 0, 1.0}, {0, 0.8, 0, 1.0}, {0, 0, 1, 1.0}
        };
    }
    
    MovingTool::MovingTool(const API &api, math::vector3f &target) : _api(api), _target(target) {
        _lineset = _api.scene->addLineSet();
        _lineset->setLine(0, {0, 0, 0}, MOVING_TOOL_SIZE * lineDirs[0], lineColors[0], true);
        _lineset->setLine(1, {0, 0, 0}, MOVING_TOOL_SIZE * lineDirs[1], lineColors[1], true);
        _lineset->setLine(2, {0, 0, 0}, MOVING_TOOL_SIZE * lineDirs[2], lineColors[2], true);
        _lineset->setPosition(target);
        
        _token = _api.platform->addPointerEventHandler([this](const foundation::PlatformPointerEventArgs &args) -> bool {
            _lineset->setLine(0, {0, 0, 0}, MOVING_TOOL_SIZE * lineDirs[0], lineColors[0], true);
            _lineset->setLine(1, {0, 0, 0}, MOVING_TOOL_SIZE * lineDirs[1], lineColors[1], true);
            _lineset->setLine(2, {0, 0, 0}, MOVING_TOOL_SIZE * lineDirs[2], lineColors[2], true);

            math::vector3f camPosition;
            math::vector3f cursorDir = _api.scene->getWorldDirection({args.coordinateX, args.coordinateY}, &camPosition);
            
            std::uint32_t index = _capturedLineIndex;
            float minDistance = 0.0f;
            
            if (_capturedPointerId == foundation::INVALID_POINTER_ID) {
                minDistance = std::numeric_limits<float>::max();

                for (std::uint32_t i = 0; i < 3; i++) {
                    const float dist = math::distanceRayLine(camPosition, cursorDir, _target, _target + MOVING_TOOL_SIZE * lineDirs[i]);
                    if (dist < minDistance) {
                        minDistance = dist;
                        index = i;
                    }
                }
            }
            if (minDistance < 0.5f) {
                _lineset->setLine(index, {0, 0, 0}, MOVING_TOOL_SIZE * lineDirs[index], lineColors[index] + math::vector4f(0.6, 0.6, 0.6, 0.0), true);

                if (args.type == foundation::PlatformPointerEventArgs::EventType::START) {
                    _capturedLineIndex = index;
                    _capturedPointerId = args.pointerID;
                    _capturedPosition = _target;
                    _capturedMovingKoeff = math::nearestKoeffsRayRay(camPosition, cursorDir, _target, lineDirs[_capturedLineIndex]).y;
                    return true;
                }
            }
            if (args.type == foundation::PlatformPointerEventArgs::EventType::MOVE) {
                if (_capturedPointerId == args.pointerID) {
                    const float lineMovingKoeff = math::nearestKoeffsRayRay(camPosition, cursorDir, _capturedPosition, lineDirs[_capturedLineIndex]).y;
                    _target = _capturedPosition + (lineMovingKoeff - _capturedMovingKoeff) * lineDirs[_capturedLineIndex];
                    _lineset->setPosition(_target);
                    return true;
                }
            }
            if (args.type == foundation::PlatformPointerEventArgs::EventType::FINISH || args.type == foundation::PlatformPointerEventArgs::EventType::CANCEL) {
                _capturedPointerId = foundation::INVALID_POINTER_ID;
                _lineset->setLine(_capturedLineIndex, {0, 0, 0}, MOVING_TOOL_SIZE * lineDirs[_capturedLineIndex], lineColors[_capturedLineIndex], true);
            }

            return false;
        }, true);
        
    }
    MovingTool::~MovingTool() {
        _api.platform->removeEventHandler(_token);
    }
    
    EditorMainContext::EditorMainContext(API &&api) : _api(std::move(api)) {
        _axis = _api.scene->addLineSet();
        _axis->setLine(0, {0, 0, 0}, {1000, 0, 0}, {1, 0, 0, 0.5});
        _axis->setLine(1, {0, 0, 0}, {0, 1000, 0}, {0, 1, 0, 0.5});
        _axis->setLine(2, {0, 0, 0}, {0, 0, 1000}, {0, 0, 1, 0.5});
        _axis->setLine(3, {-10, 0, -10}, {10, 0, -10}, {0.3, 0.3, 0.3, 0.5});
        _axis->setLine(4, {10, 0, -10}, {10, 0, 10}, {0.3, 0.3, 0.3, 0.5});
        _axis->setLine(5, {10, 0, 10}, {-10, 0, 10}, {0.3, 0.3, 0.3, 0.5});
        _axis->setLine(6, {-10, 0, 10}, {-10, 0, -10}, {0.3, 0.3, 0.3, 0.5});
        
        //_movingTool = std::make_unique<MovingTool>(_api, _target);
        
        _testButton1 = _api.ui->addImage(nullptr, ui::StageInterface::ImageParams {
            .anchorOffset = math::vector2f(10.0f, 10.0f),
            .anchorH = ui::HorizontalAnchor::LEFT,
            .anchorV = ui::VerticalAnchor::TOP,
            .textureBase = "textures/ui/btn_square_up",
            .textureAction = "textures/ui/btn_square_down",
        });
        _testButton1->setActionHandler(ui::Action::PRESS, [](float, float) {
            
        });
        
        _testButton2 = _api.ui->addImage(nullptr, ui::StageInterface::ImageParams {
            .anchorTarget = _testButton1,
            .anchorOffset = math::vector2f(10.0f, 0.0f),
            .anchorH = ui::HorizontalAnchor::RIGHTSIDE,
            .anchorV = ui::VerticalAnchor::TOP,
            .textureBase = "textures/ui/btn_square_up",
            .textureAction = "textures/ui/btn_square_down",
        });
        _testButton2->setActionHandler(ui::Action::PRESS, [](float, float) {
        
        });
        
    }
    
    EditorMainContext::~EditorMainContext() {

    }
    
    EditorNode *EditorMainContext::getNode(const std::string &name) const {
        return nullptr;
    }
    
    void EditorMainContext::update(float dtSec) {
        
    }
}

