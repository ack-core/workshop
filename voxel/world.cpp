
#include "world.h"
#include <unordered_map>

namespace voxel {
    class ObjectImpl;
    class WorldImpl : public WorldInterface, public std::enable_shared_from_this<WorldImpl> {
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

        foundation::PlatformInterface &getPlatform() const { return *_platform; }
        resource::ResourceProvider &getResources() const { return *_resourceProvider; }
        voxel::SceneInterface &getScene() const { return *_scene; }
        
    public:
        const foundation::PlatformInterfacePtr _platform;
        const resource::ResourceProviderPtr _resourceProvider;
        const voxel::SceneInterfacePtr _scene;
        
        std::unordered_map<std::string, std::shared_ptr<ObjectImpl>> _objects;
    };
}

namespace voxel {
    static math::transform3f g_identity;

    class ObjectNode {
    public:
        const ObjectNode *parent = nullptr;
        math::transform3f localTransform;
        math::transform3f worldTransform;
        std::string resourcePath;
        
        virtual ~ObjectNode() = default;
        virtual void loadResources(const std::shared_ptr<WorldImpl> &world, const std::weak_ptr<ObjectImpl> &objweak) = 0;
        virtual void unloadResources() = 0;
        virtual void update(float dtSec) = 0;
    };

    std::unique_ptr<ObjectNode> (*g_nodeConstructors[int(WorldInterface::NodeType::_count)])() = {};
    std::size_t getNextUniqueId() {
        static std::size_t nextId = 0;
        return nextId++;
    }
}

namespace voxel {
    class ObjectImpl : public WorldInterface::Object, public std::enable_shared_from_this<ObjectImpl> {
    public:
        ObjectImpl(std::shared_ptr<WorldImpl> &&owner,  const util::Description &objDesc) : _id(getNextUniqueId()), _owner(std::move(owner)) {
            const std::unordered_map<std::string, const util::Description *> descs = objDesc.getDescriptions();
            
            for (const auto &nodeDesc : descs) {
                if (const std::int64_t *type = nodeDesc.second->getInteger("type")) {
                    if (g_nodeConstructors[*type]) {
                        _nameToNodeIndex.emplace(nodeDesc.first, _nodes.size());
                        _nodes.emplace_back(g_nodeConstructors[*type]());
                        _nodes.back()->localTransform = math::transform3f::identity().translated(nodeDesc.second->getVector3f("position", {}));
                        _nodes.back()->resourcePath = nodeDesc.second->getString("resourcePath", "<Unknown>");
                        
                        if (const std::int64_t *parentIndex = nodeDesc.second->getInteger("parentIndex")) {
                            _nodes.back()->parent = _nodes[*parentIndex].get();
                            _nodes.back()->worldTransform = _nodes.back()->localTransform * _nodes[*parentIndex]->worldTransform;
                        }
                        else {
                            _nodes.back()->worldTransform = _nodes.back()->localTransform;
                        }
                    }
                    else {
                        owner->getPlatform().logError("[ObjectImpl::ObjectImpl] Unknown object type = %d", int(*type));
                        break;
                    }
                }
            }
        }
        ~ObjectImpl() override {
            
        }
        
        std::size_t getId() const override {
            return _id;
        }
        
        void loadResources(util::callback<void(WorldInterface::Object &)> &&completion) override {
            _loadingCompletion = std::move(completion);
            _loading = int(_nodes.size());
            const std::weak_ptr<ObjectImpl> weakself = weak_from_this();
            for (std::unique_ptr<ObjectNode> &node : _nodes) {
                node->loadResources(_owner, weakself);
            }
        }
        void unloadResources() override {
            for (std::unique_ptr<ObjectNode> &node : _nodes) {
                node->unloadResources();
            }
        }
        void nodeLoadingComplete() {
            if (--_loading == 0) {
                _loadingCompletion(*this);
                _loadingCompletion = {};
            }
        }
        
        void setPosition(const math::vector3f &pos) override {
            _nodes[0]->localTransform.rv3 = pos.atv4start(1.0f).block;
            _nodes[0]->worldTransform.rv3 = pos.atv4start(1.0f).block;
        }
        void setTransform(const math::transform3f &trfm) override {
            _nodes[0]->localTransform = _nodes[0]->worldTransform = trfm;
        }
        void setLocalTransform(const char *nodeName, const math::transform3f &trfm) override {
            
        }
        const math::transform3f &getLocalTransform(const char *nodeName) override {
            return g_identity;
        }
        const math::transform3f &getWorldTransform(const char *nodeName) override {
            return g_identity;
        }
        void setVelocity(const math::vector3f &v) override {
            
        }
        
