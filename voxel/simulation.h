
#pragma once
#include <memory>
#include "foundation/math.h"
#include "foundation/platform.h"

namespace voxel {
    class SimulationInterface {
    public:
        static std::shared_ptr<SimulationInterface> instance(
          const foundation::PlatformInterfacePtr &platform
      );
        
    public:
        
    public:
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~SimulationInterface() = default;
    };
    
    using SimulationInterfacePtr = std::shared_ptr<SimulationInterface>;
}

