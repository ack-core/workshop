
#include "yard_base.h"
#include "yard_thing.h"

namespace voxel {
    YardThing::YardThing(const YardFacility &facility, const math::bound3f &bbox) : YardStatic(facility, bbox) {
    
    }
    
    YardThing::~YardThing() {
    
    }

    void YardThing::setState(State newState) {
    
    }
}
