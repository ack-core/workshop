
#include "yard_base.h"

namespace voxel {
    YardStatic::YardStatic(const YardFacility &facility, std::uint64_t id, const math::bound3f &bbox) : _facility(facility), _id(id), _currentState(YardLoadingState::NONE), _bbox(bbox) {}
    
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
