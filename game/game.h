
#pragma once
#include <initializer_list>

// Here are all context includes

#include "editor/editor_main_context.h"
#include "editor/editor_camera_context.h"
#include "editor/editor_prefab_context.h"
#include "editor/editor_voxel_mesh_context.h"
#include "editor/editor_ground_context.h"
#include "editor/editor_particles_context.h"
#include "editor/editor_raycast_shape_context.h"
#include "editor/editor_collision_shape_context.h"
#include "experimental/debug_context.h"
#include "experimental/render_dev.h"

#include "contexts/game_data_interface.h"
#include "contexts/castle_interface.h"
#include "contexts/battle_interface.h"

#include "contexts/loading_context.h"
#include "contexts/menu_ui_context.h"
#include "contexts/common_ui_context.h"
#include "contexts/construction_ui_context.h"
#include "contexts/castle_context.h"
#include "contexts/battle_ui_context.h"
#include "contexts/battle_context.h"
#include "contexts/game_data_context.h"

// Rule of states:
// Context is created if the next state contains it and current state does not
// Context is deleted if the next state does not contain it

namespace game {
    static const char *datahub = R"(
        options {
            drawBoundBoxes : bool = true
            drawSimulation : bool = true
        }
    )";
    static const struct {
        const char *name;
        const std::initializer_list<MakeContextFunc> makers;
    }
    STATES[] = {
#ifdef IS_EDITOR
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
#endif
#ifdef IS_GAME
//        {"default", {
//            &makeContext<DebugContext>
//        }}
//        {"default", {
//            &makeContext<RenderDevContext>
//        }}
        
        {"default", {
            &makeContext<GameDataContext>,
            &makeContext<LoadingContext, GameDataInterface>
        }},
        {"menu", {
            &makeContext<GameDataContext>,
            &makeContext<CastleContext, GameDataInterface>,
            &makeContext<MenuUIContext, CastleInterface>,
        }},
        {"castle", {
            &makeContext<GameDataContext>,
            &makeContext<CommonUIContext>,
            &makeContext<CastleContext, GameDataInterface>,
            &makeContext<ConstructionUIContext, CastleInterface>,
        }},
        {"battle", {
            &makeContext<GameDataContext>,
            &makeContext<CommonUIContext>,
            &makeContext<CastleContext, GameDataInterface>,
            &makeContext<BattleContext, GameDataInterface>,
            &makeContext<BattleUIContext, GameDataInterface, BattleInterface>,
        }}

#endif
    };
};

