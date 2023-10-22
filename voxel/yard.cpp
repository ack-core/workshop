
#include "yard.h"
#include "yard_base.h"
#include "yard_stead.h"
#include "yard_thing.h"
#include "yard_actor.h"

#include "thirdparty/expect/expect.h"

#include "simulation.h"

#include <unordered_map>
#include <vector>
#include <sstream>

namespace voxel {
    class YardImpl : public std::enable_shared_from_this<YardImpl>, public YardInterface, public YardFacility {
    public:
        static const std::uint64_t INVALID_ID = std::uint64_t(-1);
        static constexpr float GLOBAL_COORD_DIV = 16.0f;
        
    public:
        YardImpl(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            const resource::MeshProviderPtr &meshProvider,
            const resource::TextureProviderPtr &textureProvider,
            const SceneInterfacePtr &scene
        );
        ~YardImpl() override;
        
    public:
        const foundation::PlatformInterfacePtr &getPlatform() const override { return _platform; }
        const foundation::RenderingInterfacePtr &getRendering() const override { return _rendering; }
        const resource::MeshProviderPtr &getMeshProvider() const override { return _meshProvider; }
        const resource::TextureProviderPtr &getTextureProvider() const override { return _textureProvider; }
        const SceneInterfacePtr &getScene() const override { return _scene; }
        
    public:
        void setCameraLookAt(const math::vector3f &position, const math::vector3f &target) override;
        void loadYard(const char *src, util::callback<void(bool loaded)> &&completion) override;
        void saveYard(const char *outputPath, util::callback<void(bool saved)> &&completion) override;
        void addActorType(const char *type, ActorTypeDesc &&desc) override;
        
        auto addThing(const char *model, const math::vector3f &position) -> std::shared_ptr<Thing> override;
        auto addStead(const char *heightmap, const char *texture, const math::vector3f &position) -> std::shared_ptr<Stead> override;
        auto addActor(const char *type, const math::vector3f &position, const math::vector3f &direction) -> std::shared_ptr<Actor> override;

        void remove(std::uint64_t id) override;
        void remove(const std::shared_ptr<Actor> &actor) override;

        void update(float dtSec) override;
        
    public:
        auto _addThing(const char *model, const math::vector3f &position, std::uint64_t id = INVALID_ID) -> std::shared_ptr<Thing>;
        void _addStatic(std::uint64_t id, const std::shared_ptr<YardStatic> &object);
        void _clearYard();
        
    public:
        const foundation::PlatformInterfacePtr _platform;
        const foundation::RenderingInterfacePtr _rendering;
        const resource::MeshProviderPtr _meshProvider;
        const resource::TextureProviderPtr _textureProvider;

        const SimulationInterfacePtr _simulation;
        const SceneInterfacePtr _scene;
        
        struct Node {
            std::vector<YardStatic *> items;
        };
        
        std::unordered_map<std::uint64_t, Node> _nodes;
        std::unordered_map<std::uint64_t, std::shared_ptr<YardStatic>> _statics;

        std::unordered_map<std::string, ActorTypeDesc> _actorTypes;
        std::vector<std::shared_ptr<YardActorImpl>> _actors;
        
        std::uint64_t _lastId = 0x1000;
    };
}

namespace voxel {
    std::shared_ptr<YardInterface> YardInterface::instance(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const resource::MeshProviderPtr &meshProvider,
        const resource::TextureProviderPtr &textureProvider,
        const SceneInterfacePtr &scene
    ) {
        return std::make_shared<YardImpl>(platform, rendering, meshProvider, textureProvider, scene);
    }
}

namespace voxel {
    YardImpl::YardImpl(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const resource::MeshProviderPtr &meshProvider,
        const resource::TextureProviderPtr &textureProvider,
        const SceneInterfacePtr &scene
    )
    : _platform(platform)
    , _rendering(rendering)
    , _meshProvider(meshProvider)
    , _textureProvider(textureProvider)
    , _simulation(SimulationInterface::instance())
    , _scene(scene)
    {    
    }
    
