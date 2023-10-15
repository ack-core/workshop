
#include "simulation.h"

namespace voxel {
    class SimulationImpl : public SimulationInterface {
    public:
        SimulationImpl();
        ~SimulationImpl() override;
        
    public:
        auto addHoop(const math::vector3f &position, float radius) -> HoopPtr override;
        auto addWall() -> WallPtr override;
        void updateAndDraw(float dtSec) override;

    public:

    };
}

namespace voxel {
    SimulationImpl::SimulationImpl() {
    
    }
    SimulationImpl::~SimulationImpl() {
    
    }
        
    SimulationInterface::HoopPtr SimulationImpl::addHoop(const math::vector3f &position, float radius) {
        
    }
    SimulationInterface::WallPtr SimulationImpl::addWall() {
        
    }
    void SimulationImpl::updateAndDraw(float dtSec) {
        
    }
}

namespace voxel {
    std::shared_ptr<SimulationInterface> SimulationInterface::instance() {
        return std::make_shared<SimulationImpl>();
    }
}

