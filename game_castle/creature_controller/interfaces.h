
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

#include "game_castle/castle_controller/interfaces.h"

namespace game {
    struct CreatureDataHub : datahub::Scope {
        struct Enemy : datahub::Scope {
            datahub::Value<float> health;
            datahub::Value<math::vector3f> position;
        };
        struct Ally : datahub::Scope {
            datahub::Value<float> health;
            datahub::Value<math::vector3f> position;
        };

        datahub::Array<Enemy> enemies;
        datahub::Array<Ally> allies;
    };
}

namespace game {
    class CreatureController {
    public:
        static std::shared_ptr<CreatureController> instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<voxel::MeshFactory> &factory, const std::shared_ptr<voxel::TiledWorld> &environment, const std::shared_ptr<gears::Primitives> &primitives, CreatureDataHub &creatureData, CastleDataHub &castleData);

    public:
        virtual void startBattle() = 0;
        virtual void updateAndDraw(float dtSec) = 0;

    protected:
        virtual ~CreatureController() = default;
    };
}

