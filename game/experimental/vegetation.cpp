
#include "vegetation.h"

namespace game {
    VegetationContext::VegetationContext(API &&api) : _api(std::move(api)) {
        _api.resources->getOrLoadGround("grounds/vegetation", [this](const foundation::RenderDataPtr& msh, const foundation::RenderTexturePtr& t, const resource::GroundMapDescription& desc) {
            if (msh) {
                _ground = _api.scene->addGroundMesh(msh, t);
                _ground->setPosition({ -64, 0, -64 });

                const math::vector2i msize = math::vector2i(t->getWidth() + 1, t->getHeight() + 1);
                for (const auto& vg : desc.vegetation) {
                    const math::vector3i margs = math::vector3i(msize.x, msize.y, vg.mapIndex);
                    if (vg.texture) {
                        _vg.emplace_back(_api.scene->addVegetation(desc.map, margs, vg.description, vg.texture));
                        _vg.back()->setPosition({ -64, 0, -64 });
                    }
                    else if (vg.voxmesh.size()) {
                        _vg.emplace_back(_api.scene->addVegetation(desc.map, margs, vg.description, vg.voxmesh));
                        _vg.back()->setPosition({ -64, 0, -64 });
                    }
                }
            }
        });
    }
}
