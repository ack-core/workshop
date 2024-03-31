
#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"

namespace resource {
    struct PlaceInfo {
        int sizeX;
        int sizeY;
        int sizeZ;
    };
    
    struct PlaceData : public PlaceInfo {
        struct Vertex {
            float x, y, z;
            float nx, ny, nz;
            float u, v;
        };
    
        std::vector<Vertex> vertexes;
        std::vector<std::uint32_t> indexes;
        foundation::RenderTexturePtr texture;
    };
    
    class PlaceProvider {
    public:
        static std::shared_ptr<PlaceProvider> instance(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering);
        
    public:
        // Get place info
        // @placePath - path to file without extension
        // @return    - info or nullptr
        //
        virtual const PlaceInfo *getPlaceInfo(const char *placePath) = 0;
        
        // Asynchronously load place from file if it isn't loaded yet
        // @placePath - path to file without extension
        // @return  - primitives to construct place object or nullptr's
        //
        virtual void getOrLoadPlace(const char *placePath, util::callback<void(const std::unique_ptr<resource::PlaceData> &)> &&completion) = 0;
        
        // Provider tracks resources life time and tries to free them
        //
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~PlaceProvider() = default;
    };
    
    using PlaceProviderPtr = std::shared_ptr<PlaceProvider>;
}

