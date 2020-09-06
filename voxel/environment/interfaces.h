
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform/interfaces.h"
#include "foundation/rendering/interfaces.h"
#include "foundation/math.h"

#include "voxel/mesh_factory/interfaces.h"

namespace voxel {
    struct TileOffset {
        int h;
        int v;
    };

    class TiledWorld {
    public:
        static const std::uint8_t NONEXIST_TILE = 0xff;
        static std::shared_ptr<TiledWorld> instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering, const std::shared_ptr<voxel::MeshFactory> &factory);

    public:
        virtual void clear() = 0;
        virtual void loadSpace(const char *spaceDirectory) = 0;

        virtual bool isTileTypeSuitable(const TileOffset &offset, std::uint8_t typeIndex) = 0;
        virtual bool setTileTypeIndex(const TileOffset &offset, std::uint8_t typeIndex, const char *postfix = nullptr) = 0;

        virtual std::uint8_t getTileTypeIndex(const TileOffset &offset) const = 0;
        virtual math::vector3f getTileCenterPosition(const TileOffset &offset) const = 0;

        virtual std::size_t getHelperCount(const char *name) const = 0;
        virtual bool getHelperPosition(const char *name, std::size_t index, math::vector3f &out) const = 0;
        virtual bool getHelperTileOffset(const char *name, std::size_t index, TileOffset &out) const = 0;

        virtual void updateAndDraw(float dtSec) = 0;

    protected:
        virtual ~TiledWorld() = default;
    };
}
