
#include "state_manager.h"

#include "contexts/context.h"
#include "contexts/debug_context.h"

#include <unordered_map>
#include <vector>

namespace game {
    using MakeContextFunc = std::unique_ptr<Context>(*)(const voxel::SceneInterfacePtr &, const ui::StageInterfacePtr &);
    // Rule of states:
    // Context is created if the next state contains it and previous state does not
    // Context is deleted if the next state does not contain it
    const std::unordered_map<const char *, std::vector<MakeContextFunc>> states = {
        {"default",
            {
                &makeContext<DebugContext>
            }
        },
    };
}

namespace game {
    class StateManagerImpl : public StateManager {
    public:
        StateManagerImpl(const voxel::SceneInterfacePtr &scene, const ui::StageInterfacePtr &ui);
        ~StateManagerImpl() override;
        
        void switchToState(const char *name) override;
        void update(float dtSec) override;
        
    private:
        const foundation::PlatformInterfacePtr _platform;
        const voxel::SceneInterfacePtr _scene;
        const ui::StageInterfacePtr _ui;
        
        std::string _currentStateName = "default";
        std::unordered_map<MakeContextFunc, std::unique_ptr<Context>> _currentContextList;
    };
    
    std::shared_ptr<StateManager> StateManager::instance(const voxel::SceneInterfacePtr &scene, const ui::StageInterfacePtr &ui) {
        return std::make_shared<StateManagerImpl>(scene, ui);
    }
    
    StateManagerImpl::StateManagerImpl(const voxel::SceneInterfacePtr &scene, const ui::StageInterfacePtr &ui)
    : _platform(scene->getPlatformInterface())
    , _scene(scene)
    , _ui(ui)
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
                    _newContextList.emplace(makeContextFunction, makeContextFunction(_scene, _ui));
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

