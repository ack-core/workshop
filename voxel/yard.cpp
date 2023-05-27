
#include "yard.h"
#include "yard_object.h"
#include "yard_square.h"

#include "thirdparty/upng/upng.h"

namespace voxel {
    class YardImpl : public YardInterface {
    public:
        YardImpl(const foundation::PlatformInterfacePtr &platform, const MeshProviderPtr &meshProvider, const TextureProviderPtr &textureProvider, const SceneInterfacePtr &scene);
        ~YardImpl() override;
        
    public:
        bool loadYard(const char *src) override;
        auto addObject(const char *type) -> std::shared_ptr<Object> override;
        
    public:
        void _addSquare(int x, int z, const char *heightmap, const char *texture);
        void _addStatic(int x, int z, const char *model);
        
        foundation::PlatformInterfacePtr _platform;
        MeshProviderPtr _meshProvider;
        TextureProviderPtr _textureProvider;
        SceneInterfacePtr _scene;
        
        std::unique_ptr<YardSquare> _square;
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
    , _meshProvider(meshProvider)
    , _textureProvider(textureProvider)
    , _scene(scene)
    {
        _addSquare(0, 0, "hm01", "tx01");
    }
    
    YardImpl::~YardImpl() {
    
    }

    bool YardImpl::loadYard(const char *src) {
        
        return true;
    }
    
    std::shared_ptr<YardImpl::Object> YardImpl::addObject(const char *type) {
        return nullptr;
    }
    
    void YardImpl::_addSquare(int x, int z, const char *heightmap, const char *texture) {
        std::unique_ptr<std::uint8_t[]> hmdata;
        std::uint32_t hmwidth = 0;
        std::uint32_t hmheight = 0;
        
        if (_textureProvider->getOrLoadTextureData(heightmap, hmdata, hmwidth, hmheight)) {
            if (const foundation::RenderTexturePtr &t = _textureProvider->getOrLoad2DTexture(texture)) {
                _square = std::make_unique<YardSquare>(_scene, x, z, hmwidth, hmheight, std::move(hmdata), t);
            }
        }
    }
    
    void YardImpl::_addStatic(int x, int z, const char *model) {
    
    }
    
}
