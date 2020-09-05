
namespace voxel {
    class TiledWorldImpl : public TiledWorld {
    public:
        TiledWorldImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering, const std::shared_ptr<voxel::MeshFactory> &factory);
        ~TiledWorldImpl() override;

        void clear() override;
        void loadSpace(const char *spaceDirectory) override;

        bool setTileTypeIndex(const TileOffset &offset, std::uint8_t typeIndex) override;

        std::size_t getMapSize() const override;
        std::uint8_t getTileTypeIndex(const TileOffset &offset) const override;
        math::vector3f getTileCenterPosition(const TileOffset &offset) const override;

        std::size_t getHelperCount(const char *name) const override;
        bool getHelperPosition(const char *name, std::size_t index, math::vector3f &out) const override;

        void updateAndDraw(float dtSec) override;

    public:
        void _updateTileGeometry(const TileOffset &offset);

    private:
        std::shared_ptr<foundation::PlatformInterface> _platform;
        std::shared_ptr<foundation::RenderingInterface> _rendering;
        std::shared_ptr<foundation::RenderingTexture2D> _palette;
        std::shared_ptr<foundation::RenderingShader> _shader;

        std::shared_ptr<voxel::MeshFactory> _factory;

        std::string _spaceDirectory;
        int _tileSize = 0;

        struct TileType {
            std::string name;
            std::uint32_t color;
        };

        std::vector<TileType> _tileTypes;
        std::unordered_map<std::string, std::vector<math::vector3f>> _helpers;

        struct Chunk {
            std::uint8_t **mapSource = nullptr;
            std::size_t mapSize = 0;
            std::size_t drawSize = 0;
            std::shared_ptr<voxel::StaticMesh> **geometry; // TODO: baking
        };

        Chunk _chunk; // TODO: map of chunks
    };
}
