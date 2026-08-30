
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

        _mesh0 = _api.scene->addCustomMesh("mesh", g_meshShaderSrc, layouts::VTXNRMUV);
        _api.resources->getOrLoadTexture("textures/ui/joystick_bg_00", [this](const foundation::RenderTexturePtr &t) {
            _mesh0->setTextures({
                {t, foundation::SamplerType::LINEAR}
            });
            struct Vertex {
                float x, y, z;
                float nx, ny, nz;
                float u, v;
            };
            Vertex va[] = {
                {0, 0, 0, 0, 1, 0, 0.5, 0.5},
                {10, 0, 0, 0, 1, 0, 0.5, 0.5},
                {0, 0, 10, 0, 1, 0, 0.5, 0.5},
            };
            
            _mesh0->updateMeshData(va, 3);
            const math::transform3f trfm = math::transform3f::identity();
            _mesh0->updateShaderConstants(&trfm);
        });

        _joystick = _api.ui->addExtensionElement(std::nullopt, nullptr, ui::extensions::JoystickParams {
            .anchorH = ui::HorizontalAnchor::RIGHT,
            .anchorV = ui::VerticalAnchor::BOTTOM,
            .anchorOffset = math::vector2f(50.0f, 50.0f),
            .textureBackground = "textures/ui/joystick_bg_00",
            .textureThumb = "textures/ui/joystick_thumb",
            .maxThumbOffset = 100.0f,
            .onChange = [this](const math::vector2f &direction) {
                printf("%f %f\n", direction.x, direction.y);
            }
        });
        
        _img0 = _api.ui->addImage(std::nullopt, nullptr, ui::StageInterface::ImageParams {
            .anchorH = ui::HorizontalAnchor::LEFT,
            .anchorV = ui::VerticalAnchor::TOP,
            .anchorOffset = math::vector2f(50.0f, 50.0f),
            .texture = "textures/ui/joystick_thumb",
        });
        _img1 = _api.ui->addImg9Slice(std::nullopt, nullptr, ui::StageInterface::Img9SliceParams {
            .anchorH = ui::HorizontalAnchor::CENTER,
            .anchorV = ui::VerticalAnchor::TOP,
            .anchorOffset = math::vector2f(0.0f, 250.0f),
            .size = math::vector2f(21.0f, 31.0f),
            .texture = "textures/ui/img9slice",
            .sliceArgs = math::vector3f(122.0f, 122.0f, 3.0f)
        });
        _img1->setActionHandler([this](ui::Action, float x, float y) {
            //printf("%f %f\n", x, y);
            _img0->setTexture(_txt0->getTexture());
        });
        _txt0 = _api.ui->addTextLine(std::nullopt, nullptr, ui::StageInterface::TextLineParams {
            .anchorH = ui::HorizontalAnchor::RIGHT,
            .anchorV = ui::VerticalAnchor::TOP,
            .anchorOffset = math::vector2f(0.0f, 0.0f),
            .fontSize = 20,
            .fontColor = math::color(1.0f, 0.5f, 0.0f, 1.0f),
            .shadowColor = math::color(0.0f, 0.0f, 0.0f, 1.0f),
            .shadowOffset = math::vector2f(1.0f, 1.0f),
            .shadowBlur = 1
        });
        _txt1 = _api.ui->addTextLine(std::nullopt, nullptr, ui::StageInterface::TextLineParams {
            .anchorH = ui::HorizontalAnchor::RIGHT,
            .anchorV = ui::VerticalAnchor::TOP,
            .anchorOffset = math::vector2f(0.0f, 50.0f),
            .fontSize = 20,
            .fontColor = math::color(1.0f, 0.5f, 0.0f, 1.0f),
            .shadowColor = math::color(0.0f, 0.0f, 0.0f, 1.0f),
            .shadowOffset = math::vector2f(1.0f, 1.0f),
            .shadowBlur = 1
        });

        _tb0 = _api.ui->addTextBlock(std::nullopt, nullptr, ui::StageInterface::TextBlockParams {
            .anchorH = ui::HorizontalAnchor::CENTER,
            .anchorV = ui::VerticalAnchor::TOP,
            .anchorOffset = math::vector2f(0.0f, 0.0f),
            .size = math::vector2f(601.0f, 101.0f),
            .textAlign = ui::TextAlign::CENTER,
            .fontSize = 20,
            .fontColor = math::color(1.0f, 0.5f, 0.0f, 1.0f),
            .shadowColor = math::color(0.0f, 0.0f, 0.0f, 1.0f),
            .shadowOffset = math::vector2f(1.0f, 1.0f),
            .shadowBlur = 1
        });
        
        _txt0->setText("This is going to be\nawesome!");
        _txt1->setText("Que que que");
        

        _tb0->setText("The glyphs in Block Elements\neach share the same character width in most supported fonts, allowing them to be used graphically in row and column arrangements. However, the block does not contain a space character of its own and ASCII space may or may not render at the same width as Block Elements glyphs, as those characters are intended to be used exclusively for monospaced fonts.");
        
        _btn0 = _api.ui->addExtensionElement(std::nullopt, nullptr, ui::extensions::Button9SliceParams {
            .anchorH = ui::HorizontalAnchor::CENTER,
            .anchorV = ui::VerticalAnchor::TOP,
            .anchorOffset = math::vector2f(0.0f, 350.0f),
            .size = math::vector2f(101.0f, 51.0f),
            .textureBase = "textures/ui/btn_slice_base",
            .textureOver = "textures/ui/btn_slice_press",
            .texturePress = "textures/ui/btn_slice_press",
            .sliceArgs = math::vector3f(24.0f, 24.0f, 4.0f),
            .onPress = []() {
                printf("pressed!");
            }
        });
        _txt2 = _api.ui->addTextLine(std::nullopt, _btn0, ui::StageInterface::TextLineParams {
            .anchorH = ui::HorizontalAnchor::CENTER,
            .anchorV = ui::VerticalAnchor::MIDDLE,
            .anchorOffset = math::vector2f(0.0f, -4.0f),
            .fontSize = 20,
            .fontColor = math::color(1.0f, 0.5f, 0.0f, 1.0f),
            .shadowColor = math::color(0.0f, 0.0f, 0.0f, 1.0f),
            .shadowOffset = math::vector2f(1.0f, 1.0f),
            .shadowBlur = 1
        });
        _txt2->setText("Battle!");
    }
    
    RenderDevContext::~RenderDevContext() {
        _api.platform->removeEventHandler(_token);
    }
    
    void RenderDevContext::update(float dtSec) {
        _api.scene->setCameraLookAt(_orbit, {0, 0, 0});
    }
}
