
namespace voxel {
    struct Chunk {
        std::vector<Voxel> voxels;
        int16_t modelBounds[3];
    };
    struct Model {
        struct Frame {
            std::uint32_t index;
            std::uint32_t size;
        };
        struct Animation {
            std::uint32_t firstFrame;
            std::uint32_t lastFrame;
            float frameRate;
        };

        std::shared_ptr<foundation::RenderingStructuredData> voxels;
        std::unordered_map<std::string, Animation> animations;
        std::vector<Frame> frames;
    };

    class MeshFactoryImpl;

    class StaticMeshImpl : public StaticMesh {
    public:
        StaticMeshImpl(const std::shared_ptr<MeshFactoryImpl> &factory, const std::shared_ptr<foundation::RenderingStructuredData> &geometry);
        ~StaticMeshImpl() override;

        void updateAndDraw(float dtSec) override;

    private:
        std::shared_ptr<MeshFactoryImpl> _factory;
        std::shared_ptr<foundation::RenderingStructuredData> _geometry;
    };

    class DynamicMeshImpl : public DynamicMesh {
    public:
        DynamicMeshImpl(const std::shared_ptr<MeshFactoryImpl> &factory, const std::shared_ptr<Model> &model);
        ~DynamicMeshImpl() override;

        void setTransform(const float(&position)[3], float rotationXZ) override;
        void playAnimation(const char *name, std::function<void(DynamicMesh &)> &&finished, bool cycled, bool resetAnimationTime) override;

        void updateAndDraw(float dtSec) override;

    private:
        std::shared_ptr<MeshFactoryImpl> _factory;
        std::shared_ptr<Model> _model;

        Model::Animation *_currentAnimation = nullptr;
        std::function<void(DynamicMesh &)> _finished;

        float _time = 0.0f;
        float _transform[16];
        
        bool _cycled = false;

        std::uint32_t _lastFrame = 0;
        std::uint32_t _currentFrame = 0;
    };

    class MeshFactoryImpl : public std::enable_shared_from_this<MeshFactoryImpl>, public MeshFactory {
    public:
        MeshFactoryImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering, const char *palettePath);
        ~MeshFactoryImpl() override;

        bool loadVoxels(const char *voxFullPath, int x, int y, int z, Rotation rotation, std::vector<Voxel> &out) override;

        std::shared_ptr<StaticMesh> createStaticMesh(const Voxel *voxels, std::size_t count) override;
        std::shared_ptr<DynamicMesh> createDynamicMesh(const char *resourcePath, const float(&position)[3], float rotationXZ) override;

        foundation::RenderingInterface &getRenderingInterface();

    private:
        std::shared_ptr<foundation::PlatformInterface> _platform;
        std::shared_ptr<foundation::RenderingInterface> _rendering;

        std::unordered_map<std::string, std::weak_ptr<Model>> _models;
        std::unordered_map<std::string, voxel::Chunk> _cache;
    };
}