        void play(const char *animationName, util::callback<void(WorldInterface::Object &)> &&completion) override {}
        
        void update(float dtSec) {
            for (std::unique_ptr<ObjectNode> &node : _nodes) {
                node->worldTransform = node->parent ? node->localTransform * node->parent->worldTransform : node->localTransform;
                node->update(dtSec);
            }
        }
        
    private:
        std::size_t _id;
        std::shared_ptr<WorldImpl> _owner;
        std::vector<std::unique_ptr<ObjectNode>> _nodes;
        std::unordered_map<std::string, std::size_t> _nameToNodeIndex;
        util::callback<void(WorldInterface::Object &)> _loadingCompletion;
        int _loading = 0;
    };
}

namespace voxel {
    struct VoxelMeshNode : public ObjectNode {
        voxel::SceneInterface::VoxelMeshPtr mesh;
        
        void loadResources(const std::shared_ptr<WorldImpl> &world, const std::weak_ptr<ObjectImpl> &objweak) override {
            resource::ResourceProvider &res = world->getResources();
            res.getOrLoadVoxelMesh(resourcePath.c_str(), [world, this, objweak](const std::vector<foundation::RenderDataPtr> &data, const util::Description& desc) {
                if (auto object = objweak.lock()) {
                    if (data.size()) {
                        mesh = world->getScene().addVoxelMesh(data, desc);
                        mesh->setTransform(worldTransform);
                    }
                    object->nodeLoadingComplete();
                }
            });
        }
        void unloadResources() override {
            mesh = nullptr;
        }
        void update(float dtSec) override {
            if (mesh) {
                mesh->setTransform(worldTransform);
            }
        }
    };

    struct ParticlesNode : public ObjectNode {
        float t = 0;
        voxel::SceneInterface::ParticlesPtr particles;

        void loadResources(const std::shared_ptr<WorldImpl> &world, const std::weak_ptr<ObjectImpl> &objweak) override {
            resource::ResourceProvider &res = world->getResources();
            res.getOrLoadEmitter(resourcePath.c_str(), [world, this, objweak](const util::Description &desc, const foundation::RenderTexturePtr &m, const foundation::RenderTexturePtr &t) {
                if (auto object = objweak.lock()) {
                    if (m && desc.empty() == false) {
                        const voxel::ParticlesParams parameters (desc);
                        particles = world->getScene().addParticles(t, m, parameters);
                        particles->setTransform(worldTransform);
                    }
                    object->nodeLoadingComplete();
                }
            });

        }
        void unloadResources() override {
            particles = nullptr;
        }
        void update(float dtSec) override {
            if (particles) {
                particles->setTransform(worldTransform);
                particles->setTime(t, 0.0f);
                t += dtSec;
            }
        }
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
        g_nodeConstructors[int(WorldInterface::NodeType::VOXEL)] = []() -> std::unique_ptr<ObjectNode> {
            return std::make_unique<VoxelMeshNode>();
        };
        g_nodeConstructors[int(WorldInterface::NodeType::PARTICLES)] = []() -> std::unique_ptr<ObjectNode> {
            return std::make_unique<ParticlesNode>();
        };
    }
    WorldImpl::~WorldImpl() {
    
    }
    WorldInterface::ObjectPtr WorldImpl::getObject(const char *name) {
        auto index = _objects.find(name);
        if (index != _objects.end()) {
            return index->second;
        }
        return nullptr;
    }
    WorldInterface::ObjectPtr WorldImpl::createObject(const char *name, const char *prefabPath) {
        const util::Description desc = _resourceProvider->getPrefab(prefabPath);
        if (desc.empty() == false) {
            if (_objects.find(name) == _objects.end()) {
                return _objects.emplace(std::string(name), std::make_shared<ObjectImpl>(shared_from_this(), desc)).first->second;
            }
            else {
                _platform->logError("[WorldImpl::createObject] Object with name '%s' already exists in the world", name);
            }
        }
        else {
            _platform->logError("[WorldImpl::createObject] Prefab '%s' has no valid nodes", prefabPath);
        }
        return nullptr;
    }
    void WorldImpl::update(float dtSec) {
        for (auto index = _objects.begin(); index != _objects.end(); ) {
            if (index->second.use_count() > 1) {
                index->second->update(dtSec);
                ++index;
            }
            else {
                index = _objects.erase(index);
            }
        }
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

