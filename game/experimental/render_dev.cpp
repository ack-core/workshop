
#include "render_dev.h"
#include "ui/extensions.h"
#include <list>

namespace {
    const char *g_meshShaderSrc = R"(
        const {
            modelTransform : matrix4
        }
        inout {
            uv : float2
            nrm : float3
        }
        vssrc {
            output_position = _transform(_transform(const_modelTransform, float4(vertex_position.xyz, 1.0)), frame_plmVPMatrix);
            output_uv = vertex_uv;
            output_nrm = vertex_normal.xyz;
        }
        fssrc {
            output_color[0] = float4(1.0, 0.0, 0.0, 1.0);
        }
    )";
}

namespace game {
    RenderDevContext::RenderDevContext(API &&api) : _api(std::move(api)) {
        _token = _api.platform->addPointerEventHandler(
            [this](const foundation::PlatformPointerEventArgs &args) -> bool {
                if (args.type == foundation::PlatformPointerEventArgs::EventType::START) {
                    _pointerId = args.pointerID;
                    _lockedCoordinates = { args.coordinateX, args.coordinateY };
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::MOVE) {
                    if (_pointerId != foundation::INVALID_POINTER_ID) {
                        float dx = args.coordinateX - _lockedCoordinates.x;
                        float dy = args.coordinateY - _lockedCoordinates.y;

                        _orbit.xz = _orbit.xz.rotated(dx / 200.0f);

                        math::vector3f right = math::vector3f(0, 1, 0).cross(_orbit).normalized();
                        math::vector3f rotatedOrbit = _orbit.rotated(right, dy / 200.0f);

                        if (fabs(math::vector3f(0, 1, 0).dot(rotatedOrbit.normalized())) < 0.96f) {
                            _orbit = rotatedOrbit;
                        }

                        _lockedCoordinates = { args.coordinateX, args.coordinateY };
                    }
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::FINISH) {
                    _pointerId = foundation::INVALID_POINTER_ID;
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::CANCEL) {
                    _pointerId = foundation::INVALID_POINTER_ID;
                }

                return true;
            }
        );

        _axis = _api.scene->addLineSet();
        _axis->setLine(0, {0, 0, 0}, {1000, 0, 0}, {1, 0, 0, 0.9});
        _axis->setLine(1, {0, 0, 0}, {0, 1000, 0}, {0, 1, 0, 0.9});
        _axis->setLine(2, {0, 0, 0}, {0, 0, 1000}, {0, 0, 1, 0.9});
        _axis->setLine(3, {0, 0, 0}, {-1000, 0, 0}, {0.5, 0.5, 0.5, 0.9});
        _axis->setLine(4, {0, 0, 0}, {0, 0, -1000}, {0.5, 0.5, 0.5, 0.9});

//        _mesh0 = _api.scene->addCustomMesh("mesh", g_meshShaderSrc, layouts::VTXNRMUV);
//        _api.resources->getOrLoadTexture("textures/ui/joystick_bg_00", [this](const foundation::RenderTexturePtr &t) {
//            _mesh0->setTextures({
//                {t, foundation::SamplerType::LINEAR}
//            });
//            struct Vertex {
//                float x, y, z;
//                float nx, ny, nz;
//                float u, v;
//            };
//            Vertex va[] = {
//                {0, 0, 0, 0, 1, 0, 0.5, 0.5},
//                {10, 0, 0, 0, 1, 0, 0.5, 0.5},
//                {0, 0, 10, 0, 1, 0, 0.5, 0.5},
//            };
//
//            _mesh0->updateMeshData(va, 3);
//            const math::transform3f trfm = math::transform3f::identity();
//            _mesh0->updateShaderConstants(&trfm);
//        });

//        _joystick = _api.ui->addExtensionElement(std::nullopt, nullptr, ui::extensions::JoystickParams {
//            .anchorH = ui::HorizontalAnchor::RIGHT,
//            .anchorV = ui::VerticalAnchor::BOTTOM,
//            .anchorOffset = math::vector2f(50.0f, 50.0f),
//            .textureBackground = "textures/ui/joystick_bg_00",
//            .textureThumb = "textures/ui/joystick_thumb",
//            .maxThumbOffset = 100.0f,
//            .onChange = [this](const math::vector2f &direction) {
//                printf("%f %f\n", direction.x, direction.y);
//            }
//        });
        _api.resources->getOrLoadVoxelMesh("meshes/a-knight-blue", [this](const std::vector<foundation::RenderDataPtr> &frames, const util::Description &desc) {
            _knight = _api.scene->addVoxelMesh(frames, desc);
            _knight->setPosition({55, 0, 48});
        });
        
        _api.resources->getOrLoadGround("grounds/test", [this](const foundation::RenderDataPtr &mesh, const foundation::RenderTexturePtr &texture, const resource::GroundMapDescription &desc) {
            _ground = _api.scene->addGroundMesh(mesh, texture);
            const math::vector2i msize = math::vector2i(texture->getWidth() + 1, texture->getHeight() + 1);
            for (std::size_t i = 0; i < desc.vegetation.size(); i++) {
                const auto &vg = desc.vegetation[i];
                if (vg.texture) {
                    _api.scene->addVegetation(desc.map, math::vector3i(msize.x, msize.y, 1 << i), vg.description, vg.texture);
                }
                else if (vg.voxmesh.size()) {
                    _api.scene->addVegetation(desc.map, math::vector3i(msize.x, msize.y, 1 << i), vg.description, vg.voxmesh);
                }
            }
//            if (const util::Description *vg = desc->cfg.getDescription("vegetation")) {
//                const std::vector<const util::Description *> vgTypes = vg->getDescriptions("type");
//                const math::vector2i msize = math::vector2i(texture->getWidth() + 1, texture->getHeight() + 1);
//
//                //for (std::uint8_t i = 0; i < std::uint8_t(vgTypes.size()); i++) {
//                {
//                    const util::Description *type = vgTypes[0];
//                    if (const std::string *texture = type->getString("source")) {
//                        _api.resources->getOrLoadTexture(texture->c_str(), [this, desc, type, msize](const foundation::RenderTexturePtr &tx) {
//                            _grass = _api.scene->addVegetation(tx, desc->map, msize, 0, *type);
//                        });
//                    }
//                }
//                {
//                    const util::Description *type = vgTypes[1];
//                    if (const std::string *texture = type->getString("source")) {
//                        _api.resources->getOrLoadTexture(texture->c_str(), [this, desc, type, msize](const foundation::RenderTexturePtr &tx) {
//                            _api.scene->addVegetation(tx, desc->map, msize, 1, *type);
//                        });
//                    }
//                }
//            }
        });
        
        
    }
    
    RenderDevContext::~RenderDevContext() {
        _api.platform->removeEventHandler(_token);
    }
    
    void RenderDevContext::update(float dtSec) {
        _api.scene->setCameraLookAt(math::vector3f{64, 0, 64} + _orbit, math::vector3f{64, 0, 64});
    }
}
