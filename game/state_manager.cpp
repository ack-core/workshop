
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
            const voxel::WorldInterfacePtr &world,
            const voxel::RaycastInterfacePtr &raycast,
            const voxel::SimulationInterfacePtr &simulation,
            const ui::StageInterfacePtr &ui,
            const dh::DataHubPtr &dh
        )
        : _platform(platform)
        , _rendering(rendering)
        , _resourceProvider(resourceProvider)
        , _scene(scene)
        , _world(world)
        , _ui(ui)
        , _dh(dh)
        {
        }
        ~StateManagerImpl() override {}
        
        void switchToState(const char *name) override;
        void update(float dtSec) override;
        
    private:
        const foundation::PlatformInterfacePtr _platform;
        const foundation::RenderingInterfacePtr _rendering;
        const resource::ResourceProviderPtr _resourceProvider;
        const voxel::SceneInterfacePtr _scene;
        const voxel::WorldInterfacePtr _world;
        const voxel::RaycastInterfacePtr _raycast;
        const ui::StageInterfacePtr _ui;
        const dh::DataHubPtr _dh;
        
        std::string _currentStateName;
        std::unordered_map<MakeContextFunc, std::shared_ptr<Context>> _currentContextList;
    };
    
    std::shared_ptr<StateManager> StateManager::instance(
        const foundation::PlatformInterfacePtr &platform,
        const foundation::RenderingInterfacePtr &rendering,
        const resource::ResourceProviderPtr &resourceProvider,
        const voxel::SceneInterfacePtr &scene,
        const voxel::WorldInterfacePtr &world,
        const voxel::RaycastInterfacePtr &raycast,
        const voxel::SimulationInterfacePtr &simulation,
        const ui::StageInterfacePtr &ui,
        const dh::DataHubPtr &dh
    )
    {
        return std::make_shared<StateManagerImpl>(platform, rendering, resourceProvider, scene, world, raycast, simulation, ui, dh);
    }
    
    void StateManagerImpl::switchToState(const char *name) {
        for (auto index = std::begin(STATES); index != std::end(STATES); ++index) {
            if (::strcmp(name, index->name) == 0) {
                std::unordered_map<MakeContextFunc, std::shared_ptr<Context>> newContextList;
                std::vector<Interface *> interfaces;
                
                for (auto& makeContextFunction : index->makers) {
                    std::shared_ptr<Context> ctx = nullptr;
                    
                    auto index = _currentContextList.find(makeContextFunction);
                    if (index == _currentContextList.end()) {
                        ctx = makeContextFunction(API { _platform, _rendering, _resourceProvider, _scene, _world, _raycast, _ui, _dh, shared_from_this() }, interfaces.data(), interfaces.size());
                    }
                    else {
                        ctx = index->second;
                    }
                    
                    if (ctx) {
                        if (Interface *iface = dynamic_cast<Interface *>(ctx.get())) {
                            interfaces.push_back(iface);
                        }
                        
                        newContextList.emplace(makeContextFunction, std::move(ctx));
                    }
                    else {
                        _platform->logError("[StateManager] Unexpected state while switching to '%s'");
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

