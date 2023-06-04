
#include "yard_base.h"
#include "yard_thing.h"

namespace voxel {
    YardThing::YardThing(const YardInterfaceProvider &interfaces, const math::bound3f &bbox) : YardStatic(interfaces, bbox) {
    
    }
    
    YardThing::~YardThing() {
    
    }

}
