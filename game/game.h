
#pragma once
#include <initializer_list>

// Here are all context includes

#include "contexts/debug_context.h"
#include "contexts/editor/editor_main_context.h"
#include "contexts/editor/editor_camera_context.h"
#include "contexts/editor/editor_voxel_mesh_context.h"
#include "contexts/editor/editor_particles_context.h"

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
    states[] = {
        {"default", {
            &makeContext<EditorCameraContext>,
            &makeContext<EditorMainContext, CameraAccessInterface>,
            &makeContext<EditorVoxelMeshContext, NodeAccessInterface>,
            &makeContext<EditorParticlesContext, NodeAccessInterface, CameraAccessInterface>
        }}
//        {"default", {
//            &makeContext<DebugContext>
//        }}
    };
};

