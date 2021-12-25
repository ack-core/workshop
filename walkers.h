
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/rendering.h"
#include "voxel/mesh_factory.h"

namespace game {
    class Walkers {
    public:
        static std::shared_ptr<Walkers> instance(const voxel::MeshFactoryPtr &factory);
        virtual void updateAndDraw(float dtSec) = 0;
        virtual void test() = 0;

    protected:
        virtual ~Walkers() = default;
    };

    using WalkersPtr = std::shared_ptr<Walkers>;
}