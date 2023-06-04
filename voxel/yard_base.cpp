
#include "yard_base.h"

namespace voxel {
    YardStatic::YardStatic(const YardInterfaceProvider &interfaces, const math::bound3f &bbox) : _interfaces(interfaces), _currentState(YardStatic::State::NONE), _bbox(bbox) {}
    
    YardStatic::State YardStatic::getState() const {
        return _currentState;
    }
    
    math::bound3f YardStatic::getBBox() const {
        return _bbox;
    }
    
    const std::vector<YardStatic *> &YardStatic::getLinks() const {
        return _links;
    }
    
    void YardStatic::linkTo(YardStatic *object) {
        bool objectHasLink = false;
        bool thisHasLink = false;
        
        for (const YardStatic *link : object->getLinks()) {
            if (link == this) {
                objectHasLink = true;
                break;
            }
        }
        for (const YardStatic *link : _links) {
            if (link == object) {
                thisHasLink= true;
                break;
            }
        }
        
        if (thisHasLink == false) {
            _links.emplace_back(object);
        }
        if (objectHasLink == false) {
            object->linkTo(this);
        }
    }
}
