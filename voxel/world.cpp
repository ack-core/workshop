
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
    struct Node {
        math::transform3f transform;
        std::string resourcePath;
        std::vector<Node *> children;
        
        Node(const util::Description &nodeDesc) {
            
        }
        
        virtual void load() {
            
        }
        
    };
}

namespace voxel {
    class ObjectImpl : public WorldInterface::Object, public std::enable_shared_from_this<ObjectImpl> {
    public:
        ObjectImpl() {

        }
        ~ObjectImpl() override {
            
        }
        void setTransform(const math::transform3f &trfm) override {
            _transform = trfm;
        }
        void release() override {
            
        }
        void load(const util::Description &objDesc) {
            for (auto &item : objDesc) {
                if (const util::Description *nodeDesc = std::any_cast<util::Description>(&item.second)) {
                    const std::string *name = nullptr;
                    auto index = nodeDesc->find("name");
                    if (index != nodeDesc->end() && (name = std::any_cast<std::string>(&index->second)) != nullptr) {
                        const std::string parentName = getParentName(*name);
                        Node *newNode = _nodes.emplace(*name, std::make_unique<Node>(*nodeDesc)).first->second.get();
                        if (parentName.length()) {
                            _nodes.at(parentName)->children.emplace_back(newNode);
                        }
                        else {
                            _root = newNode;
                        }
                    }
                    else {
                        
                    }
                }
            }
        }
        
    private:
        Node *_root = nullptr;
        std::unordered_map<std::string, std::unique_ptr<Node>> _nodes;
        math::transform3f _transform = math::transform3f::identity();
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
        void loadWorld(const char *descPath, util::callback<void(bool, float)> &&progress) override;
        auto getObject(const char *name) -> ObjectPtr override;
        auto createObject(const char *name, const char *prefabPath, util::callback<void(ObjectPtr)> &&completion) -> ObjectPtr override;
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
    void WorldImpl::loadWorld(const char *descPath, util::callback<void(bool, float)> &&progress) {
        
    }
    WorldInterface::ObjectPtr WorldImpl::getObject(const char *name) {
        return nullptr;
    }
    WorldInterface::ObjectPtr WorldImpl::createObject(const char *name, const char *prefabPath, util::callback<void(ObjectPtr)> &&completion) {
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

