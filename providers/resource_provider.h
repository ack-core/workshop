
#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/layouts.h"

// TODO: util::Config instead of ParticlesParams, ParticlesParams -> back to scene

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
        // @return - info or nullptr
        //
        virtual const TextureInfo *getTextureInfo(const char *texPath) = 0;
        
        // Get mesh info
        // @voxPath - path to file without extension
        // @return - info or nullptr
        //
        virtual const MeshInfo *getMeshInfo(const char *voxPath) = 0;
        
        // Get ground info
        // @groundPath - path to file without extension
        // @return - info or nullptr
        //
        virtual const GroundInfo *getGroundInfo(const char *groundPath) = 0;
        
        // Asynchronously Load texture from file if it isn't loaded yet
        // @texturePath - path to file without extension
        // @return - texture object or nullptr
        //
        virtual void getOrLoadTexture(const char *texturePath, util::callback<void(const foundation::RenderTexturePtr &)> &&completion) = 0;
        
        // Asynchronously load voxels with VTXMVOX layout from file if they aren't loaded yet
        // @meshPath - path to file without extension
        // @return - mesh frames (zero size if not loaded)
        //
        virtual void getOrLoadVoxelMesh(const char *meshPath, util::callback<void(const std::vector<foundation::RenderDataPtr> &, const util::IntegerOffset3D &)> &&completion) = 0;
        
        // Asynchronously load ground from file if it isn't loaded yet
        // @groundPath - path to file without extension
        // @return - primitives to construct ground object or nullptr's
        //
        virtual void getOrLoadGround(const char *groundPath, util::callback<void(const foundation::RenderDataPtr &, const foundation::RenderTexturePtr &)> &&completion) = 0;

        // Asynchronously Load emitter from cfg file and textures if it isn't loaded yet
        // @cfgPath - path to file without extension
        // @return  - primitives to construct emitter or nullptrs
        //
        virtual void getOrLoadEmitter(const char *configPath, util::callback<void(const foundation::RenderTexturePtr &, const foundation::RenderTexturePtr &, const layouts::ParticlesParams *)> &&completion) = 0;
        
        // Force removing resources from internal storages
        //
        virtual void removeTexture(const char *texturePath) = 0;
        virtual void removeMesh(const char *meshPath) = 0;
        virtual void removeGround(const char *groundPath) = 0;
        virtual void removeEmitter(const char *configPath) = 0;

        // Provider tracks resources life time and tries to free them
        //
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~ResourceProvider() = default;
    };
    
    using ResourceProviderPtr = std::shared_ptr<ResourceProvider>;
}

