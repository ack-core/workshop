
#include "world.h"
#include <unordered_map>

namespace core {
    class ObjectImpl;
    class CollisionNode;
}

namespace core {
    class WorldImpl : public WorldInterface, public std::enable_shared_from_this<WorldImpl> {
    public:
        WorldImpl(
            const foundation::PlatformInterfacePtr &platform,
            const resource::ResourceProviderPtr &resources,
            const core::SceneInterfacePtr &scene,
            const core::RaycastInterfacePtr &raycast,
            const core::SimulationInterfacePtr &simulation
        );
        ~WorldImpl() override;
        
    public:
        auto getObject(const char *name) -> ObjectPtr override;
        auto createObject(const char *prefabPath, std::uint64_t typeMask, const char *name) -> ObjectPtr override;
        void removeObject(const char *name) override;
        void update(float dtSec) override;

        foundation::PlatformInterface &getPlatform() const { return *_platform; }
        resource::ResourceProvider &getResources() const { return *_resourceProvider; }
        core::SceneInterface &getScene() const { return *_scene; }
        core::RaycastInterface &getRaycast() const { return *_raycast; }
        core::SimulationInterface &getSimulation() const { return *_simulation; }
        
    public:
        const foundation::PlatformInterfacePtr _platform;
        const resource::ResourceProviderPtr _resourceProvider;
        const core::SceneInterfacePtr _scene;
        const core::RaycastInterfacePtr _raycast;
        const core::SimulationInterfacePtr _simulation;
        
        std::unordered_map<std::string, std::shared_ptr<ObjectImpl>> _namedObjects;
        std::unordered_map<std::uint64_t, std::shared_ptr<ObjectImpl>> _unnamedObjects;
    };
}

namespace core {
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
        static std::size_t nextId = 1000000000;
        return nextId++;
    }
}

namespace core {
    class ObjectImpl : public WorldInterface::Object, public std::enable_shared_from_this<ObjectImpl> {
    public:
        ObjectImpl(std::shared_ptr<WorldImpl> &&owner,  std::uint64_t id, const util::Description &objDesc, std::size_t mask);
        ~ObjectImpl() override {
            
        }
        
        std::uint64_t getId() const override {
            return _id;
        }
        std::uint64_t getTypeMask() const override {
            return _typeMask;
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
        
        void setPosition(const math::vector3f &pos) override;
        void setTransform(const math::transform3f &trfm) override;
        void setLocalTransform(const char *nodeName, const math::transform3f &trfm) override;
        auto getLocalTransform(const char *nodeName) const -> const math::transform3f & override;
        auto getWorldTransform(const char *nodeName) const -> const math::transform3f & override;
        auto getWorldTransform() const -> const math::transform3f & override;
        auto getWorldPosition() const -> const math::vector3f override;
        void setVelocity(const math::vector3f &v) override;
        void play(const char *animationName, util::callback<void(WorldInterface::Object &)> &&completion) override;
        void update(float dtSec);
        
    private:
        std::size_t _id;
        std::size_t _typeMask;
        std::shared_ptr<WorldImpl> _owner;
        std::vector<std::unique_ptr<ObjectNode>> _nodes;
        std::unordered_map<std::string, std::size_t> _nameToNodeIndex;
        util::callback<void(WorldInterface::Object &)> _loadingCompletion;
        int _loading = 0;
        CollisionNode *_collisionNode = nullptr;
    };
}

namespace core {
    struct VoxelMeshNode : public ObjectNode {
        core::SceneInterface::VoxelMeshPtr mesh;
        
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
        core::SceneInterface::ParticlesPtr particles;

