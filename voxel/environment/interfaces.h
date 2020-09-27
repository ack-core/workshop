
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform/interfaces.h"
#include "foundation/rendering/interfaces.h"
#include "foundation/primitives.h"
#include "foundation/math.h"

#include "voxel/mesh_factory/interfaces.h"

namespace voxel {
    struct TileLocation {
        int h;
        int v;

        bool operator ==(const TileLocation &other) const {
            return h == other.h && v == other.v;
        }
        bool operator !=(const TileLocation &other) const {
            return h != other.h || v != other.v;
        }
    };

    class TiledWorld {
    public:
        static const std::uint8_t NONEXIST_TILE = 0xff;
        static std::shared_ptr<TiledWorld> instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering, const std::shared_ptr<voxel::MeshFactory> &factory, const std::shared_ptr<gears::Primitives> &primitives);

    public:
        virtual void clear() = 0;
        virtual void loadSpace(const char *spaceDirectory) = 0;

        virtual bool isTileTypeSuitable(const TileLocation &offset, std::uint8_t typeIndex) = 0;
        virtual bool setTileTypeIndex(const TileLocation &offset, std::uint8_t typeIndex, const char *postfix = nullptr) = 0;

        virtual std::uint8_t getTileTypeIndex(const TileLocation &location) const = 0;
        virtual math::vector3f getTileCenterPosition(const TileLocation &location) const = 0;

        virtual std::size_t getHelperCount(const char *name) const = 0;
        virtual bool getHelperPosition(const char *name, std::size_t index, math::vector3f &out) const = 0;
        virtual bool getHelperTileLocation(const char *name, std::size_t index, TileLocation &out) const = 0;

        virtual std::vector<TileLocation> findPath(const TileLocation &from, const TileLocation &to, std::uint8_t minTypeIndex, std::uint8_t maxTypeIndex) const = 0;

        virtual void updateAndDraw(float dtSec) = 0;

    protected:
        virtual ~TiledWorld() = default;
    };
}
