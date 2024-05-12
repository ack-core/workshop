
#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"

namespace resource {
    struct MeshInfo {
        int sizeX;
        int sizeY;
        int sizeZ;
    };
    
    class MeshProvider {
    public:
        static std::shared_ptr<MeshProvider> instance(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering);
        
    public:
        // Get mesh info
        // @voxPath - path to file without extension
        // @return  - info or nullptr
        //
        virtual const MeshInfo *getMeshInfo(const char *voxPath) = 0;

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
        
        // Provider tracks resources life time and tries to free them
        //
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~MeshProvider() = default;
    };
    
    using MeshProviderPtr = std::shared_ptr<MeshProvider>;
}
