
#include "state_manager.h"

#include "game.h"

#include <unordered_map>
#include <vector>

namespace game {
    class StateManagerImpl : public StateManager {
    public:
        StateManagerImpl(
            const foundation::PlatformInterfacePtr &platform,
            const voxel::SceneInterfacePtr &scene,
            const voxel::YardInterfacePtr &yard,
            const ui::StageInterfacePtr &ui,
            const dh::DataHubPtr &dh
        );
        ~StateManagerImpl() override;
        
        void switchToState(const char *name) override;
        void update(float dtSec) override;
        
    private:
        const foundation::PlatformInterfacePtr _platform;
        const voxel::SceneInterfacePtr _scene;
        const voxel::YardInterfacePtr _yard;
        const ui::StageInterfacePtr _ui;
        const dh::DataHubPtr _dh;
        
        std::string _currentStateName = "default";
        std::unordered_map<MakeContextFunc, std::unique_ptr<Context>> _currentContextList;
    };
    
    std::shared_ptr<StateManager> StateManager::instance(
        const foundation::PlatformInterfacePtr &platform,
        const voxel::SceneInterfacePtr &scene,
        const voxel::YardInterfacePtr &yard,
        const ui::StageInterfacePtr &ui,
        const dh::DataHubPtr &dh
    ) {
        return std::make_shared<StateManagerImpl>(platform, scene, yard, ui, dh);
    }
    
    StateManagerImpl::StateManagerImpl(
        const foundation::PlatformInterfacePtr &platform,
        const voxel::SceneInterfacePtr &scene,
        const voxel::YardInterfacePtr &yard,
        const ui::StageInterfacePtr &ui,
        const dh::DataHubPtr &dh
    )
    : _platform(platform)
    , _scene(scene)
    , _yard(yard)
    , _ui(ui)
    , _dh(dh)
    {
        switchToState("default");
    }
    
    StateManagerImpl::~StateManagerImpl() {
    
    }
    
    void StateManagerImpl::switchToState(const char *name) {
        auto index = states.find(name);
        if (index != states.end()) {
            std::unordered_map<MakeContextFunc, std::unique_ptr<Context>> _newContextList;
            
            for (auto& makeContextFunction : index->second) {
                auto index = _currentContextList.find(makeContextFunction);
                if (index == _currentContextList.end()) {
                    _newContextList.emplace(makeContextFunction, makeContextFunction(API{_platform, _scene, _yard, _ui, _dh}));
                }
                else {
                    _newContextList.emplace(index->first, std::move(index->second));
                }
            }
            
            std::swap(_currentContextList, _newContextList);
        }
        else {
            _platform->logError("[StateManager] '%s' state not found");
        }
    }
    
    void StateManagerImpl::update(float dtSec) {
        for (auto& contextEntry : _currentContextList) {
            contextEntry.second->update(dtSec);
        }
    }
}

