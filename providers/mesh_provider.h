
#pragma once
#include "foundation/util.h"
#include "foundation/math.h"
#include "foundation/platform.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace resource {
    struct MeshInfo {
        int sizeX;
        int sizeY;
        int sizeZ;
    };
    
    struct VoxelMesh {
        struct Voxel {
            std::int16_t positionX, positionY, positionZ;
            std::uint8_t colorIndex, mask, scaleX, scaleY, scaleZ;
        };
        struct Frame {
            std::unique_ptr<Voxel[]> voxels;
            std::uint16_t voxelCount = 0;
        };
        
        std::unique_ptr<Frame[]> frames;
        std::uint16_t frameCount = 0;
    };
    
    class MeshProvider {
    public:
        static std::shared_ptr<MeshProvider> instance(const foundation::PlatformInterfacePtr &platform, const char *resourceList);
        
    public:
        // Get mesh info
        // @voxPath - path to file without extension
        // @return  - info or nullptr
        //
        virtual const MeshInfo *getMeshInfo(const char *voxPath) = 0;
        
        // Load voxels from file if they aren't loaded yet
        // @voxPath - path to file without extension
        // @scaledVoxels  - optimization using scaled voxels
        // @return  - pointer to Mesh or nullptr
        //
        virtual void getOrLoadVoxelMesh(const char *voxPath, bool scaledVoxels, util::callback<void(const std::unique_ptr<VoxelMesh> &)> &&completion) = 0;
                
    public:
        virtual ~MeshProvider() = default;
    };
    
    using MeshProviderPtr = std::shared_ptr<MeshProvider>;
}