        void loadResources(const std::shared_ptr<WorldImpl> &world, const std::weak_ptr<ObjectImpl> &objweak) override {
            resource::ResourceProvider &res = world->getResources();
            res.getOrLoadEmitter(resourcePath.c_str(), [world, this, objweak](const util::Description &desc, const foundation::RenderTexturePtr &m, const foundation::RenderTexturePtr &t) {
                if (auto object = objweak.lock()) {
                    if (m && desc.empty() == false) {
                        const core::ParticlesParams parameters (desc);
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
    
    struct RaycastNode : public ObjectNode {
        core::RaycastInterface::ShapePtr shape;

        void loadResources(const std::shared_ptr<WorldImpl> &world, const std::weak_ptr<ObjectImpl> &objweak) override {
            resource::ResourceProvider &res = world->getResources();
            res.getOrLoadDescription(resourcePath.c_str(), [world, this, objweak](const util::Description &desc) {
                if (auto object = objweak.lock()) {
                    if (desc.empty() == false) {
                        shape = world->getRaycast().addShape(desc, object->getId(), object->getTypeMask());
                        shape->setTransform(worldTransform);
                    }
                    object->nodeLoadingComplete();
                }
            });
        }
        void unloadResources() override {
            shape = nullptr;
        }
        void update(float dtSec) override {
            if (shape) {
                shape->setTransform(worldTransform);
            }
        }
    };
    
    struct CollisionNode : public ObjectNode {
        core::SimulationInterface::BodyPtr body;

        void loadResources(const std::shared_ptr<WorldImpl> &world, const std::weak_ptr<ObjectImpl> &objweak) override {
            resource::ResourceProvider &res = world->getResources();
            res.getOrLoadDescription(resourcePath.c_str(), [world, this, objweak](const util::Description &desc) {
                if (auto object = objweak.lock()) {
                    if (desc.empty() == false) {
                        body = world->getSimulation().addBody(desc);
                        body->setTransform(worldTransform);
                    }
                    object->nodeLoadingComplete();
                }
            });
        }
        void unloadResources() override {
            body = nullptr;
        }
        void update(float dtSec) override {
            if (body) {
                worldTransform = body->getTransform();
            }
        }
    };

}

namespace core {
    ObjectImpl::ObjectImpl(std::shared_ptr<WorldImpl> &&owner,  std::uint64_t id, const util::Description &objDesc, std::size_t mask) : _id(id), _typeMask(mask), _owner(std::move(owner)) {
        const std::map<std::string, const util::Description *> descs = objDesc.getDescriptions();
        
        for (const auto &nodeDesc : descs) {
            if (const std::int64_t *type = nodeDesc.second->getInteger("type")) {
                if (g_nodeConstructors[*type]) {
                    _nameToNodeIndex.emplace(nodeDesc.first, _nodes.size());
                    _nodes.emplace_back(g_nodeConstructors[*type]());
                    _nodes.back()->localTransform = math::transform3f::identity().translated(nodeDesc.second->getVector3f("position", {}));
                    if (const std::string *resourcePath = nodeDesc.second->getString("resourcePath")) {
                        _nodes.back()->resourcePath = *resourcePath;
                    }
                    else {
                        owner->getPlatform().logError("[ObjectImpl::ObjectImpl] Invalid resource path for = %s", nodeDesc.first.data());
                    }
                    if (const std::int64_t *parentIndex = nodeDesc.second->getInteger("parentIndex")) {
                        _nodes.back()->parent = _nodes[*parentIndex].get();
                        _nodes.back()->worldTransform = _nodes.back()->localTransform * _nodes[*parentIndex]->worldTransform;
                    }
                    else {
                        _nodes.back()->worldTransform = _nodes.back()->localTransform;
                    }
                    if (*type == std::int64_t(WorldInterface::NodeType::COLLISION)) {
                        if (_collisionNode == nullptr) {
                            _collisionNode = static_cast<CollisionNode *>(_nodes.back().get());
                        }
                        else {
                            owner->getPlatform().logError("[ObjectImpl::ObjectImpl] There can be only one collision node at the root");
                        }
                    }
                }
                else {
                    owner->getPlatform().logError("[ObjectImpl::ObjectImpl] Unknown object type = %d", int(*type));
                    break;
                }
            }
        }
    }
    void ObjectImpl::setPosition(const math::vector3f &pos) {
        _nodes[0]->worldTransform.rv3 = pos.atv4start(1.0f).block;
        if (_collisionNode && _collisionNode->body) {
            _collisionNode->body->setTransform(_nodes[0]->worldTransform);
        }
    }
    void ObjectImpl::setTransform(const math::transform3f &trfm) {
        _nodes[0]->worldTransform = trfm;
        if (_collisionNode && _collisionNode->body) {
            _collisionNode->body->setTransform(_nodes[0]->worldTransform);
        }
    }
    void ObjectImpl::setLocalTransform(const char *nodeName, const math::transform3f &trfm) {
        
    }
    const math::transform3f &ObjectImpl::getLocalTransform(const char *nodeName) const {
        return g_identity;
    }
    const math::transform3f &ObjectImpl::getWorldTransform(const char *nodeName) const {
        return g_identity;
    }
    const math::transform3f &ObjectImpl::getWorldTransform() const {
        return _nodes[0]->worldTransform;
    }
    const math::vector3f ObjectImpl::getWorldPosition() const {
        return math::vector3f(_nodes[0]->worldTransform.m41, _nodes[0]->worldTransform.m42, _nodes[0]->worldTransform.m43);
    }
    void ObjectImpl::setVelocity(const math::vector3f &v) {
        if (_collisionNode && _collisionNode->body) {
            _collisionNode->body->setVelocity(v);
        }
    }
    void ObjectImpl::play(const char *animationName, util::callback<void(WorldInterface::Object &)> &&completion) {
        
    }
    void ObjectImpl::update(float dtSec) {
        for (std::unique_ptr<ObjectNode> &node : _nodes) {
            node->worldTransform = node->parent ? node->localTransform * node->parent->worldTransform : node->worldTransform;
            node->update(dtSec);
        }
    }
}

namespace core {
    WorldImpl::WorldImpl(
        const foundation::PlatformInterfacePtr &platform,
        const resource::ResourceProviderPtr &resources,
        const core::SceneInterfacePtr &scene,
        const core::RaycastInterfacePtr &raycast,
        const core::SimulationInterfacePtr &simulation
    )
    : _platform(platform)
    , _resourceProvider(resources)
    , _scene(scene)
    , _raycast(raycast)
    , _simulation(simulation)
    {
        g_nodeConstructors[int(WorldInterface::NodeType::VOXEL)] = []() -> std::unique_ptr<ObjectNode> {
            return std::make_unique<VoxelMeshNode>();
        };
        g_nodeConstructors[int(WorldInterface::NodeType::PARTICLES)] = []() -> std::unique_ptr<ObjectNode> {
            return std::make_unique<ParticlesNode>();
        };
        g_nodeConstructors[int(WorldInterface::NodeType::RAYCAST)] = []() -> std::unique_ptr<ObjectNode> {
            return std::make_unique<RaycastNode>();
        };
        g_nodeConstructors[int(WorldInterface::NodeType::COLLISION)] = []() -> std::unique_ptr<ObjectNode> {
            return std::make_unique<CollisionNode>();
        };
    }
    WorldImpl::~WorldImpl() {
    
    }
    WorldInterface::ObjectPtr WorldImpl::getObject(const char *name) {
        auto index = _namedObjects.find(name);
        if (index != _namedObjects.end()) {
            return index->second;
        }
        return nullptr;
    }
    WorldInterface::ObjectPtr WorldImpl::createObject(const char *prefabPath, std::uint64_t typeMask, const char *name) {
        const std::uint64_t newId = getNextUniqueId();
        const util::Description desc = _resourceProvider->getPrefab(prefabPath);
        if (desc.empty() == false) {
            if (name) {
                if (_namedObjects.find(name) == _namedObjects.end()) {
                    return _namedObjects.emplace(std::string(name), std::make_shared<ObjectImpl>(shared_from_this(), newId, desc, typeMask)).first->second;
                }
                else {
                    _platform->logError("[WorldImpl::createObject] Object with name '%s' already exists in the world", name);
                }
            }
            else {
                return _unnamedObjects.emplace(newId, std::make_shared<ObjectImpl>(shared_from_this(), newId, desc, typeMask)).first->second;
            }
        }
        else {
            _platform->logError("[WorldImpl::createObject] Prefab '%s' has no valid nodes", prefabPath);
        }
        return nullptr;
    }
    void WorldImpl::removeObject(const char *name) {
        _namedObjects.erase(name);
    }
    void WorldImpl::update(float dtSec) {
        for (auto &item : _namedObjects) {
            item.second->update(dtSec);
        }
        for (auto index = _unnamedObjects.begin(); index != _unnamedObjects.end(); ) {
            if (index->second.use_count() > 1) {
                index->second->update(dtSec);
                ++index;
            }
            else {
                index = _unnamedObjects.erase(index);
            }
        }
    }
}

namespace core {
    std::shared_ptr<WorldInterface> WorldInterface::instance(
        const foundation::PlatformInterfacePtr &platform,
        const resource::ResourceProviderPtr &resources,
        const core::SceneInterfacePtr &scene,
        const core::RaycastInterfacePtr &raycast,
        const core::SimulationInterfacePtr &simulation
    )
    {
        return std::make_shared<WorldImpl>(platform, resources, scene, raycast, simulation);
    }
}

