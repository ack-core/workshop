
namespace voxel {
    struct Chunk {
        std::vector<Voxel> voxels;
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

    class StaticMeshImpl : public StaticMesh {
    public:
        StaticMeshImpl(const std::shared_ptr<foundation::RenderingStructuredData> &geometry);
        ~StaticMeshImpl() override;

        const std::shared_ptr<foundation::RenderingStructuredData> &getGeometry() const override;

    private:
        std::shared_ptr<foundation::RenderingStructuredData> _geometry;
    };

    class DynamicMeshImpl : public DynamicMesh {
    public:
        DynamicMeshImpl(const std::shared_ptr<Model> &model);
        ~DynamicMeshImpl() override;

        void setTransform(const float(&position)[3], float rotationXZ) override;
        void playAnimation(const char *name, std::function<void(DynamicMesh &)> &&finished, bool cycled, bool resetAnimationTime) override;
        void update(float dtSec) override;

        const float(&getTransform() const)[16] override;
        const std::shared_ptr<foundation::RenderingStructuredData> &getGeometry() const override;
        const std::uint32_t getFrameStartIndex() const override;
        const std::uint32_t getFrameSize() const override;

    private:
        std::shared_ptr<Model> _model;
        Model::Animation *_currentAnimation = nullptr;

        std::function<void(DynamicMesh &)> _finished;
        
        float _transform[16];
        float _time = 0.0f;
        
        bool _cycled = false;

        std::uint32_t _lastFrame = 0;
        std::uint32_t _currentFrame = 0;
    };

    class MeshFactoryImpl : public MeshFactory {
    public:
        MeshFactoryImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering);
        ~MeshFactoryImpl() override;

        bool loadVoxels(const char *voxFullPath, int x, int y, int z, Rotation rotation, std::vector<Voxel> &out) override;

        std::shared_ptr<StaticMesh> createStaticMesh(const std::vector<Voxel> &voxels) override;
        std::shared_ptr<DynamicMesh> createDynamicMesh(const char *resourcePath, const float(&position)[3], float rotationXZ) override;

    private:
        std::shared_ptr<foundation::PlatformInterface> _platform;
        std::shared_ptr<foundation::RenderingInterface> _rendering;
        std::unordered_map<std::string, std::weak_ptr<Model>> _models;
    };
}

