
namespace voxel {
    class TiledWorldImpl : public TiledWorld {
    public:
        TiledWorldImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering, const std::shared_ptr<voxel::MeshFactory> &factory, const std::shared_ptr<gears::Primitives> &primitives);
        ~TiledWorldImpl() override;

        void clear() override;
        void loadSpace(const char *spaceDirectory) override;

        bool isTileTypeSuitable(const TileLocation &location, std::uint8_t typeIndex) override;
        bool setTileTypeIndex(const TileLocation &location, std::uint8_t typeIndex, const char *postfix) override;

        std::uint8_t getTileTypeIndex(const TileLocation &location) const override;
        math::vector3f getTileCenterPosition(const TileLocation &location) const override;

        std::size_t getHelperCount(const char *name) const override;
        bool getHelperPosition(const char *name, std::size_t index, math::vector3f &out) const override;
        bool getHelperTileLocation(const char *name, std::size_t index, TileLocation &out) const override;

        std::vector<TileLocation> findPath(const TileLocation &from, const TileLocation &to, std::uint8_t minTypeIndex, std::uint8_t maxTypeIndex) const override;

        void updateAndDraw(float dtSec) override;

    private:
        std::shared_ptr<foundation::PlatformInterface> _platform;
        std::shared_ptr<foundation::RenderingInterface> _rendering;
        std::shared_ptr<foundation::RenderingTexture2D> _palette;
        std::shared_ptr<foundation::RenderingShader> _shader;

        std::shared_ptr<gears::Primitives> _primitives;
        std::shared_ptr<voxel::MeshFactory> _factory;

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

        std::vector<TileType> _tileTypes;
        std::unordered_map<std::string, std::vector<Helper>> _helpers;

        struct Chunk {
            std::uint8_t **mapSource = nullptr;
            std::size_t mapSize = 0;
            std::size_t drawSize = 0;
            std::shared_ptr<voxel::StaticMesh> **geometry; // TODO: baking
        };

        Chunk _chunk; // TODO: map of chunks

    private:
        bool _isInChunk(const Chunk &chunk, const TileLocation &location) const;
        void _updateTileGeometry(Chunk &chunk, const TileLocation &location);
        std::size_t _getNeighbors(const Chunk &chunk, const TileLocation &location, std::uint8_t minTypeIndex, std::uint8_t maxTypeIndex, TileLocation(&out)[8]) const;
    };
}
