
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/math.h"

namespace voxel {
    class YardThing : public YardStatic {
    public:
        YardThing(const YardInterfaceProvider &interfaces, const math::bound3f &bbox);
        ~YardThing() override;

    };
}

