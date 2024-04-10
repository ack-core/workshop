
#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"

namespace resource {
    struct GroundInfo {
        int sizeX;
        int sizeY;
        int sizeZ;
    };
    
    struct GroundData : public GroundInfo {
        struct Vertex {
            float x, y, z;
            float nx, ny, nz;
            float u, v;
        };
    
        std::vector<Vertex> vertexes;
        std::vector<std::uint32_t> indexes;
        foundation::RenderTexturePtr texture;
    };
    
    class GroundProvider {
    public:
        static std::shared_ptr<GroundProvider> instance(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering);
        
    public:
        // Get ground info
        // @groundPath - path to file without extension
        // @return    - info or nullptr
        //
        virtual const GroundInfo *getGroundInfo(const char *groundPath) = 0;
        
        // Asynchronously load ground from file if it isn't loaded yet
        // @groundPath - path to file without extension
        // @return  - primitives to construct ground object or nullptr's
        //
        virtual void getOrLoadGround(const char *groundPath, util::callback<void(const std::unique_ptr<resource::GroundData> &)> &&completion) = 0;
        
        // Provider tracks resources life time and tries to free them
        //
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~GroundProvider() = default;
    };
    
    using GroundProviderPtr = std::shared_ptr<GroundProvider>;
}

