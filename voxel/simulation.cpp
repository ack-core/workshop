
#include "simulation.h"

namespace voxel {
    class SimulationInterfaceImpl : public SimulationInterface {
    public:
        SimulationInterfaceImpl(const foundation::PlatformInterfacePtr &platform) {}
        ~SimulationInterfaceImpl() override {}
        
    public:
        void update(float dtSec) override {
            
        }

    public:

    };
}

namespace voxel {
    std::shared_ptr<SimulationInterface> SimulationInterface::instance(const foundation::PlatformInterfacePtr &platform) {
        return std::make_shared<SimulationInterfaceImpl>(platform);
    }
}

