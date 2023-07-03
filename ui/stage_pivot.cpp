
#include "stage.h"
#include "stage_base.h"
#include "stage_pivot.h"

namespace ui {
    PivotImpl::PivotImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent) : ElementImpl(facility, parent) {
        _state = CoordinateState::ANCHOR;
    }
    PivotImpl::~PivotImpl() {
    
    }
    
    void PivotImpl::updateCoordinates() {
        if (_state == CoordinateState::ANCHOR) {
            ElementImpl::updateCoordinates();
        }
        else {
            if (_state == CoordinateState::SCREEN) {
                _globalPosition = _screenPosition;
            }
            else if (_state == CoordinateState::WORLD) {
                math::transform3f view, proj;
                math::vector3f camPos, camDir;
                
                _facility.getRendering()->getFrameConstants(view.flat16, proj.flat16, camPos.flat3, camDir.flat3);
                
                math::transform3f vp = view * proj;
                math::vector4f tpos = math::vector4f(_worldPosition, 1.0f);

                tpos = tpos.transformed(vp);
                tpos.x /= tpos.w;
                tpos.y /= tpos.w;
                tpos.z /= tpos.w;
                
                if (tpos.z > 0.0f) {
                    _globalPosition.x = (tpos.x + 1.0f) * 0.5f * _facility.getRendering()->getBackBufferWidth();
                    _globalPosition.y = (1.0f - tpos.y) * 0.5f * _facility.getRendering()->getBackBufferHeight();
                }
                else {
                    _globalPosition = math::vector2f(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
                }
            }
            
            for (auto &item : _attachedElements) {
                item->updateCoordinates();
            }
        }
    }

    void PivotImpl::setScreenPosition(const math::vector2f &position) {
        _state = CoordinateState::SCREEN;
        _screenPosition = position;
    }
    
    void PivotImpl::setWorldPosition(const math::vector3f &position) {
        _state = CoordinateState::WORLD;
        _worldPosition = position;
    }
    
    bool PivotImpl::onInteraction(ui::Action action, std::size_t id, float x, float y) {
        return ElementImpl::onInteraction(action, id, x, y);
    }
    
    void PivotImpl::draw(ShaderConstants &constStorage) {
        ElementImpl::draw(constStorage);
    }
    
}
