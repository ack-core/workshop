
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
                _globalPosition = _facility.getRendering()->getScreenCoordinates(_worldPosition);
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
