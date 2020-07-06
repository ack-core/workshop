
#include <cstddef>
#include <cstdint>
#include <memory>

namespace voxel {
    class Mesh {
    public:


    protected:
        virtual ~Mesh() = default;
    };

    class MeshContainer {
    public:
        static std::shared_ptr<MeshContainer> instance();

    public:
        virtual std::shared_ptr<Mesh> createMesh(const char *resourcePath);

    protected:
        virtual ~MeshContainer() = default;
    };
}