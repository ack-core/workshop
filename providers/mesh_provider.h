
#pragma once
#include "foundation/platform.h"

namespace resource {
    struct MeshInfo {
        int sizeX;
        int sizeY;
        int sizeZ;
    };
    
    struct VoxelMesh {
        struct Voxel {
            std::int16_t positionX, positionY, positionZ;
            std::uint8_t colorIndex, mask, scaleX, scaleY, scaleZ, reserved;
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
        static std::shared_ptr<MeshProvider> instance(const foundation::PlatformInterfacePtr &platform);
        
    public:
        // Get mesh info
        // @voxPath - path to file without extension
        // @return  - info or nullptr
        //
        virtual const MeshInfo *getMeshInfo(const char *voxPath) = 0;
        
        // Asynchronously load voxels from file if they aren't loaded yet
        // @voxPath - path to file without extension
        // @return  - pointer to Mesh or nullptr
        //
        virtual void getOrLoadVoxelMesh(const char *voxPath, util::callback<void(const std::unique_ptr<resource::VoxelMesh> &)> &&completion) = 0;
        
        // Provider tracks resources life time and tries to free them
        //
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~MeshProvider() = default;
    };
    
    using MeshProviderPtr = std::shared_ptr<MeshProvider>;
}

