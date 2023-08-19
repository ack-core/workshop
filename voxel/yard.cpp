
#include "yard.h"
#include "yard_base.h"
#include "yard_square.h"
#include "yard_thing.h"
#include "yard_object.h"

#include "thirdparty/expect/expect.h"

#include <unordered_map>
#include <vector>
#include <sstream>

namespace {
    static const std::size_t LOADING_DEPTH = 3;
    static const std::size_t RENDERING_DEPTH = 2;
    static const float GLOBAL_COORD_DIV = 16.0f;
}

namespace voxel {
    class YardImpl : public std::enable_shared_from_this<YardImpl>, public YardInterface, public YardFacility, public YardCollision {
    public:
        YardImpl(const foundation::PlatformInterfacePtr &platform, const resource::MeshProviderPtr &meshProvider, const resource::TextureProviderPtr &textureProvider, const SceneInterfacePtr &scene);
        ~YardImpl() override;
        
    public:
        const foundation::PlatformInterfacePtr &getPlatform() const override { return _platform; }
        const resource::MeshProviderPtr &getMeshProvider() const override { return _meshProvider; }
        const resource::TextureProviderPtr &getTextureProvider() const override { return _textureProvider; }
        const SceneInterfacePtr &getScene() const override { return _scene; }
    
    public:
        void correctMovement(const math::vector3f &position, math::vector3f &movement) const override;
        
    public:
        void loadYard(const char *src, util::callback<void(bool loaded)> &&completion) override;
        auto addObject(const char *type, const math::vector3f &position, const math::vector3f &direction) -> std::shared_ptr<Object> override;
        void update(float dtSec) override;
        
    public:
        void _addStatic(std::uint64_t id, const std::shared_ptr<YardStatic> &object);
        void _clearYard();
        
    public:
        const foundation::PlatformInterfacePtr _platform;
        const resource::MeshProviderPtr _meshProvider;
        const resource::TextureProviderPtr _textureProvider;
        const SceneInterfacePtr _scene;
        
        struct Node {
            std::vector<YardStatic *> items;
        };
        
        bool _partial;
        std::unordered_map<std::uint64_t, Node> _nodes;
        std::unordered_map<std::uint64_t, std::shared_ptr<YardStatic>> _statics;
        std::unordered_map<std::string, YardObjectType> _objectTypes;
        std::vector<std::shared_ptr<YardObjectImpl>> _objects;
    };
}

namespace voxel {
    std::shared_ptr<YardInterface> YardInterface::instance(
        const foundation::PlatformInterfacePtr &platform,
        const resource::MeshProviderPtr &meshProvider,
        const resource::TextureProviderPtr &textureProvider,
        const SceneInterfacePtr &scene
    ) {
        return std::make_shared<YardImpl>(platform, meshProvider, textureProvider, scene);
    }
}

namespace voxel {
    YardImpl::YardImpl(
        const foundation::PlatformInterfacePtr &platform,
        const resource::MeshProviderPtr &meshProvider,
        const resource::TextureProviderPtr &textureProvider,
        const SceneInterfacePtr &scene
    )
    : _platform(platform)
    , _meshProvider(meshProvider)
    , _textureProvider(textureProvider)
    , _scene(scene)
    , _partial(false)
    {
    }
    
    YardImpl::~YardImpl() {
    
    }
    
    void YardImpl::correctMovement(const math::vector3f &position, math::vector3f &movement) const {
        // collide
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
                                if (parameter == "partial" && bool(input >> std::boolalpha >> self->_partial) == false) {
                                    platform->logError("[YardImpl::loadYard] options block has invalid 'partial' syntax\n", id);
                                }
                                input >> std::ws;
                            }
                            
                            if (input.fail() != false) {
                                platform->logError("[YardImpl::loadYard] unable to load 'options' block\n", id);
                            }
                        }
                        if (type == "square" && bool(source >> id >> expect::braced(block, '{', '}'))) {
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
                                if (const resource::TextureInfo *info = self->_textureProvider->getTextureInfo(texture.data())) {
                                    bbox.xmin = position.x - 0.5;
                                    bbox.ymin = position.y - 0.5;
                                    bbox.zmin = position.z - 0.5;
                                    bbox.xmax = bbox.xmin + info->width;
                                    bbox.ymax = bbox.ymin + 1.0;
                                    bbox.zmax = bbox.zmin + info->height;
                                    self->_addStatic(id, std::make_shared<YardSquare>(*self, bbox, std::move(texture), std::move(heightmap)));
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
                            std::string model;
                            
                            while (input.eof() == false && input >> parameter) {
                                if (parameter == "model" && bool(input >> expect::braced(model, '"', '"')) == false) {
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
                                if (const resource::MeshInfo *info = self->_meshProvider->getMeshInfo(model.data())) {
                                    bbox.xmin = position.x - 0.5;
                                    bbox.ymin = position.y - 0.5;
                                    bbox.zmin = position.z - 0.5;
                                    bbox.xmax = bbox.xmin + info->sizeX;
                                    bbox.ymax = bbox.ymin + info->sizeY;
                                    bbox.zmax = bbox.zmin + info->sizeZ;
                                    self->_addStatic(id, std::make_shared<YardThing>(*self, bbox, std::move(model)));
                                }
                                else {
                                    platform->logError("[YardImpl::loadYard] mesh '%s' for thing with id = '%zu' has no info\n", model.data(), id);
                                }
                            }
                            else {
                                platform->logError("[YardImpl::loadYard] unable to load thing with id = '%zu'\n", id);
                            }
                        }
                        if (type == "object" && bool(source >> expect::braced(name, '"', '"') >> expect::braced(block, '{', '}'))) {
                            std::istringstream input = std::istringstream(block);
                            std::string model;
                            math::vector3f center;
                            
                            while (input.eof() == false && input >> parameter) {
                                if (parameter == "model" && bool(input >> expect::braced(model, '"', '"')) == false) {
                                    platform->logError("[YardImpl::loadYard] object type '%s' has invalid 'model' syntax\n", name.data());
                                }
                                if (parameter == "center" && bool(input >> center.x >> center.y >> center.z) == false) {
                                    platform->logError("[YardImpl::loadYard] object type '%s' has invalid 'center' syntax\n", name.data());
                                }
                                input >> std::ws;
                            }
                            
                            if (input.fail() == false) {
                                self->_objectTypes.emplace(name, YardObjectType{std::move(model), center});
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
                        platform->logError("[YardImpl::loadYard] yard '%s' isn't complete\n", yardpath.data());
                    }
                }
                else {
                    platform->logError("[YardImpl::loadYard] unable to load yard '%s'\n", yardpath.data());
                }

                completion(false);
            }
        });
    }
    
    std::shared_ptr<YardImpl::Object> YardImpl::addObject(const char *type, const math::vector3f &position, const math::vector3f &direction) {
        auto index = _objectTypes.find(type);
        if (index != _objectTypes.end()) {
            std::shared_ptr<YardObjectImpl> object = std::make_shared<YardObjectImpl>(*this, *this, index->second, position, direction);
            _objects.emplace_back(object);
        }
        else {
            _platform->logError("[YardImpl::addObject] unknown object type '%s'\n", type);
        }
        
        return nullptr;
    }
    
    void YardImpl::update(float dtSec) {
        if (_partial) {
        
        }
        else {
            for (auto &item : _statics) {
                item.second->updateState(YardStatic::State::RENDERING);
            }
        }
        
        for (auto &item : _objects) {
            item->update(dtSec);
        }
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
