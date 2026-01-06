
#include "editor_moving_tool.h"

namespace game {
    namespace {
        const math::vector3f lineDirs[] = {
            {1, 0, 0}, {0, 1, 0}, {0, 0, 1}
        };
        const math::vector4f lineColors[] = {
            {1, 0, 0, 1.0}, {0, 0.8, 0, 1.0}, {0, 0, 1, 1.0}
        };
    }
    
    MovingTool::MovingTool(const API &api, const CameraAccessInterface &cameraAccess, math::vector3f &target, float size)
    : _api(api)
    , _cameraAccess(cameraAccess)
    , _target(target)
    , _toolSize(size)
    {
        const float currentToolSize = _cameraAccess.getOrbitSize() / 10.0f * _toolSize;
        _lineset = _api.scene->addLineSet();
        _lineset->setLine(0, {0, 0, 0}, currentToolSize * lineDirs[0], lineColors[0], true);
        _lineset->setLine(1, {0, 0, 0}, currentToolSize * lineDirs[1], lineColors[1], true);
        _lineset->setLine(2, {0, 0, 0}, currentToolSize * lineDirs[2], lineColors[2], true);
        _lineset->setPosition(target);
        
        _token = _api.platform->addPointerEventHandler([this](const foundation::PlatformPointerEventArgs &args) -> bool {
            const float currentToolSize = _cameraAccess.getOrbitSize() / 10.0f * _toolSize;
            _lineset->setLine(0, {0, 0, 0}, currentToolSize * lineDirs[0], lineColors[0], true);
            _lineset->setLine(1, {0, 0, 0}, currentToolSize * lineDirs[1], lineColors[1], true);
            _lineset->setLine(2, {0, 0, 0}, currentToolSize * lineDirs[2], lineColors[2], true);

            math::vector3f camPosition;
            math::vector3f cursorDir = _api.scene->getWorldDirection({args.coordinateX, args.coordinateY}, &camPosition);
            
            std::uint32_t index = _capturedLineIndex;
            float minDistance = 0.0f;
            
            if (_capturedPointerId == foundation::INVALID_POINTER_ID) {
                minDistance = std::numeric_limits<float>::max();

                for (std::uint32_t i = 0; i < 3; i++) {
                    const float dist = math::distanceRayLine(camPosition, cursorDir, _base + _target, _base + _target + currentToolSize * lineDirs[i]);
                    if (dist < minDistance) {
                        minDistance = dist;
                        index = i;
                    }
                }
            }
            if (minDistance < 0.1f * currentToolSize) {
                _lineset->setLine(index, {0, 0, 0}, currentToolSize * lineDirs[index], lineColors[index] + math::vector4f(0.6, 0.6, 0.6, 0.0), true);

                if (args.type == foundation::PlatformPointerEventArgs::EventType::START) {
                    _capturedLineIndex = index;
                    _capturedPointerId = args.pointerID;
                    _capturedPosition = _target;
                    _capturedMovingKoeff = math::nearestKoeffsRayRay(camPosition, cursorDir, _base + _target, lineDirs[_capturedLineIndex]).y;
                    return true;
                }
            }
            if (args.type == foundation::PlatformPointerEventArgs::EventType::MOVE) {
                if (_capturedPointerId == args.pointerID) {
                    const float lineMovingKoeff = math::nearestKoeffsRayRay(camPosition, cursorDir, _base + _capturedPosition, lineDirs[_capturedLineIndex]).y;
                    _target = _capturedPosition + (lineMovingKoeff - _capturedMovingKoeff) * lineDirs[_capturedLineIndex];
                    
                    if (_useGrid) {
                        _target.x = std::floor(_target.x);
                        _target.y = std::floor(_target.y);
                        _target.z = std::floor(_target.z);
                    }
                    
                    _lineset->setPosition(_base + _target);
                    
                    if (_editorMsg.size()) {
                        _api.platform->sendEditorMsg(_editorMsg, util::strstream::ftos(_target.x) + " " + util::strstream::ftos(_target.y) + " " + util::strstream::ftos(_target.z));
                    }
                    
                    return true;
                }
            }
            if (args.type == foundation::PlatformPointerEventArgs::EventType::FINISH || args.type == foundation::PlatformPointerEventArgs::EventType::CANCEL) {
                if (_onDragEnd && _capturedPointerId != foundation::INVALID_POINTER_ID) {
                    _onDragEnd();
                }
                
                _capturedPointerId = foundation::INVALID_POINTER_ID;
                _lineset->setLine(_capturedLineIndex, {0, 0, 0}, currentToolSize * lineDirs[_capturedLineIndex], lineColors[_capturedLineIndex], true);
            }

            return false;
        }, true);
    }
    MovingTool::~MovingTool() {
        _api.platform->removeEventHandler(_token);
    }
    
    void MovingTool::setPosition(const math::vector3f &position) {
        _target = position;
        _lineset->setPosition(_base + _target);
    }
    void MovingTool::setUseGrid(bool useGrid) {
        _useGrid = useGrid;
    }
    void MovingTool::setEditorMsg(const char *editorMsg) {
        _editorMsg = editorMsg;
        _api.platform->sendEditorMsg(_editorMsg, util::strstream::ftos(_target.x) + " " + util::strstream::ftos(_target.y) + " " + util::strstream::ftos(_target.z));
    }
    void MovingTool::setBase(const math::vector3f &base) {
        _base = base;
        _lineset->setPosition(_base + _target);
    }
    void MovingTool::setOnDragEnd(util::callback<void()> &&handler) {
        _onDragEnd = std::move(handler);
    }
    const math::vector3f &MovingTool::getPosition() const {
        return _target;
    }
    bool MovingTool::isDragging() const {
        return _capturedPointerId != foundation::INVALID_POINTER_ID;
    }
}

