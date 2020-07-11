
#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform/interfaces.h"
#include "foundation/rendering/interfaces.h"

namespace voxel {
    class StaticMesh {
    public:
        enum class Rotation : std::uint32_t {
            Rotate90,
            Rotate180,
            Rotate270,
        };

        virtual void applyTransform(int x, int y, int z, Rotation rotation) = 0;
        virtual const std::shared_ptr<foundation::RenderingStructuredData> &getGeometry() const = 0;

    protected:
        virtual ~StaticMesh() = default;
    };

    class DynamicMesh {
    public:
        virtual void setTransform(const float (&position)[3], float rotationXZ) = 0;
        virtual void playAnimation(const char *name, std::function<void(DynamicMesh &)> &&finished = nullptr, bool cycled = false) = 0;

        virtual void update(float dtSec) = 0;

        virtual const float(&getTransform() const)[16] = 0;
        virtual const std::shared_ptr<foundation::RenderingStructuredData> &getGeometry() const = 0;

    protected:
        virtual ~DynamicMesh() = default;
    };

    class MeshFactory {
    public:
        static std::shared_ptr<MeshFactory> instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering);

    public:
        virtual std::shared_ptr<StaticMesh> createStaticMesh(const char *resourcePath) = 0;
        virtual std::shared_ptr<DynamicMesh> createDynamicMesh(const char *resourcePath) = 0;

        virtual std::shared_ptr<StaticMesh> mergeStaticMeshes(const std::vector<std::shared_ptr<StaticMesh>> &meshes) = 0;

    protected:
        virtual ~MeshFactory() = default;
    };
}