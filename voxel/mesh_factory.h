
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform.h"

namespace voxel {
    struct VoxelPosition {
        std::int16_t positionX, positionY, positionZ, reserved;
    };
    struct VoxelInfo {
        std::int16_t positionX, positionY, positionZ, colorIndex;
        std::uint8_t lightFaceNX[4], lightFacePX[4]; // XFace Indeces : [-y-z, -y+z, +y-z, +y+z]
        std::uint8_t lightFaceNY[4], lightFacePY[4]; // YFace Indeces : [-z-x, -z+x, +z-x, +z+x]
        std::uint8_t lightFaceNZ[4], lightFacePZ[4]; // ZFace Indeces : [-y-x, -y+x, +y-x, +y+x]
    };
    
    struct Mesh {
        struct Frame {
            std::unique_ptr<VoxelPosition[]> positions;
            std::unique_ptr<VoxelInfo[]> voxels;
            std::uint16_t voxelCount;
        };
        
        std::unique_ptr<Frame[]> frames;
        std::uint16_t frameCount;
    };

    class MeshFactory {
    public:
        static std::shared_ptr<MeshFactory> instance(const foundation::PlatformInterfacePtr &platform);

    public:
        // Load voxels from file and fill 'output' Mesh with loaded data
        // @voxPath - absolute path to '.vox' file
        // @offset  - values to be added to voxel position
        // Notice: voxels on borders arent part of the mesh. They are needed to correct lighting
        //
        virtual bool createMesh(const char *voxPath, const int16_t(&offset)[3], Mesh &output) = 0;

    protected:
        virtual ~MeshFactory() = default;
    };

    using MeshFactoryPtr = std::shared_ptr<MeshFactory>;
}

