
#include "raycast.h"

namespace voxel {
    class RaycastInterfaceImpl : public RaycastInterface {
    public:
        RaycastInterfaceImpl() {}
        ~RaycastInterfaceImpl() override {}
        
    public:
        void updateAndDraw(float dtSec) override {}

    public:

    };
}

namespace voxel {
    std::shared_ptr<RaycastInterface> RaycastInterface::instance() {
        return std::make_shared<RaycastInterfaceImpl>();
    }
}

