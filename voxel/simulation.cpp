
#include "simulation.h"

namespace voxel {
    class SimulationImpl : public SimulationInterface {
    public:
        SimulationImpl();
        ~SimulationImpl() override;
        
    public:
        auto addActor(const math::vector3f &position, float radius) -> ActorPtr override;
        auto addFence() -> FencePtr override;
        void update(float dtSec) override;

    public:

    };
}

namespace voxel {
    SimulationImpl::SimulationImpl() {
    
    }
    SimulationImpl::~SimulationImpl() {
    
    }
        
    SimulationInterface::ActorPtr SimulationImpl::addActor(const math::vector3f &position, float radius) {
        return nullptr;
    }
    SimulationInterface::FencePtr SimulationImpl::addFence() {
        return nullptr;        
    }
    void SimulationImpl::update(float dtSec) {
        
    }
}

namespace voxel {
    std::shared_ptr<SimulationInterface> SimulationInterface::instance() {
        return std::make_shared<SimulationImpl>();
    }
}

