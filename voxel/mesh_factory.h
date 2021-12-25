
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform.h"
#include "foundation/rendering.h"

namespace voxel {
    enum class Rotation {
        Rotate0,
        Rotate90,
        Rotate180,
        Rotate270,
    };

    struct Voxel {
        std::int16_t positionX, positionY, positionZ, reserved;
        std::uint8_t sizeX, sizeY, sizeZ, colorIndex;
    };

    class StaticMesh {
    public:
        virtual void updateAndDraw(float dtSec) = 0;

    protected:
        virtual ~StaticMesh() = default;
    };

    class DynamicMesh {
    public:
        virtual void updateAndDraw(float dtSec) = 0;
        virtual void setTransform(const float(&position)[3], float rotationXZ) = 0;
        virtual void playAnimation(const char *name, std::function<void(DynamicMesh &)> &&finished = nullptr, bool cycled = false, bool resetAnimationTime = true) = 0;

    protected:
        virtual ~DynamicMesh() = default;
    };

    class MeshFactory {
    public:
        static std::shared_ptr<MeshFactory> instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering, const char *palettePath);

    public:
        virtual bool appendVoxels(const char *voxFullPath, int x, int y, int z, Rotation rotation, std::vector<Voxel> &target) = 0;

        virtual std::shared_ptr<StaticMesh> createStaticMesh(std::vector<Voxel> &voxels) = 0;
        virtual std::shared_ptr<DynamicMesh> createDynamicMesh(const char *resourcePath, const float(&position)[3], float rotationXZ) = 0;

    protected:
        virtual ~MeshFactory() = default;
    };

    using MeshFactoryPtr = std::shared_ptr<MeshFactory>;
    using StaticMeshPtr = std::shared_ptr<StaticMesh>;
    using DynamicMeshPtr = std::shared_ptr<DynamicMesh>;
}

