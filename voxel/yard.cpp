
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
        //std::unique_ptr<YardStatic> _addSquare(int x, int y, int z, const char *texture, const char *heightmap);
        //std::unique_ptr<YardStatic> _addThing(int x, int y, int z, const char *model);
        
        const foundation::PlatformInterfacePtr _platform;
        const foundation::LoggerInterfacePtr _logger;
        const MeshProviderPtr _meshProvider;
        const TextureProviderPtr _textureProvider;
        const SceneInterfacePtr _scene;
        
        struct Node {
            std::unique_ptr<YardStatic> object;
            std::vector<Node *> links;
        };
        struct Entry {
            std::vector<Node *> nodes;
        };
        
        std::unordered_map<std::uint64_t, Entry> _nodes;
        std::unique_ptr<YardStatic> _square;
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
    {
    }
    
    YardImpl::~YardImpl() {
    
    }

    bool YardImpl::loadYard(const char *sourcepath) {
        std::unique_ptr<std::uint8_t[]> binary;
        std::size_t size;
        
        if (_platform->loadFile((std::string(sourcepath) + ".yard").data(), binary, size)) {
            std::istringstream source = std::istringstream((const char *)binary.get());
            std::string type;
            std::string block;
            std::uint64_t id;
            
            while (source >> type >> id >> expect::braced(block, '{', '}')) {
                if (type == "square") {
                    std::istringstream input = std::istringstream(block);
                    std::string parameter;
                    std::string texture;
                    std::string heightmap;
                    std::vector<std::uint64_t> links;
                    math::bound3f bbox;
                    
                    while (input.eof() == false && input >> parameter) {
                        if (parameter == "texture" && bool(input >> expect::braced(texture, '"', '"')) == false) {
                            _platform->logError("[YardImpl::loadYard] square with id = '%zu' has invalid texture syntax\n", id);
                        }
                        if (parameter == "heightmap" && bool(input >> expect::braced(heightmap, '"', '"')) == false) {
                            _platform->logError("[YardImpl::loadYard] square with id = '%zu' has invalid heightmap syntax\n", id);
                        }
                        if (parameter == "position" && bool(input >> bbox.xmin >> bbox.ymin >> bbox.zmin >> bbox.xmax >> bbox.ymax >> bbox.zmax) == false) {
                            _platform->logError("[YardImpl::loadYard] square with id = '%zu' has invalid position syntax\n", id);
                        }
                        if (parameter == "link" && bool(input >> expect::nlist(links)) == false) {
                            _platform->logError("[YardImpl::loadYard] square with id = '%zu' has invalid links syntax\n", id);
                        }
                        input >> std::ws;
                    }
                    
                    if (input.fail() == false) {
                        
                        bbox.xmin -= 0.5f; bbox.ymin -= 0.5f; bbox.zmin -= 0.5f;
                        bbox.xmax += 0.5f; bbox.ymax += 0.5f; bbox.zmax += 0.5f;
                        _square = std::make_unique<YardSquare>(*this, bbox, std::move(texture), std::move(heightmap));
                    }
                    else {
                        _platform->logError("[YardImpl::loadYard] unable to load square with id = '%zu'\n", id);
                    }
                }
            }
        }
        else {
        
        }
            
        return true;
    }
    
    std::shared_ptr<YardImpl::Object> YardImpl::addObject(const char *type) {
        return nullptr;
    }
    
    void YardImpl::update(float dtSec) {
//        if (_square == nullptr) {
//            _square = std::make_unique<YardSquare>(*this, 0, 0, 0, std::string("textures/yards/dungeon_room00"), std::string());
//        }
//        else {
            _square->setState(YardStatic::State::RENDERED);
//        }


        
    }
    
    
    
}
