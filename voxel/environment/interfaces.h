
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform/interfaces.h"
#include "foundation/rendering/interfaces.h"
#include "foundation/math.h"

#include "voxel/mesh_factory/interfaces.h"

namespace voxel {
    class TiledWorld {
    public:
        static std::shared_ptr<TiledWorld> instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering, const std::shared_ptr<voxel::MeshFactory> &factory);

    public:
        virtual void clear() = 0;
        virtual void loadSpace(const char *spaceDirectory) = 0;

        //virtual std::uint8_t tileUp(int i, int c) = 0;
        //virtual std::uint8_t tileDown(int i, int c) = 0;

        //virtual std::size_t getMapSize() const = 0;
        //virtual std::uint8_t getTileTypeIndex(int i, int c) const = 0;
        //virtual math::vector3f getTileCenterPosition(int i, int c) const = 0;

        virtual void updateAndDraw(float dtSec) = 0;

    protected:
        virtual ~TiledWorld() = default;
    };
}
