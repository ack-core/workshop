
namespace voxel {
    class TiledWorldImpl : public TiledWorld {
    public:
        TiledWorldImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<voxel::MeshFactory> &factory, const std::shared_ptr<gears::Primitives> &primitives);
        ~TiledWorldImpl() override;

        void clear() override;
        void loadSpace(const char *spaceDirectory) override;

        std::size_t getMapSize() const override;

        bool isTileTypeSuitable(const TileLocation &location, std::uint8_t typeIndex) override;
        bool setTileTypeIndex(const TileLocation &location, std::uint8_t typeIndex, const char *postfix) override;

        std::size_t getTileNeighbors(const TileLocation &location, std::uint8_t minTypeIndex, std::uint8_t maxTypeIndex, TileLocation(&out)[8]) const override;
        std::uint8_t getTileTypeIndex(const TileLocation &location) const override;
        math::vector3f getTileCenterPosition(const TileLocation &location) const override;
        TileClass getTileClass(const TileLocation &location, MeshFactory::Rotation *rotation = nullptr) const override;
        TileLocation getTileLocation(const math::vector3f &position) const override;

        std::size_t getHelperCount(const char *name) const override;
        math::vector3f getHelperPosition(const char *name, std::size_t index) const override;
        bool getHelperPosition(const char *name, std::size_t index, math::vector3f &out) const override;
        bool getHelperTileLocation(const char *name, std::size_t index, TileLocation &out) const override;

        std::vector<TileLocation> findPath(const TileLocation &from, const TileLocation &to, std::uint8_t minTypeIndex, std::uint8_t maxTypeIndex) const override;

        void updateAndDraw(float dtSec) override;

    private:
        std::shared_ptr<foundation::PlatformInterface> _platform;
        std::shared_ptr<voxel::MeshFactory> _factory;
        std::shared_ptr<gears::Primitives> _primitives;

        std::string _spaceDirectory;
        int _tileSize = 0;

        struct TileType {
            std::string name;
            std::uint32_t color;
        };

        struct Helper {
            TileLocation location;
            math::vector3f position;
        };

        struct MapCell {
            std::uint8_t tileTypeIndex;
            std::uint8_t neighborTypeIndex;
            MeshFactory::Rotation rotation;
            TileClass tileClass;
        };

        std::vector<TileType> _tileTypes;
        std::unordered_map<std::string, std::vector<Helper>> _helpers;

        MapCell **_mapSource = nullptr;
        std::size_t _mapSize = 0;
        std::size_t _visibleSize = 0;

        std::shared_ptr<voxel::StaticMesh> **_geometry; // TODO: baking, split to chunks

    private:
        bool _isInMap(const TileLocation &location) const;
        void _updateTileInfo(const TileLocation &location);
        void _updateTileGeometry(const TileLocation &location);
        std::size_t _getNeighbors(const TileLocation &location, std::uint8_t minTypeIndex, std::uint8_t maxTypeIndex, TileLocation(&out)[8]) const;
    };
}
