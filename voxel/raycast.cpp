
#include "raycast.h"

namespace voxel {
    class RaycastInterfaceImpl : public RaycastInterface {
    public:
        RaycastInterfaceImpl(const foundation::PlatformInterfacePtr &platform, const voxel::SceneInterfacePtr &scene) {}
        ~RaycastInterfaceImpl() override {}
        
    public:
        auto addSphereMesh(const std::vector<math::vector4f> &data) -> MeshPtr override {
            return nullptr;
        }
        auto addTriangleMesh(const std::vector<math::vector3f> &vtx, const std::vector<std::uint16_t> &idx) -> MeshPtr override {
            return nullptr;
        }
        bool sphereCast(const math::vector3f &start, const math::vector3f &target, float radius, float length) const override {
            return false;
        }
        void update(float dtSec) override {
            
        }

    public:

    };
}

namespace voxel {
    std::shared_ptr<RaycastInterface> RaycastInterface::instance(const foundation::PlatformInterfacePtr &platform, const voxel::SceneInterfacePtr &scene) {
        return std::make_shared<RaycastInterfaceImpl>(platform, scene);
    }
}

