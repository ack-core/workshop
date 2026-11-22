
#include "context.h"
#include "state_manager.h"

#include <algorithm>
#include <unordered_map>
#include <utility>
#include <vector>

#if __has_include("game.h")
#include "game.h"
#elif defined(IS_EDITOR)
#include "editor/editor_main_context.h"
#include "editor/editor_camera_context.h"
#include "editor/editor_prefab_context.h"
#include "editor/editor_voxel_mesh_context.h"
#include "editor/editor_ground_context.h"
#include "editor/editor_particles_context.h"
#include "editor/editor_raycast_shape_context.h"
#include "editor/editor_collision_shape_context.h"

namespace game {
    inline const State STATES[] = {
        {"default", {
            &makeContext<EditorCameraContext>,
            &makeContext<EditorMainContext, CameraAccessInterface>,
            &makeContext<EditorPrefabContext, NodeAccessInterface>,
            &makeContext<EditorVoxelMeshContext, NodeAccessInterface>,
            &makeContext<EditorGroundContext, NodeAccessInterface>,
            &makeContext<EditorRaycastShapeContext, NodeAccessInterface, CameraAccessInterface>,
            &makeContext<EditorCollisionShapeContext, NodeAccessInterface, CameraAccessInterface>,
            &makeContext<EditorParticlesContext, NodeAccessInterface, CameraAccessInterface>
        }}
    };
}
#else
#include "experimental/render_dev.h"
#include "experimental/vegetation.h"
#include "experimental/voxel_meshes.h"
#include "experimental/uielements.h"
#include "experimental/collision.h"

namespace game {
    inline const State STATES[] = {
        {"default", {
            &makeContext<RenderDevContext>
        }},
        {"vegetation", {
            &makeContext<RenderDevContext>,
            &makeContext<VegetationContext>
        }},
        {"voxelmeshes", {
            &makeContext<RenderDevContext>,
            &makeContext<VoxelMeshesContext>
        }},
        {"uielements", {
            &makeContext<RenderDevContext>,
            &makeContext<UIElementsContext>
        }},
        {"collision", {
            &makeContext<CollisionContext>
        }}
    };
}
#endif

namespace game {
    class StateManagerImpl : public StateManager, public std::enable_shared_from_this<StateManagerImpl> {
    public:
        StateManagerImpl(
            const foundation::PlatformInterfacePtr &platform,
            const resource::ResourceProviderPtr &resourceProvider,
            const resource::FontAtlasProviderPtr &fontAtlasProvider,
            const core::SceneInterfacePtr &scene,
            const core::WorldInterfacePtr &world,
            const core::RaycastInterfacePtr &raycast,
            const core::SimulationInterfacePtr &simulation,
            const ui::StageInterfacePtr &ui
        )
        : _platform(platform)
        , _resourceProvider(resourceProvider)
        , _fontAtlasProvider(fontAtlasProvider)
        , _scene(scene)
        , _world(world)
        , _raycast(raycast)
        , _simulation(simulation)
        , _ui(ui)
        {
        }
        ~StateManagerImpl() override {}
        
        void switchToState(const char *name) override;
        void update(float dtSec) override;
        
    private:
        void _applyState(const std::string& name);

        const foundation::PlatformInterfacePtr _platform;
        const foundation::RenderingInterfacePtr _rendering;
        const resource::ResourceProviderPtr _resourceProvider;
        const resource::FontAtlasProviderPtr _fontAtlasProvider;
        const core::SceneInterfacePtr _scene;
        const core::WorldInterfacePtr _world;
        const core::RaycastInterfacePtr _raycast;
        const core::SimulationInterfacePtr _simulation;
        const ui::StageInterfacePtr _ui;
        
        std::string _nextStateName;
        std::unordered_map<MakeContextFunc, std::shared_ptr<Context>> _currentContextList;
    };

    void StateManagerImpl::switchToState(const char *name) {
        _nextStateName = name;
    }

    void StateManagerImpl::_applyState(const std::string &name) {
        auto state = std::find_if(std::begin(STATES), std::end(STATES), [&name](const State &s) { return s.name == name; });
        if (state == std::end(STATES)) {
            _platform->logError("[StateManager] '%s' state not found", name.c_str());
            return;
        }

        std::unordered_map<MakeContextFunc, std::shared_ptr<Context>> newContextList;
        std::vector<Interface *> interfaces;

        for (auto& makeContextFunction : state->makers) {
            std::shared_ptr<Context> ctx = nullptr;

            auto index = _currentContextList.find(makeContextFunction);
            if (index == _currentContextList.end()) {
                ctx = makeContextFunction(API { shared_from_this(), _platform, _resourceProvider, _fontAtlasProvider, _scene, _world, _raycast, _simulation, _ui }, interfaces.data(), interfaces.size());
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
                _platform->logError("[StateManager] Unexpected state while switching to '%s'", name.c_str());
            }
        }

        std::swap(_currentContextList, newContextList);
    }

    void StateManagerImpl::update(float dtSec) {
        if (_nextStateName.size()) {
            _applyState(std::exchange(_nextStateName, {}));
        }
        for (auto& contextEntry : _currentContextList) {
            contextEntry.second->update(dtSec);
        }
    }
}

std::shared_ptr<game::StateManager> game::StateManager::instance(
    const foundation::PlatformInterfacePtr &platform,
    const resource::ResourceProviderPtr &resourceProvider,
    const resource::FontAtlasProviderPtr &fontAtlasProvider,
    const core::SceneInterfacePtr &scene,
    const core::WorldInterfacePtr &world,
    const core::RaycastInterfacePtr &raycast,
    const core::SimulationInterfacePtr &simulation,
    const ui::StageInterfacePtr &ui
)
{
    return std::make_shared<game::StateManagerImpl>(platform, resourceProvider, fontAtlasProvider, scene, world, raycast, simulation, ui);
}
