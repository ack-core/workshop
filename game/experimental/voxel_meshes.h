#pragma once
#include <game/context.h>

namespace game {
    class VoxelMeshesContext : public Context {
    public:
        VoxelMeshesContext(API &&api);
        ~VoxelMeshesContext() override {}

        void init() override {}
        void update(float dtSec) override {}

    private:
        const API _api;

        core::SceneInterface::VoxelMeshPtr _plane;
        core::SceneInterface::VoxelMeshPtr _ruins;
        core::SceneInterface::VoxelMeshPtr _debris;
        core::SceneInterface::VoxelMeshPtr _knight;
    };
}