    YardImpl::~YardImpl() {
    
    }
    
    void YardImpl::setCameraLookAt(const math::vector3f &position, const math::vector3f &target) {
        _scene->setCameraLookAt(position, target);
    }
    
    void YardImpl::loadYard(const char *sourcepath, util::callback<void(bool loaded)> &&completion) {
        std::string yardpath = std::string(sourcepath) + ".yard";
        
        _platform->loadFile(yardpath.data(), [weak = weak_from_this(), yardpath, completion = std::move(completion)](std::unique_ptr<uint8_t[]> &&binary, std::size_t size){
            if (std::shared_ptr<YardImpl> self = weak.lock()) {
                const foundation::PlatformInterfacePtr &platform = self->_platform;
                self->_clearYard();

                if (size) {
                    std::istringstream source = std::istringstream((const char *)binary.get());
                    std::uint64_t id;
                    std::string type, block, name, parameter;
                    std::unordered_map<std::uint64_t, std::vector<std::uint64_t>> links;

                    math::vector3f position;
                    math::bound3f bbox;
                    
                    while (!source.eof() && (source >> type)) {
                        if (type == "options" && bool(source >> expect::braced(block, '{', '}'))) {
                            std::istringstream input = std::istringstream(block);
                            
                            while (input.eof() == false && input >> parameter) {
                                //if (parameter == "partial" && bool(input >> std::boolalpha >> self->_partial) == false) {
                                //    platform->logError("[YardImpl::loadYard] options block has invalid 'partial' syntax\n", id);
                                //}
                                input >> std::ws;
                            }
                            
                            if (input.fail() != false) {
                                platform->logError("[YardImpl::loadYard] unable to load 'options' block\n", id);
                            }
                        }
                        if (type == "stead" && bool(source >> id >> expect::braced(block, '{', '}'))) {
                            std::istringstream input = std::istringstream(block);
                            std::string texture, heightmap;
                            
                            while (input.eof() == false && input >> parameter) {
                                if (parameter == "texture" && bool(input >> expect::braced(texture, '"', '"')) == false) {
                                    platform->logError("[YardImpl::loadYard] square with id = '%zu' has invalid 'texture' syntax\n", id);
                                }
                                if (parameter == "heightmap" && bool(input >> expect::braced(heightmap, '"', '"')) == false) {
                                    platform->logError("[YardImpl::loadYard] square with id = '%zu' has invalid 'heightmap' syntax\n", id);
                                }
                                if (parameter == "position" && bool(input >> position.x >> position.y >> position.z) == false) {
                                    platform->logError("[YardImpl::loadYard] square with id = '%zu' has invalid 'position' syntax\n", id);
                                }
                                if (parameter == "link" && bool(input >> expect::nlist(links[id])) == false) {
                                    platform->logError("[YardImpl::loadYard] square with id = '%zu' has invalid 'links' syntax\n", id);
                                }
                                input >> std::ws;
                            }
                            
                            if (input.fail() == false) {
                                if (const resource::TextureInfo *txInfo = self->_textureProvider->getTextureInfo(texture.data())) {
                                    const resource::TextureInfo *hmInfo = self->_textureProvider->getTextureInfo(heightmap.data());
                                    
                                    bbox.xmin = position.x - 0.5;
                                    bbox.ymin = position.y - 0.5;
                                    bbox.zmin = position.z - 0.5;
                                    bbox.xmax = bbox.xmin + txInfo->width;
                                    bbox.ymax = bbox.ymin + 1.0;
                                    bbox.zmax = bbox.zmin + txInfo->height;
                                    
                                    if (heightmap[0]) {
                                        if (hmInfo) {
                                            if (txInfo->type == resource::TextureInfo::Type::RGBA8 && hmInfo->type == resource::TextureInfo::Type::GRAYSCALE8) {
                                                if (txInfo->width == hmInfo->width + 1 && txInfo->height == hmInfo->height + 1) {
                                                    self->_addStatic(id, std::make_shared<YardSteadImpl>(*self, id, bbox, std::move(texture), std::move(heightmap)));
                                                }
                                                else {
                                                    platform->logError("[YardImpl::loadYard] texture and heightmap doesn't fit each other for square with id = '%zu'\n", id);
                                                }
                                            }
                                            else {
                                                platform->logError("[YardImpl::loadYard] texture or heightmap has bad format for square with id = '%zu'\n", id);
                                            }
                                        }
                                        else {
                                            platform->logError("[YardImpl::loadYard] heightmap '%s' for square with id = '%zu' has no info\n", heightmap.data(), id);
                                        }
                                    }
                                    else {
                                        self->_addStatic(id, std::make_shared<YardSteadImpl>(*self, id, bbox, std::move(texture), std::string()));
                                    }
                                }
                                else {
                                    platform->logError("[YardImpl::loadYard] texture '%s' for square with id = '%zu' has no info\n", texture.data(), id);
                                }
                            }
                            else {
                                platform->logError("[YardImpl::loadYard] unable to load square with id = '%zu'\n", id);
                            }
                        }
                        if (type == "thing" && bool(source >> id >> expect::braced(block, '{', '}'))) {
                            std::istringstream input = std::istringstream(block);
                            std::string modelPath;
                            
                            while (input.eof() == false && input >> parameter) {
                                if (parameter == "model" && bool(input >> expect::braced(modelPath, '"', '"')) == false) {
                                    platform->logError("[YardImpl::loadYard] thing with id = '%zu' has invalid 'model' syntax\n", id);
                                }
                                if (parameter == "position" && bool(input >> position.x >> position.y >> position.z) == false) {
                                    platform->logError("[YardImpl::loadYard] thing with id = '%zu' has invalid 'position' syntax\n", id);
                                }
                                if (parameter == "link" && bool(input >> expect::nlist(links[id])) == false) {
                                    platform->logError("[YardImpl::loadYard] thing with id = '%zu' has invalid 'links' syntax\n", id);
                                }
                                input >> std::ws;
                            }
                            
                            if (input.fail() == false) {
                                self->_addThing(modelPath.data(), position, id);
                            }
                            else {
                                platform->logError("[YardImpl::loadYard] unable to load thing with id = '%zu'\n", id);
                            }
                        }
                        if (type == "actor" && bool(source >> expect::braced(name, '"', '"') >> expect::braced(block, '{', '}'))) {
                            std::istringstream input = std::istringstream(block);
                            std::string modelPath;
                            math::vector3f centerPoint;
                            float radius;
                            
                            while (input.eof() == false && input >> parameter) {
                                if (parameter == "model" && bool(input >> expect::braced(modelPath, '"', '"')) == false) {
                                    platform->logError("[YardImpl::loadYard] object type '%s' has invalid 'model' syntax\n", name.data());
                                }
                                if (parameter == "center" && bool(input >> centerPoint.x >> centerPoint.y >> centerPoint.z) == false) {
                                    platform->logError("[YardImpl::loadYard] object type '%s' has invalid 'center' syntax\n", name.data());
                                }
                                if (parameter == "radius" && bool(input >> radius) == false) {
                                    platform->logError("[YardImpl::loadYard] object type '%s' has invalid 'radius' syntax\n", name.data());
                                }
                                input >> std::ws;
                            }
                            
                            if (input.fail() == false) {
                                self->addActorType(name.data(), ActorTypeDesc {
                                    .modelPath = modelPath,
                                    .centerPoint = centerPoint,
                                    .radius = radius
                                });
                            }
                            else {
                                platform->logError("[YardImpl::loadYard] unable to load object type '%s'\n", name.data());
                            }
                        }
                        
                        block.clear();
                        source >> std::ws;
                    }
                    
                    for (auto &item : self->_statics) {
                        for (auto &link : links[item.first]) {
                            item.second->linkTo(self->_statics[link].get());
                        }
                    }
                    
                    if (source.fail() == false) {
                        completion(true);
                        return;
                    }
                    else {
                        //platform->logError("[YardImpl::loadYard] yard '%s' isn't complete\n", yardpath.data());
                    }
                }
                else {
                    platform->logError("[YardImpl::loadYard] unable to load yard '%s'\n", yardpath.data());
                }

                completion(false);
            }
        });
    }
    
