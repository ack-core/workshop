
namespace game {
    class CastleControllerImp : public CastleController {
    public:
        CastleControllerImp(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<voxel::TiledWorld> &environment, const std::shared_ptr<gears::Primitives> &primitives, CastleDataHub &castleData);
        ~CastleControllerImp() override;

        void startBattle() override;
        void updateAndDraw(float dtSec) override;

    private:
        std::shared_ptr<foundation::PlatformInterface> _platform;
        std::shared_ptr<gears::Primitives> _primitives;
        std::shared_ptr<voxel::TiledWorld> _environment;

        CastleDataHub &_castleData;
    };
}

