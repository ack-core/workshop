
#include "interfaces.h"
#include "castle_controller.h"

namespace game {
    CastleControllerImp::CastleControllerImp(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<voxel::TiledWorld> &environment, const std::shared_ptr<gears::Primitives> &primitives, CastleDataHub &castleData)
        : _platform(platform)
        , _primitives(primitives)
        , _environment(environment)
        , _castleData(castleData)
    {

    }

    CastleControllerImp::~CastleControllerImp() {

    }

    void CastleControllerImp::startBattle() {
        _castleData.health = 1000.0f;
        _castleData.buildings.clear();
        
        voxel::TileLocation castleHallLocation;
        if (_environment->getHelperTileLocation("castle", 0, castleHallLocation)) {
            for (int v = castleHallLocation.v - 1; v <= castleHallLocation.v + 1; v++) {
                for (int h = castleHallLocation.h - 1; h <= castleHallLocation.h + 1; h++) {
                    CastleDataHub::Building::Type buildingType = (v == castleHallLocation.v && h == castleHallLocation.h) ? CastleDataHub::Building::Type::HALL : CastleDataHub::Building::Type::NONE;
                    
                    _castleData.buildings.add([&](CastleDataHub::Building &building) {
                        building.health = 100.0f;
                        building.level = 0;
                        building.type = buildingType;
                        building.location = voxel::TileLocation{ h, v };
                    });

                    _environment->setTileTypeIndex(voxel::TileLocation{ h, v }, 0);
                }
            }
        }
        else {

        }
    }

    void CastleControllerImp::updateAndDraw(float dtSec) {

    }
}

namespace game {
    std::shared_ptr<CastleController> CastleController::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<voxel::TiledWorld> &environment, const std::shared_ptr<gears::Primitives> &primitives, CastleDataHub &castleData) {
        return std::make_shared<CastleControllerImp>(platform, environment, primitives, castleData);
    }
}