    void YardImpl::saveYard(const char *outputPath, util::callback<void(bool saved)> &&completion) {
    
    }
    
    void YardImpl::addActorType(const char *type, ActorTypeDesc &&desc) {
        _actorTypes.emplace(type, std::move(desc));
    }
    
    std::shared_ptr<YardInterface::Thing> YardImpl::addThing(const char *model, const math::vector3f &position) {
        return _addThing(model, position, _lastId++);
    }

    std::shared_ptr<YardInterface::Stead> YardImpl::addStead(const char *heightmap, const char *texture, const math::vector3f &position) {
    
    }
    
    std::shared_ptr<YardInterface::Actor> YardImpl::addActor(const char *type, const math::vector3f &position, const math::vector3f &direction) {
        auto index = _actorTypes.find(type);
        if (index != _actorTypes.end()) {
            std::shared_ptr<YardActorImpl> actor = std::make_shared<YardActorImpl>(*this, index->second, position, direction);
            return _actors.emplace_back(actor);
        }
        else {
            _platform->logError("[YardImpl::addObject] unknown object type '%s'\n", type);
        }
        
        return nullptr;
    }
    
    void YardImpl::remove(std::uint64_t id) {
    
    }
    
    void YardImpl::remove(const std::shared_ptr<Actor> &actor) {
    
    }
    
