
#include "yard_base.h"
#include "yard_square.h"
#include "yard_thing.h"
#include "yard.h"
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
    class YardImpl : public YardInterface, public YardInterfaceProvider {
    public:
        YardImpl(const foundation::PlatformInterfacePtr &platform, const MeshProviderPtr &meshProvider, const TextureProviderPtr &textureProvider, const SceneInterfacePtr &scene);
        ~YardImpl() override;
        
    public:
        const foundation::LoggerInterfacePtr &getLogger() const override { return _logger; }
        const MeshProviderPtr &getMeshProvider() const override { return _meshProvider; }
        const TextureProviderPtr &getTextureProvider() const override { return _textureProvider; }
        const SceneInterfacePtr &getScene() const override { return _scene; }

    public:
        bool loadYard(const char *src) override;
        auto addObject(const char *type) -> std::shared_ptr<Object> override;
        void update(float dtSec) override;
        
    public:
        void _addStatic(std::uint64_t id, std::unique_ptr<YardStatic> &&object);
        void _clearYard();
        
    public:
        const foundation::PlatformInterfacePtr _platform;
        const foundation::LoggerInterfacePtr _logger;
        const MeshProviderPtr _meshProvider;
        const TextureProviderPtr _textureProvider;
        const SceneInterfacePtr _scene;
        
        struct Node {
            std::vector<YardStatic *> items;
        };
        struct ObjectType {
            std::string model;
        };
        
        bool _partial;
        std::unordered_map<std::uint64_t, Node> _nodes;
        std::unordered_map<std::uint64_t, std::unique_ptr<YardStatic>> _statics;
        std::unordered_map<std::string, ObjectType> _objectTypes;
        std::vector<std::shared_ptr<YardObjectImpl>> _objects;
    };
}

namespace voxel {
    std::shared_ptr<YardInterface> YardInterface::instance(
        const foundation::PlatformInterfacePtr &platform,
        const MeshProviderPtr &meshProvider,
        const TextureProviderPtr &textureProvider,
        const SceneInterfacePtr &scene
    ) {
        return std::make_shared<YardImpl>(platform, meshProvider, textureProvider, scene);
    }
}

namespace voxel {
    YardImpl::YardImpl(
        const foundation::PlatformInterfacePtr &platform,
        const MeshProviderPtr &meshProvider,
        const TextureProviderPtr &textureProvider,
        const SceneInterfacePtr &scene
    )
    : _platform(platform)
    , _logger(platform)
    , _meshProvider(meshProvider)
    , _textureProvider(textureProvider)
    , _scene(scene)
    , _partial(false)
    {
    }
    
    YardImpl::~YardImpl() {
    
    }

    bool YardImpl::loadYard(const char *sourcepath) {
        std::string yardpath = std::string(sourcepath) + ".yard";
        std::unique_ptr<std::uint8_t[]> binary;
        std::size_t size;
        
        _clearYard();
        
        if (_platform->loadFile(yardpath.data(), binary, size)) {
            std::istringstream source = std::istringstream((const char *)binary.get());
            std::uint64_t id;
            std::string type, block, name, parameter;
            std::unordered_map<std::uint64_t, std::vector<std::uint64_t>> links;
            math::bound3f bbox;
            
            while (source >> type) {
                if (type == "options" && bool(source >> expect::braced(block, '{', '}'))) {
                    std::istringstream input = std::istringstream(block);
                    
                    while (input.eof() == false && input >> parameter) {
                        if (parameter == "partial" && bool(input >> std::boolalpha >> _partial) == false) {
                            _platform->logError("[YardImpl::loadYard] options block has invalid 'partial' syntax\n", id);
                        }
                        input >> std::ws;
                    }
                    
                    if (input.fail() != false) {
                        _platform->logError("[YardImpl::loadYard] unable to load 'options' block\n", id);
                    }
                }
                if (type == "square" && bool(source >> id >> expect::braced(block, '{', '}'))) {
                    std::istringstream input = std::istringstream(block);
                    std::string texture, heightmap;
                    
                    while (input.eof() == false && input >> parameter) {
                        if (parameter == "texture" && bool(input >> expect::braced(texture, '"', '"')) == false) {
                            _platform->logError("[YardImpl::loadYard] square with id = '%zu' has invalid 'texture' syntax\n", id);
                        }
                        if (parameter == "heightmap" && bool(input >> expect::braced(heightmap, '"', '"')) == false) {
                            _platform->logError("[YardImpl::loadYard] square with id = '%zu' has invalid 'heightmap' syntax\n", id);
                        }
                        if (parameter == "position" && bool(input >> bbox.xmin >> bbox.ymin >> bbox.zmin >> bbox.xmax >> bbox.ymax >> bbox.zmax) == false) {
                            _platform->logError("[YardImpl::loadYard] square with id = '%zu' has invalid 'position' syntax\n", id);
                        }
                        if (parameter == "link" && bool(input >> expect::nlist(links[id])) == false) {
                            _platform->logError("[YardImpl::loadYard] square with id = '%zu' has invalid 'links' syntax\n", id);
                        }
                        input >> std::ws;
                    }
                    
                    if (input.fail() == false) {
                        bbox.xmin -= 0.5f; bbox.ymin -= 0.5f; bbox.zmin -= 0.5f;
                        bbox.xmax += 0.5f; bbox.ymax += 0.5f; bbox.zmax += 0.5f;
                        _addStatic(id, std::make_unique<YardSquare>(*this, bbox, std::move(texture), std::move(heightmap)));
                    }
                    else {
                        _platform->logError("[YardImpl::loadYard] unable to load square with id = '%zu'\n", id);
                    }
                }
                if (type == "object" && bool(source >> expect::braced(name, '"', '"') >> expect::braced(block, '{', '}'))) {
                    std::istringstream input = std::istringstream(block);
                    std::string model;
                    
                    while (input.eof() == false && input >> parameter) {
                        if (parameter == "model" && bool(input >> expect::braced(model, '"', '"')) == false) {
                            _platform->logError("[YardImpl::loadYard] object type '%s' has invalid 'model' syntax\n", name.data());
                        }
                        input >> std::ws;
                    }
                    
                    if (input.fail() == false) {
                        _objectTypes.emplace(name, ObjectType{std::move(model)});
                    }
                    else {
                        _platform->logError("[YardImpl::loadYard] unable to load object type '%s'\n", name.data());
                    }
                }
                
                block.clear();
            }
            
            for (auto &item : _statics) {
                for (auto &link : links[item.first]) {
                    item.second->linkTo(_statics[link].get());
                }
            }
            
            return true;
        }
        else {
            _platform->logError("[YardImpl::loadYard] unable to load yard '%s'\n", yardpath.data());
        }
        
        return false;
    }
    
    std::shared_ptr<YardImpl::Object> YardImpl::addObject(const char *type) {
        return nullptr;
    }
    
    void YardImpl::update(float dtSec) {
        if (_partial) {
        
        }
        else {
            for (auto &item : _statics) {
                item.second->setState(YardStatic::State::RENDERED);
            }
        }
    }
    
    void YardImpl::_addStatic(std::uint64_t id, std::unique_ptr<YardStatic> &&object) {
        const math::bound3f &bbox = object->getBBox();
        int startx = int(bbox.xmin / GLOBAL_COORD_DIV);
        int startz = int(bbox.zmin / GLOBAL_COORD_DIV);
        int endx = int(bbox.xmax / GLOBAL_COORD_DIV);
        int endz = int(bbox.zmax / GLOBAL_COORD_DIV);
        
        if (_statics.find(id) == _statics.end()) {
            auto index = _statics.emplace(id, std::move(object));
        
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
