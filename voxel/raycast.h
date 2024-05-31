
#pragma once
#include <memory>

namespace voxel {
    class RaycastInterface {
    public:
        static std::shared_ptr<RaycastInterface> instance();
        
    public:
        virtual void updateAndDraw(float dtSec) = 0;
        
    public:
        virtual ~RaycastInterface() = default;
    };
    
    using RaycastInterfacePtr = std::shared_ptr<RaycastInterface>;
}

