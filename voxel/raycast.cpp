
#include "raycast.h"

namespace voxel {
    class RaycastInterfaceImpl : public RaycastInterface {
    public:
        RaycastInterfaceImpl(const foundation::PlatformInterfacePtr &platform) {}
        ~RaycastInterfaceImpl() override {}
        
    public:
        auto addMesh(const util::Description &desc) -> MeshPtr override {
            return {};
        }
        bool sphereCast(const math::vector3f &start, const math::vector3f &target, float radius, float length) const override {
            return false;
        }
        bool rayCast(const math::vector3f &start, const math::vector3f &target, float length) const override {
            return false;
        }

    public:

    };
}

namespace voxel {
    std::shared_ptr<RaycastInterface> RaycastInterface::instance(const foundation::PlatformInterfacePtr &platform) {
        return std::make_shared<RaycastInterfaceImpl>(platform);
    }
}

