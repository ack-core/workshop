
namespace voxel {
    class TiledWorldImpl : public TiledWorld {
    public:
        TiledWorldImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering, const std::shared_ptr<voxel::MeshFactory> &factory);
        ~TiledWorldImpl() override;

        void clear() override;
        void loadSpace(const char *spaceDirectory) override;

        void updateAndDraw(float dtSec) override;

    private:
        static const std::uint8_t NONEXIST_TILE = 0xff;

        std::shared_ptr<foundation::PlatformInterface> _platform;
        std::shared_ptr<foundation::RenderingInterface> _rendering;
        std::shared_ptr<foundation::RenderingTexture2D> _palette;
        std::shared_ptr<foundation::RenderingShader> _shader;

        std::shared_ptr<voxel::MeshFactory> _factory;

        int _tileSize = 0;

        struct TileType {
            std::string name;
            std::uint32_t color;
        };

        std::vector<TileType> _tileTypes;

        struct Chunk {
            std::uint8_t **mapSource = nullptr;
            std::size_t mapSize = 0;
            std::size_t drawSize = 0;
            std::vector<voxel::Voxel> voxels;
            std::shared_ptr<voxel::StaticMesh> geometry;
        };

        Chunk _chunk; // TODO: map of chunks
    };
}
