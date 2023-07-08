
#pragma once
#include "foundation/math.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace voxel {
    class YardThing : public YardStatic {
    public:
        YardThing(const YardFacility &facility, const math::bound3f &bbox);
        ~YardThing() override;

    public:
        void updateState(State targetState) override;
    };
}

