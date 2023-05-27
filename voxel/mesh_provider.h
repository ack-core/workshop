
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform.h"

namespace voxel {
    struct VoxelInfo {
        std::int16_t positionX, positionY, positionZ;
        std::uint8_t colorIndex, flags;
    };
    
    struct Mesh {
        struct Frame {
            std::unique_ptr<VoxelInfo[]> voxels;
            std::uint16_t voxelCount = 0;
        };
        
        std::unique_ptr<Frame[]> frames;
        std::uint16_t frameCount = 0;
    };
    
    class MeshProvider {
    public:
        static std::shared_ptr<MeshProvider> instance(const foundation::PlatformInterfacePtr &platform);
        
    public:
        // Load voxels from file if they aren't loaded yet
        // @name    - path to file without '.vox' extension
        // @offset  - value to be added to voxel positions
        // @return  - pointer to Mesh or nullptr
        //
        virtual auto getOrLoadVoxelMesh(const char *name, const int16_t(&offset)[3]) -> const std::unique_ptr<Mesh> & = 0;
        
    public:
        virtual ~MeshProvider() = default;
    };
    
    using MeshProviderPtr = std::shared_ptr<MeshProvider>;
}