    void YardImpl::update(float dtSec) {
        for (auto &item : _statics) {
            item.second->updateState(YardLoadingState::RENDERING);
        }
        for (auto &item : _actors) {
            item->update(dtSec);
        }
    }
    
    std::shared_ptr<YardInterface::Thing> YardImpl::_addThing(const char *model, const math::vector3f &position, std::uint64_t id) {
        std::shared_ptr<YardThingImpl> thing = nullptr;
        
        if (const resource::MeshInfo *info = _meshProvider->getMeshInfo(model)) {
            math::bound3f bbox;
            bbox.xmin = -0.5;
            bbox.ymin = -0.5;
            bbox.zmin = -0.5;
            bbox.xmax = bbox.xmin + info->sizeX;
            bbox.ymax = bbox.ymin + info->sizeY;
            bbox.zmax = bbox.zmin + info->sizeZ;
            thing = std::make_shared<YardThingImpl>(*this, id, position, bbox, std::move(model));
            _addStatic(id, thing);
        }
        else {
            _platform->logError("[YardImpl::loadYard] mesh '%s' has no info\n", model);
        }
        
        return thing;
    }
    
    void YardImpl::_addStatic(std::uint64_t id, const std::shared_ptr<YardStatic> &object) {
        const math::bound3f &bbox = object->getBBox();
        
        int startx = int(bbox.xmin / GLOBAL_COORD_DIV);
        int startz = int(bbox.zmin / GLOBAL_COORD_DIV);
        int endx = int(bbox.xmax / GLOBAL_COORD_DIV);
        int endz = int(bbox.zmax / GLOBAL_COORD_DIV);
        
        if (_statics.find(id) == _statics.end()) {
            auto index = _statics.emplace(id, object);
        
            for (int i = startx; i <= endx; i++) {
                for (int c = startz; c <= endz; c++) {
                    std::uint64_t key = (std::uint64_t(c) << 32) | std::uint64_t(i);
                    Node &node = _nodes[key];
                    node.items.emplace_back(index.first->second.get());
                }
            }
        }
        else {
            _platform->logError("[YardImpl::_addStatic] static object with id = '%zu' already exists\n", id);
        }
    }
    
    void YardImpl::_clearYard() {
    
    }
    
}
