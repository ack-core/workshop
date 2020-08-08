
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform/interfaces.h"
#include "foundation/rendering/interfaces.h"

namespace voxel {
    struct Voxel {
        std::int16_t positionX, positionY, positionZ, reserved;
        std::uint8_t sizeX, sizeY, sizeZ, colorIndex;
        //std::uint8_t colorIndex, darkness, metallicity;
    };

    class StaticMesh {
    public:
        virtual const std::shared_ptr<foundation::RenderingStructuredData> &getGeometry() const = 0;

    protected:
        virtual ~StaticMesh() = default;
    };

    class DynamicMesh {
    public:
        virtual void setTransform(const float (&position)[3], float rotationXZ) = 0;
        virtual void playAnimation(const char *name, std::function<void(DynamicMesh &)> &&finished = nullptr, bool cycled = false, bool resetAnimationTime = true) = 0;

        virtual void update(float dtSec) = 0;

        virtual const float(&getTransform() const)[16] = 0;
        virtual const std::shared_ptr<foundation::RenderingStructuredData> &getGeometry() const = 0;
        virtual const std::uint32_t getFrameStartIndex() const = 0;
        virtual const std::uint32_t getFrameSize() const = 0;

    protected:
        virtual ~DynamicMesh() = default;
    };

    class MeshFactory {
    public:
        static std::shared_ptr<MeshFactory> instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering);

    public:
        enum class Rotation : std::uint32_t {
            Rotate0,
            Rotate90,
            Rotate180,
            Rotate270,
        };

        virtual bool loadVoxels(const char *voxFullPath, int x, int y, int z, Rotation rotation, std::vector<Voxel> &out) = 0;

        virtual std::shared_ptr<StaticMesh> createStaticMesh(const std::vector<Voxel> &voxels) = 0;
        virtual std::shared_ptr<DynamicMesh> createDynamicMesh(const char *resourcePath, const float(&position)[3], float rotationXZ) = 0;

    protected:
        virtual ~MeshFactory() = default;
    };
}