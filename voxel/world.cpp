
#include "world.h"
#include <unordered_map>

namespace {
    std::string getParentName(const std::string &nodeName) {
        std::size_t pos = nodeName.rfind('.');
        if (pos == std::string::npos)
            return "";

        return nodeName.substr(0, pos);
    }
}

namespace voxel {
    class ObjectNode {
    public:
        math::transform3f transform;
        std::string resourcePath;

        ObjectNode(const util::Description &nodeDesc) {
            
        }
        virtual ~ObjectNode() {
            
        }
        virtual void update(float dtSec) {
            
        }
    };

    struct VoxelMeshNode : public ObjectNode {
        
    };

    struct ParticlesNode : public ObjectNode {
        
    };
}

namespace voxel {
    class ObjectImpl : public WorldInterface::Object, public std::enable_shared_from_this<ObjectImpl> {
    public:
        ObjectImpl() {

        }
        ~ObjectImpl() override {
            
        }
        void setTransform(const char *nodeName, const math::transform3f &trfm) override {
            
        }
        void loadResources(util::callback<void(WorldInterface::ObjectPtr)> &&completion) override {
            
        }
        void unloadResources() override {
            
        }
        void update(float dtSec) {
            
        }
        
    private:
        std::vector<std::unique_ptr<ObjectNode>> _nodes;
        std::unordered_map<std::string, std::size_t> _nameToNodeIndex;
    };
}

namespace voxel {
    class WorldImpl : public WorldInterface {
    public:
        WorldImpl(
            const foundation::PlatformInterfacePtr &platform,
            const resource::ResourceProviderPtr &resources,
            const voxel::SceneInterfacePtr &scene
        );
        ~WorldImpl() override;
        
    public:
        auto getObject(const char *name) -> ObjectPtr override;
        auto createObject(const char *name, const char *prefabPath) -> ObjectPtr override;
        void update(float dtSec) override;

    public:
        const foundation::PlatformInterfacePtr _platform;
        const resource::ResourceProviderPtr _resourceProvider;
        const voxel::SceneInterfacePtr _scene;
        
        std::unordered_map<std::string, std::shared_ptr<ObjectImpl>> _objects;
    };
}

namespace voxel {
    WorldImpl::WorldImpl(
        const foundation::PlatformInterfacePtr &platform,
        const resource::ResourceProviderPtr &resources,
        const voxel::SceneInterfacePtr &scene
    )
    : _platform(platform)
    , _resourceProvider(resources)
    , _scene(scene)
    {
    
    }
    WorldImpl::~WorldImpl() {
    
    }
    WorldInterface::ObjectPtr WorldImpl::getObject(const char *name) {
        return nullptr;
    }
    WorldInterface::ObjectPtr WorldImpl::createObject(const char *name, const char *prefabPath) {
        return nullptr;
    }
    void WorldImpl::update(float dtSec) {
        
    }
}

namespace voxel {
    std::shared_ptr<WorldInterface> WorldInterface::instance(
        const foundation::PlatformInterfacePtr &platform,
        const resource::ResourceProviderPtr &resources,
        const voxel::SceneInterfacePtr &scene
    )
    {
        return std::make_shared<WorldImpl>(platform, resources, scene);
    }
}

