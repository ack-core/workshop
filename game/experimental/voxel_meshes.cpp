
#include "voxel_meshes.h"

namespace game {
    VoxelMeshesContext::VoxelMeshesContext(API &&api) : _api(std::move(api)) {
        _api.resources->getOrLoadVoxelMesh("meshes/voxplane", [this](const std::vector<foundation::RenderDataPtr> &data, const util::Description &desc) {
            _plane = _api.scene->addVoxelMesh(data, desc);
            _plane->setPosition({-32, 0, -32});
        });
        _api.resources->getOrLoadVoxelMesh("meshes/ruins", [this](const std::vector<foundation::RenderDataPtr> &data, const util::Description &desc) {
            _ruins = _api.scene->addVoxelMesh(data, desc);
            _ruins->setPosition({-20, 0, -20});
        });
        _api.resources->getOrLoadVoxelMesh("meshes/debris", [this](const std::vector<foundation::RenderDataPtr> &data, const util::Description &desc) {
            _debris = _api.scene->addVoxelMesh(data, desc);
            _debris->setPosition({5, 0, 5});
        });
        _api.resources->getOrLoadVoxelMesh("meshes/knight", [this](const std::vector<foundation::RenderDataPtr>& data, const util::Description& desc) {
            _knight = _api.scene->addVoxelMesh(data, desc);
            _knight->setPosition({ 0, 1, 0 });
        });
        _api.scene->setSun({ 1, 1, 0 }, {});
    }
}

