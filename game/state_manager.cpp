
#include "state_manager.h"
#include "game.h"

#include <unordered_map>
#include <vector>

namespace game {
    class StateManagerImpl : public StateManager, public std::enable_shared_from_this<StateManagerImpl> {
    public:
        StateManagerImpl(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering,
            const resource::ResourceProviderPtr &resourceProvider,
            const voxel::SceneInterfacePtr &scene,
            const voxel::SimulationInterfacePtr &simulation,
            const voxel::RaycastInterfacePtr &raycast,
            const ui::StageInterfacePtr &ui,
            const dh::DataHubPtr &dh
        );
        ~StateManagerImpl() override;
        
        void switchToState(const char *name) override;
        void update(float dtSec) override;
        
    private:
        const foundation::PlatformInterfacePtr _platform;
        const foundation::RenderingInterfacePtr _rendering;
        const resource::ResourceProviderPtr _resourceProvider;
        const voxel::SceneInterfacePtr _scene;
        const voxel::SimulationInterfacePtr _simulation;
        const voxel::RaycastInterfacePtr _raycast;
        const ui::StageInterfacePtr _ui;
        const dh::DataHubPtr _dh;
        
        std::string _currentStateName;
        std::unordered_map<MakeContextFunc, std::unique_ptr<Context>> _currentContextList;
    };
    
    std::shared_ptr<StateManager> StateManager::instance(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const resource::ResourceProviderPtr &resourceProvider,
        const voxel::SceneInterfacePtr &scene,
        const voxel::SimulationInterfacePtr &simulation,
        const voxel::RaycastInterfacePtr &raycast,
        const ui::StageInterfacePtr &ui,
        const dh::DataHubPtr &dh
    ) {
        return std::make_shared<StateManagerImpl>(platform, rendering, resourceProvider, scene, simulation, raycast, ui, dh);
    }
    
    StateManagerImpl::StateManagerImpl(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const resource::ResourceProviderPtr &resourceProvider,
        const voxel::SceneInterfacePtr &scene,
        const voxel::SimulationInterfacePtr &simulation,
        const voxel::RaycastInterfacePtr &raycast,
        const ui::StageInterfacePtr &ui,
        const dh::DataHubPtr &dh
    )
    : _platform(platform)
    , _rendering(rendering)
    , _resourceProvider(resourceProvider)
    , _scene(scene)
    , _simulation(simulation)
    , _ui(ui)
    , _dh(dh)
    {
    }
    
    StateManagerImpl::~StateManagerImpl() {
    
    }
    
    void StateManagerImpl::switchToState(const char *name) {
        for (auto index = std::begin(states); index != std::end(states); ++index) {
            if (::strcmp(name, index->name) == 0) {
                std::unordered_map<MakeContextFunc, std::unique_ptr<Context>> newContextList;

                for (auto& makeContextFunction : index->makers) {
                    auto index = _currentContextList.find(makeContextFunction);
                    if (index == _currentContextList.end()) {
                        newContextList.emplace(makeContextFunction, makeContextFunction(API{
                            _platform,
                            _rendering,
                            _resourceProvider,
                            _scene,
                            _simulation,
                            _raycast,
                            _ui,
                            _dh,
                            shared_from_this()
                        }));
                    }
                    else {
                        newContextList.emplace(index->first, std::move(index->second));
                    }
                }

                std::swap(_currentContextList, newContextList);
                return;
            }
        }
        
        _platform->logError("[StateManager] '%s' state not found");
    }
    
    void StateManagerImpl::update(float dtSec) {
        for (auto& contextEntry : _currentContextList) {
            contextEntry.second->update(dtSec);
        }
    }
}

