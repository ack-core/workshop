
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform/interfaces.h"
#include "foundation/rendering/interfaces.h"

#include "foundation/math.h"
#include "foundation/primitives.h"
#include "foundation/datahub.h"

#include "voxel/environment/interfaces.h"

namespace game {
    struct CastleDataHub : datahub::Scope {
        struct Building : datahub::Scope {
            enum class Type {
                NONE,
                HALL,
            };

            datahub::Value<Type> type;
            datahub::Value<unsigned> level;
            datahub::Value<float> health;
            datahub::Value<voxel::TileLocation> location;
        };

        struct Gate : datahub::Scope {
            datahub::Value<datahub::Token> building;
            datahub::Value<math::vector3f> entrancePosition;
        };

        struct Tower : datahub::Scope {
            datahub::Value<datahub::Token> building;
            datahub::Value<datahub::Token> nearestEnemy;
            datahub::Value<math::vector3f> projectileStart;
        };

        datahub::Array<Building> buildings;
        datahub::Value<float> health;
    };
}

namespace game {
    class CastleController {
    public:
        static std::shared_ptr<CastleController> instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<voxel::TiledWorld> &environment, const std::shared_ptr<gears::Primitives> &primitives, CastleDataHub &castleData);

    public:
        virtual void startBattle() = 0;
        virtual void updateAndDraw(float dtSec) = 0;

    protected:
        virtual ~CastleController() = default;
    };
}

