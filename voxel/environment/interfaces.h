
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
    enum class TileClass : std::uint8_t {
        SOLID = 0,
        T_0000_1000,
        T_0000_1010,
        T_0000_1100,
        T_0000_1110,
        T_0000_1111,
        T_1000_xx00,
        T_1000_xx01,
        T_1000_xx10,
        T_1000_xx11,
        T_1100_xxx0,
        T_1100_xxx1,
        T_1010_xxxx,
        T_1110_xxxx,
        T_1111_xxxx,
    };

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
        static const TileLocation NONEXIST_LOCATION;

        static std::shared_ptr<TiledWorld> instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<voxel::MeshFactory> &factory, const std::shared_ptr<gears::Primitives> &primitives);

    public:
        virtual void clear() = 0;
        virtual void loadSpace(const char *spaceDirectory) = 0;

        virtual std::size_t getMapSize() const = 0;

        virtual bool isTileTypeSuitable(const TileLocation &offset, std::uint8_t typeIndex) = 0;
        virtual bool setTileTypeIndex(const TileLocation &offset, std::uint8_t typeIndex, const char *postfix = nullptr) = 0;

        virtual std::size_t getTileNeighbors(const TileLocation &location, std::uint8_t minTypeIndex, std::uint8_t maxTypeIndex, TileLocation(&out)[8]) const = 0;
        virtual std::uint8_t getTileTypeIndex(const TileLocation &location) const = 0;
        virtual math::vector3f getTileCenterPosition(const TileLocation &location) const = 0;
        virtual TileClass getTileClass(const TileLocation &location, MeshFactory::Rotation *rotation = nullptr) const = 0;
        virtual TileLocation getTileLocation(const math::vector3f &position) const = 0;

        virtual std::size_t getHelperCount(const char *name) const = 0;
        virtual math::vector3f getHelperPosition(const char *name, std::size_t index) const = 0;
        virtual bool getHelperPosition(const char *name, std::size_t index, math::vector3f &out) const = 0;
        virtual bool getHelperTileLocation(const char *name, std::size_t index, TileLocation &out) const = 0;

        virtual std::vector<TileLocation> findPath(const TileLocation &from, const TileLocation &to, std::uint8_t minTypeIndex, std::uint8_t maxTypeIndex) const = 0;

        virtual void updateAndDraw(float dtSec) = 0;

    protected:
        virtual ~TiledWorld() = default;
    };
}
