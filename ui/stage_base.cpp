
#include "stage.h"
#include "stage_base.h"

namespace ui {
    ElementImpl::ElementImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent) : _facility(facility), _parent(std::dynamic_pointer_cast<ElementImpl>(parent)) {
        _globalPosition = math::vector2f(0, 0);
        _anchorOffsets = math::vector2f(0, 0);
        _hAnchor = HorizontalAnchor::LEFT;
        _vAnchor = VerticalAnchor::TOP;
        _size = math::vector2f(0, 0);
    }
    
    ElementImpl::~ElementImpl() {
    
    }
    
    void ElementImpl::setAnchor(const std::shared_ptr<Element> &target, HorizontalAnchor h, VerticalAnchor v, float hOffset, float vOffset) {
        if (target) {
            std::shared_ptr<ElementImpl> targetImpl = std::dynamic_pointer_cast<ElementImpl>(target);
            if (targetImpl->_parent.lock() == _parent.lock()) {
                if (targetImpl->_anchorTarget.lock().get() != this) {
                    _anchorTarget = targetImpl;
                }
                else {
                    _facility.getPlatform()->logError("[ElementImpl::setAnchor] Cyclic anchor is not allowed\n");
                }
            }
            else {
                _facility.getPlatform()->logError("[ElementImpl::setAnchor] Target must be in same hierarchy level\n");
            }
        }
        
        _anchorOffsets = math::vector2f(hOffset, vOffset);
        _hAnchor = h;
        _vAnchor = v;
    }
    
    void ElementImpl::attachElement(const std::shared_ptr<ElementImpl> &element) {
        _attachedElements.emplace_back(element);
    }
    
    const math::vector2f &ElementImpl::getPosition() const {
        return _globalPosition;
    }
    
    const math::vector2f &ElementImpl::getSize() const {
        return _size;
    }
    
    void ElementImpl::updateCoordinates() {
        math::vector2f lt = math::vector2f(0.0f, 0.0f);
        math::vector2f rb = math::vector2f(_facility.getPlatform()->getScreenWidth(), _facility.getPlatform()->getScreenHeight());
        
        if (auto target = _anchorTarget.lock()) {
            lt = target->_globalPosition;
            rb = target->_globalPosition + target->_size;
        }
        else if (auto parent = _parent.lock()) {
            lt = parent->_globalPosition;
            rb = parent->_globalPosition + parent->_size;
        }
        
        if (_hAnchor == HorizontalAnchor::LEFTSIDE) {
            _globalPosition.x = lt.x - _anchorOffsets.x - _size.x;
        }
        else if (_hAnchor == HorizontalAnchor::LEFT) {
            _globalPosition.x = lt.x + _anchorOffsets.x;
        }
        else if (_hAnchor == HorizontalAnchor::CENTER) {
            _globalPosition.x = lt.x + std::floor(0.5f * (rb.x - lt.x - _size.x));
        }
        else if (_hAnchor == HorizontalAnchor::RIGHT) {
            _globalPosition.x = rb.x - _size.x - _anchorOffsets.x;
        }
        else if (_hAnchor == HorizontalAnchor::RIGHTSIDE) {
            _globalPosition.x = rb.x + _anchorOffsets.x;
        }
        
        if (_vAnchor == VerticalAnchor::TOPSIDE) {
            _globalPosition.y = lt.y - _anchorOffsets.y - _size.y;
        }
        else if (_vAnchor == VerticalAnchor::TOP) {
            _globalPosition.y = lt.y + _anchorOffsets.y;
        }
        else if (_vAnchor == VerticalAnchor::MIDDLE) {
            _globalPosition.y = lt.y + std::floor(0.5f * (rb.y - lt.y - _size.y));
        }
        else if (_vAnchor == VerticalAnchor::BOTTOM) {
            _globalPosition.y = rb.y - _size.y - _anchorOffsets.y;
        }
        else if (_vAnchor == VerticalAnchor::BOTTOMSIDE) {
            _globalPosition.y = rb.y + _anchorOffsets.y;
        }
        
        for (auto &item : _attachedElements) {
            item->updateCoordinates();
        }
    }
    
    bool ElementImpl::onInteraction(ui::Action action, std::size_t id, float x, float y) {
        for (auto &item : _attachedElements) {
            if (item->onInteraction(action, id, x, y)) {
                return true;
            }
        }
        
        return false;
    }
    
    void ElementImpl::draw(ShaderConstants &constStorage) {
        for (auto &item : _attachedElements) {
            item->draw(constStorage);
        }
    }
    
    InteractorImpl::InteractorImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent, bool capturePointer) : ElementImpl(facility, parent) {
        _capturePointer = capturePointer;
        _pointerId = foundation::INVALID_POINTER_ID;
        _currentAction = ui::Action::RELEASE;
        _activeAreaOffset = 0.0f;
        _activeAreaRadius = 0.0f;
    }
    
    InteractorImpl::~InteractorImpl() {
    
    }
    
    void InteractorImpl::setActiveArea(float offset, float radius) {
        _activeAreaOffset = offset;
        _activeAreaRadius = radius;
    }
    
    void InteractorImpl::setActionHandler(ui::Action action, util::callback<void(float x, float y)> &&handler) {
        _handlers[int(action)] = std::move(handler);
    }
    
    bool InteractorImpl::onInteraction(ui::Action action, std::size_t id, float x, float y) {
        if (ElementImpl::onInteraction(action, id, x, y)) {
            return true;
        }
        
        bool isInArea = false;
        const math::vector2f lt = math::vector2f(_globalPosition.x + _activeAreaOffset, _globalPosition.y + _activeAreaOffset);
        const math::vector2f rb = math::vector2f(_globalPosition.x + _size.x - _activeAreaOffset, _globalPosition.y + _size.y - _activeAreaOffset);
        
        if (x < lt.x) {
            if (y < lt.y) {
                isInArea = math::vector2f(x, y).distanceTo(lt) < _activeAreaRadius;
            }
            else if (y > rb.y) {
                isInArea = math::vector2f(x, y).distanceTo(math::vector2f(lt.x, rb.y)) < _activeAreaRadius;
            }
            else {
                isInArea = std::fabs(x - lt.x) < _activeAreaRadius;
            }
        }
        else if (x > rb.x) {
            if (y < lt.y) {
                isInArea = math::vector2f(x, y).distanceTo(math::vector2f(rb.x, lt.y)) < _activeAreaRadius;
            }
            else if (y > rb.y) {
                isInArea = math::vector2f(x, y).distanceTo(rb) < _activeAreaRadius;
            }
            else {
                isInArea = std::fabs(x - rb.x) < _activeAreaRadius;
            }
        }
        else {
            if (y < lt.y) {
                isInArea = std::fabs(y - lt.y) < _activeAreaRadius;
            }
            else if (y > rb.y) {
                isInArea = std::fabs(y - rb.y) < _activeAreaRadius;
            }
            else {
                isInArea = true;
            }
        }

        if (isInArea && action == ui::Action::CAPTURE) {
            if (_handlers[int(action)]) {
                _pointerId = id;
                _currentAction = action;
                _handlers[int(action)](x, y);
                return true;
            }
            if (_capturePointer) {
                _currentAction = action;
                _pointerId = id;
                return true;
            }
        }
        if (action == ui::Action::MOVE && _pointerId != foundation::INVALID_POINTER_ID) {
            _currentAction = action;
            
            if (_handlers[int(action)]) {
                _handlers[int(action)](x, y);
            }

            return true;
        }
        if (action == ui::Action::RELEASE) {
            if (_handlers[int(action)] && _pointerId != foundation::INVALID_POINTER_ID) {
                _handlers[int(action)](x, y);
            }

            _pointerId = foundation::INVALID_POINTER_ID;
            _currentAction = action;
        }
        
        return false;
    }
}
