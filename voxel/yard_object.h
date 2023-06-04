
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/math.h"

namespace voxel {
    class YardObjectImpl : public YardInterface::Object {
    public:
        YardObjectImpl();
        ~YardObjectImpl();

    public:
        bool instantMove(const math::vector3f &position) = 0;
        void continuousMove(const math::vector3f &increment) = 0;
        void rotate(const math::vector3f &targetDirection) = 0;
    };
}

