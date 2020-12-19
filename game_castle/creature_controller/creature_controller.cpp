
#include "interfaces.h"
#include "foundation/fsm.h"
#include "creature_controller.h"

namespace game {
    Enemy::Enemy(const std::shared_ptr<CreatureControllerImp> &controller, const std::shared_ptr<voxel::MeshFactory> &factory, const math::vector3f &position)
        : _controller(controller)
        , _position(position)
    {
        _mesh = factory->createDynamicMesh("enemies/troglodyte", position.flat3, 0.0f);
        _fsm.changeState(States::MOVING);
    }

    Enemy::~Enemy() {

    }

    void Enemy::movingEntered() {

    }

    void Enemy::movingUpdate(float dtSec) {
        _target = _controller->getNextPathPoint(_position);
        math::vector3f movement = (_target - _position).normalized(6.0f);

        _position = _position + movement * dtSec;
        _mesh->setTransform(_position.flat3, 0.0f);
    }

    void Enemy::movingLeaving() {

    }

    void Enemy::attackEntered() {

    }

    void Enemy::attackUpdate(float dtSec) {

    }

    void Enemy::attackLeaving() {

    }

    void Enemy::updateAndDraw(float dtSec) {
        _fsm.update(dtSec);
        _mesh->updateAndDraw(dtSec);
    }

    //---

    CreatureControllerImp::CreatureControllerImp(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<voxel::MeshFactory> &factory, const std::shared_ptr<voxel::TiledWorld> &environment, const std::shared_ptr<gears::Primitives> &primitives, CreatureDataHub &creatureData, CastleDataHub &castleData)
        : _platform(platform)
        , _factory(factory)
        , _environment(environment)
        , _primitives(primitives)
        , _creatureData(creatureData)
        , _castleData(castleData)
    {
        _onEnemyAddedToken = _creatureData.enemies.onElementAdded += [this](datahub::Token token, game::CreatureDataHub::Enemy &initData) {
            _enemies.emplace(std::piecewise_construct, std::forward_as_tuple(token), std::forward_as_tuple(shared_from_this(), _factory, initData.position));
        };
        _onEnemyRemovingToken = _creatureData.enemies.onElementRemoving += [this](datahub::Token token) {
            _enemies.erase(token);
        };
    }

    CreatureControllerImp::~CreatureControllerImp() {
        _creatureData.enemies.onElementAdded -= _onEnemyAddedToken;
        _creatureData.enemies.onElementRemoving -= _onEnemyRemovingToken;
    }

    void CreatureControllerImp::startBattle() {
        _initializePathMap();
    }

    void CreatureControllerImp::updateAndDraw(float dtSec) {
        for (auto &item : _enemies) {
            item.second.updateAndDraw(dtSec);
        }

        //const int mapSize = int(_environment->getMapSize());

        //for (int i = 0; i < mapSize; i++) {
        //    for (int c = 0; c < mapSize; c++) {
        //        if (_map[i][c].next != voxel::TiledWorld::NONEXIST_LOCATION) {
        //            math::vector3f start = _environment->getTileCenterPosition(voxel::TileLocation{c, i});
        //            math::vector3f dir = (_environment->getTileCenterPosition(_map[i][c].next) - start).normalized(4.0f);

        //            start.y += 2;

        //            _primitives->drawLine(start, start + dir, math::color(0, 1.0f, 0, 1.0f));
        //            _primitives->drawLine(start, start - dir, math::color(1.0f, 0, 0, 1.0f));
        //        }
        //    }
        //}
    }

    voxel::TileLocation CreatureControllerImp::getCurrentTileLocation(const math::vector3f &position) const {
        return _environment->getTileLocation(position);
    }

    math::vector3f CreatureControllerImp::getNextPathPoint(const math::vector3f &position) const {
        voxel::TileLocation location = _environment->getTileLocation(position);
        math::vector3f next = position;

        float dist = 1000.0f;

        for (int i = location.v - 1; i <= location.v + 1; i++) {
            for (int c = location.h - 1; c <= location.h + 1; c++) {
                if (_map[i][c].next != voxel::TiledWorld::NONEXIST_LOCATION) {
                    math::vector3f tilePosition = _environment->getTileCenterPosition(voxel::TileLocation{ c, i });
                    float distToNeighbor = position.distanceTo(tilePosition);

                    if (dist > distToNeighbor) {
                        dist = distToNeighbor;

                        next = _environment->getTileCenterPosition(_map[i][c].next);
                    }
                }
            }
        }

        return next;
    }

    void CreatureControllerImp::_initializePathMap() {
        const std::size_t mapSize = _environment->getMapSize();
        const std::uint8_t minTileType = 1;
        const std::uint8_t maxTileType = 1;

        if (_map == nullptr) {
            _map = new MapCell * [mapSize];

            for (std::size_t i = 0; i < mapSize; i++) {
                _map[i] = new MapCell[mapSize];
            }
        }

        for (std::size_t i = 0; i < mapSize; i++) {
            for (std::size_t c = 0; c < mapSize; c++) {
                _map[i][c].next = voxel::TiledWorld::NONEXIST_LOCATION;
                _map[i][c].totalCost = 0;
            }
        }

        std::list<voxel::TileLocation> frontline;

        auto evaluateNeighborPaths = [&](const voxel::TileLocation &targetLocation) {
            voxel::TileLocation neighborLocations[8];
            std::size_t neighborCount = _environment->getTileNeighbors(targetLocation, minTileType, maxTileType, neighborLocations);

            for (std::size_t i = 0; i < neighborCount; i++) {
                voxel::TileLocation neighborLocation = neighborLocations[i];

                MapCell &cell = _map[neighborLocation.v][neighborLocation.h];
                float dist = _map[targetLocation.v][targetLocation.h].totalCost + math::vector2f(float(neighborLocation.h), float(neighborLocation.v)).distanceTo(math::vector2f(float(targetLocation.h), float(targetLocation.v)));

                if (cell.next == voxel::TiledWorld::NONEXIST_LOCATION || dist < cell.totalCost) {
                    cell.next = targetLocation;
                    cell.totalCost = dist;
                    frontline.push_back(neighborLocations[i]);
                }
            }
        };

        _castleData.buildings.foreach([&](datahub::Token token, game::CastleDataHub::Building &buildingData) {
            evaluateNeighborPaths(buildingData.location);
        });

        int counter = 0;

        for (auto &item : frontline) {
            _map[item.v][item.h].totalCost = 0;
        }

        while (frontline.empty() == false) {
            voxel::TileLocation targetLocation = frontline.front();
            frontline.pop_front();

            evaluateNeighborPaths(targetLocation);
        }
    }

    void CreatureControllerImp::_updateCastleBorder() {

    }

}

namespace game {
    std::shared_ptr<CreatureController> CreatureController::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<voxel::MeshFactory> &factory, const std::shared_ptr<voxel::TiledWorld> &environment, const std::shared_ptr<gears::Primitives> &primitives, CreatureDataHub &enemyData, CastleDataHub &castleData) {
        return std::make_shared<CreatureControllerImp>(platform, factory, environment, primitives, enemyData, castleData);
    }
}