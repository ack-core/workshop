
namespace voxel {
    struct Voxel {
        std::int16_t positionX, positionY, positionZ, reserved;
        std::uint8_t sizeX, sizeY, sizeZ, colorIndex;
    };
    struct Frame {
        std::vector<Voxel> voxels;
    };

    std::vector<Frame> loadModel(const std::shared_ptr<foundation::PlatformInterface> &platform, const char *fullPath, float offsetX, float offsetY, float offsetZ);
    std::shared_ptr<foundation::RenderingTexture2D> loadPalette(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &renderingDevice, const char *fullPath);

    class StaticMeshImpl : StaticMesh {
    public:
        StaticMeshImpl(std::shared_ptr<foundation::RenderingStructuredData> &&geometry);
        ~StaticMeshImpl() override;

        void applyTransform(int x, int y, int z, Rotation rotation) override;
        const std::shared_ptr<foundation::RenderingStructuredData> &getGeometry() const override;

    private:
        std::shared_ptr<foundation::RenderingStructuredData> _geometry;
    };

    class DynamicMeshImpl : public DynamicMesh {
    public:
        DynamicMeshImpl(std::shared_ptr<foundation::RenderingStructuredData> &&geometry);
        ~DynamicMeshImpl() override;

        void setTransform(const float(&position)[3], float rotationXZ) override;
        void playAnimation(const char *name, std::function<void(DynamicMesh &)> &&finished = nullptr, bool cycled = false) override;

        void update(float dtSec) override;

        const float(&getTransform() const)[16] override;
        const std::shared_ptr<foundation::RenderingStructuredData> &getGeometry() const;

    private:
        std::shared_ptr<foundation::RenderingStructuredData> _geometry;

        float _transform[16];
    };

    class MeshFactoryImpl : public MeshFactory {
    public:
        MeshFactoryImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering);
        ~MeshFactoryImpl() override;

        std::shared_ptr<StaticMesh> createStaticMesh(const char *resourcePath) override;
        std::shared_ptr<DynamicMesh> createDynamicMesh(const char *resourcePath) override;

        std::shared_ptr<StaticMesh> mergeStaticMeshes(const std::vector<std::shared_ptr<StaticMesh>> &meshes) override;

    private:
        std::shared_ptr<foundation::PlatformInterface> _platform;
        std::shared_ptr<foundation::RenderingInterface> _rendering;
    };
}

