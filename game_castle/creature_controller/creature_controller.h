
namespace game {
    class CreatureControllerImp;

    class Enemy {
    public:
        Enemy(const std::shared_ptr<CreatureControllerImp> &controller, const std::shared_ptr<voxel::MeshFactory> &factory, const math::vector3f &position);
        ~Enemy();

        void movingEntered();
        void movingUpdate(float dtSec);
        void movingLeaving();

        void attackEntered();
        void attackUpdate(float dtSec);
        void attackLeaving();

        void updateAndDraw(float dtSec);

    protected:
        std::shared_ptr<CreatureControllerImp> _controller;
        std::shared_ptr<voxel::DynamicMesh> _mesh;
        voxel::TileLocation _currentTile = {};
        math::vector3f _position;
        math::vector3f _target;

        enum class States {
            MOVING,
            ATTACK,
            _count
        };

        gears::FSM<States> _fsm {
            { States::MOVING, gears::FSMState(this, &Enemy::movingEntered, &Enemy::movingUpdate, &Enemy::movingLeaving) },
            { States::ATTACK, gears::FSMState(this, &Enemy::attackEntered, &Enemy::attackUpdate, &Enemy::attackLeaving) },
        };
    };

    class CreatureControllerImp : public std::enable_shared_from_this<CreatureControllerImp>, public CreatureController {
    public:
        CreatureControllerImp(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<voxel::MeshFactory> &factory, const std::shared_ptr<voxel::TiledWorld> &environment, const std::shared_ptr<gears::Primitives> &primitives, CreatureDataHub &creatureData, CastleDataHub &castleData);
        ~CreatureControllerImp() override;

        void startBattle() override;
        void updateAndDraw(float dtSec) override;

        voxel::TileLocation getCurrentTileLocation(const math::vector3f &position) const;
        math::vector3f getNextPathPoint(const math::vector3f &position) const;

    private:
        std::shared_ptr<foundation::PlatformInterface> _platform;
        std::shared_ptr<voxel::MeshFactory> _factory;
        std::shared_ptr<voxel::TiledWorld> _environment;
        std::shared_ptr<gears::Primitives> _primitives;

        CreatureDataHub &_creatureData;
        CastleDataHub &_castleData;

        datahub::Token _onEnemyAddedToken = nullptr;
        datahub::Token _onEnemyRemovingToken = nullptr;

        std::unordered_map<datahub::Token, Enemy> _enemies;

        struct MapCell {
            voxel::TileLocation next;
            float totalCost;
        };

        MapCell **_map = nullptr;
        std::vector<math::vector3f> _castleBorder;

        void _initializePathMap();
        void _updateCastleBorder();
    };
}

