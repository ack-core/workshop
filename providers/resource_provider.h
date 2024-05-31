
#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"

namespace resource {
    struct TextureInfo {
        std::uint32_t width;
        std::uint32_t height;
        foundation::RenderTextureFormat format;
    };
    
    struct MeshInfo {
        std::uint32_t sizeX;
        std::uint32_t sizeY;
        std::uint32_t sizeZ;
    };
    
    struct GroundInfo {
        std::uint32_t sizeX;
        std::uint32_t sizeY;
        std::uint32_t sizeZ;
    };
    
    class ResourceProvider {
    public:
        static std::shared_ptr<ResourceProvider> instance(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering);
        
    public:
        // Get texture info
        // @texPath - path to file without extension
        // @return  - info or nullptr
        //
        virtual const TextureInfo *getTextureInfo(const char *texPath) = 0;
        
        // Get mesh info
        // @voxPath - path to file without extension
        // @return  - info or nullptr
        //
        virtual const MeshInfo *getMeshInfo(const char *voxPath) = 0;
        
        // Get ground info
        // @groundPath - path to file without extension
        // @return    - info or nullptr
        //
        virtual const GroundInfo *getGroundInfo(const char *groundPath) = 0;
        
        // Asynchronously Load texture from file if it isn't loaded yet
        // @texPath - path to file without extension
        // @return  - texture object or nullptr
        //
        virtual void getOrLoadTexture(const char *texPath, util::callback<void(const foundation::RenderTexturePtr &)> &&completion) = 0;
        
        // Asynchronously load voxels with VTXSVOX layout from file if they aren't loaded yet
        // @voxPath - path to file without extension
        // @return  - pointer to mesh or nullptr
        //
        virtual void getOrLoadVoxelStatic(const char *voxPath, util::callback<void(const foundation::RenderDataPtr &)> &&completion) = 0;

        // Asynchronously load voxels with VTXDVOX layout from file if they aren't loaded yet
        // @voxPath - path to file without extension
        // @return  - mesh frames (zero size if not loaded)
        //
        virtual void getOrLoadVoxelObject(const char *voxPath, util::callback<void(const std::vector<foundation::RenderDataPtr> &)> &&completion) = 0;
        
        // Asynchronously load ground from file if it isn't loaded yet
        // @groundPath - path to file without extension
        // @return  - primitives to construct ground object or nullptr's
        //
        virtual void getOrLoadGround(const char *groundPath, util::callback<void(const foundation::RenderDataPtr &, const foundation::RenderTexturePtr &)> &&completion) = 0;
        
        // Provider tracks resources life time and tries to free them
        //
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~ResourceProvider() = default;
    };
    
    using ResourceProviderPtr = std::shared_ptr<ResourceProvider>;
}

