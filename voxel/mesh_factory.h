
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform.h"

namespace voxel {
    struct VoxelInfo {
        std::int16_t positionX, positionY, positionZ, colorIndex;
    };
    
    struct Mesh {
        struct Frame {
            std::unique_ptr<VoxelInfo[]> voxels;
            std::uint16_t voxelCount = 0;
        };
        
        std::unique_ptr<Frame[]> frames;
        std::uint16_t frameCount = 0;
    };
    
    class MeshFactory {
    public:
        static std::shared_ptr<MeshFactory> instance(const foundation::PlatformInterfacePtr &platform);
        
    public:
        // Load voxels from file and fill 'output' Mesh with loaded data
        // @voxPath - absolute path to '.vox' file
        // @offset  - values to be added to voxel position
        //
        virtual bool createMesh(const char *voxPath, const int16_t(&offset)[3], Mesh &output) = 0;
        
    protected:
        virtual ~MeshFactory() = default;
    };
    
    using MeshFactoryPtr = std::shared_ptr<MeshFactory>;
}

