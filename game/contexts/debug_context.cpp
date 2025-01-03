
#include "debug_context.h"

namespace game {
//    template <> std::unique_ptr<Context> makeContext<DebugContext>(API &&api) {
//        return std::make_unique<DebugContext>(std::move(api));
//    }
    
    DebugContext::DebugContext(API &&api) : _api(std::move(api)) {
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
        _axis->setLine(0, {0, 0, 0}, {1000, 0, 0}, {1, 0, 0, 0.5});
        _axis->setLine(1, {0, 0, 0}, {0, 1000, 0}, {0, 1, 0, 0.5});
        _axis->setLine(2, {0, 0, 0}, {0, 0, 1000}, {0, 0, 1, 0.5});
        
        _api.resources->getOrLoadVoxelObject("objects/stool", [this](const std::vector<foundation::RenderDataPtr> &frames) {
            if (frames.size()) {
                _actor = _api.scene->addDynamicMesh(frames);
                _actor->setTransform(math::transform3f({0, 1, 0}, M_PI / 4).translated({20, 0, 40}));
            }
        });
        _api.resources->getOrLoadVoxelStatic("statics/ruins", [this](const foundation::RenderDataPtr &mesh) {
            if (mesh) {
                _thing = _api.scene->addStaticMesh(mesh);
            }
        });
        _api.resources->getOrLoadGround("grounds/white", [this](const foundation::RenderDataPtr &data, const foundation::RenderTexturePtr &texture) {
            if (data && texture) {
                _ground = _api.scene->addTexturedMesh(data, texture);
            }
        });
        _api.resources->getOrLoadTexture("textures/particles/test", [this](const foundation::RenderTexturePtr &texture){
            if (texture) {
                std::uint32_t ptcparamssrc[] = {
                    0x00000000,
                    0x00000000,
                    0x000000ff,
                    0x00000000,
                };
                foundation::RenderTexturePtr ptcparams = _api.rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, 1, 4, { ptcparamssrc });
                _ptc = _api.scene->addParticles(texture, ptcparams, voxel::ParticlesParams {
                    .additiveBlend = false,
                    .orientation = voxel::ParticlesOrientation::WORLD,
                    .particleCount = 2,
                    .minXYZ = {-5.0, 0, 0},
                    .maxXYZ = {5.0, 0, 0},
                    .minMaxWidth = {10.0f, 10.0f},
                    .minMaxHeight = {10.0f, 10.0f},
                });
                _ptc->setTransform(math::transform3f::identity().translated({32, 5, 32}));
            }
        });

        math::bound3f bb1 = {-0.5f, -0.5f, -0.5f, 63.0f + 0.5f, 19.0f + 0.5f, 63.0f + 0.5f};
        _bbox = _api.scene->addBoundingBox(bb1);
        _bbox->setColor({0.5f, 0.5f, 0.5f, 0.5f});
        
        
    }
    
    DebugContext::~DebugContext() {
        _api.platform->removeEventHandler(_token);
    }
    
    void DebugContext::update(float dtSec) {
        _api.scene->setCameraLookAt(_orbit + math::vector3f{32, 0, 32}, {32, 0, 32});
    }
}
