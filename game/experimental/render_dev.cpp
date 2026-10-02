
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

        _joystick = _api.ui->addExtensionElement(std::nullopt, nullptr, ui::extensions::JoystickParams {
            .anchorH = ui::HorizontalAnchor::RIGHT,
            .anchorV = ui::VerticalAnchor::BOTTOM,
            .anchorOffset = math::vector2f(50.0f, 50.0f),
            .textureBackground = "textures/ui/joystick_bg_00",
            .textureThumb = "textures/ui/joystick_thumb",
            .maxThumbOffset = 100.0f,
            .onChange = [this](const math::vector2f &direction) {
                //printf("%f %f\n", direction.x, direction.y);
            }
        });
        _api.resources->getOrLoadVoxelMesh("meshes/a-knight-blue", [this](const std::vector<foundation::RenderDataPtr> &frames, const util::Description &desc) {
            _knight = _api.scene->addVoxelMesh(frames, desc);
            _knight->setPosition({10, 0, 5});
        });
        
//        _img0 = _api.ui->addImage(std::nullopt, nullptr, ui::StageInterface::ImageParams {
//            .anchorH = ui::HorizontalAnchor::LEFT,
//            .anchorV = ui::VerticalAnchor::TOP,
//            .anchorOffset = math::vector2f(0.0f, 0.0f),
//            .texture = "textures/ui/castle-hp-bar",
//        });

        
//        _api.resources->getOrLoadGround("grounds/test", [this](const foundation::RenderDataPtr &mesh, const foundation::RenderTexturePtr &texture, const resource::GroundMapDescription &desc) {
//            _ground = _api.scene->addGroundMesh(mesh, texture);
//            const math::vector2i msize = math::vector2i(texture->getWidth() + 1, texture->getHeight() + 1);
//            for (std::size_t i = 0; i < desc.vegetation.size(); i++) {
//                const auto &vg = desc.vegetation[i];
//                if (vg.texture) {
//                    _api.scene->addVegetation(desc.map, math::vector3i(msize.x, msize.y, 1 << i), vg.description, vg.texture);
//                }
//                else if (vg.voxmesh.size()) {
//                    _api.scene->addVegetation(desc.map, math::vector3i(msize.x, msize.y, 1 << i), vg.description, vg.voxmesh);
//                }
//            }
////            if (const util::Description *vg = desc->cfg.getDescription("vegetation")) {
////                const std::vector<const util::Description *> vgTypes = vg->getDescriptions("type");
////                const math::vector2i msize = math::vector2i(texture->getWidth() + 1, texture->getHeight() + 1);
////
////                //for (std::uint8_t i = 0; i < std::uint8_t(vgTypes.size()); i++) {
////                {
////                    const util::Description *type = vgTypes[0];
////                    if (const std::string *texture = type->getString("source")) {
////                        _api.resources->getOrLoadTexture(texture->c_str(), [this, desc, type, msize](const foundation::RenderTexturePtr &tx) {
////                            _grass = _api.scene->addVegetation(tx, desc->map, msize, 0, *type);
////                        });
////                    }
////                }
////                {
////                    const util::Description *type = vgTypes[1];
////                    if (const std::string *texture = type->getString("source")) {
////                        _api.resources->getOrLoadTexture(texture->c_str(), [this, desc, type, msize](const foundation::RenderTexturePtr &tx) {
////                            _api.scene->addVegetation(tx, desc->map, msize, 1, *type);
////                        });
////                    }
////                }
////            }
//        });
        
        const char *testShaderSrc = R"(
            inout {
                texcoord : float2
            }
            vssrc {
                float2 screenTransform = float2(2.0, -2.0) / frame_rtBounds.xy;
                output_texcoord = vertex_position_tx.zw;
                output_position = float4(vertex_position_tx.xy * screenTransform + float2(-1.0, 1.0), 0.1, 1); //
            }
            fssrc {
                float4 texcolor = _tex2d(0, input_texcoord);
                output_color[0] = texcolor; //input_color * _lerp(texcolor, float4(1.0, 1.0, 1.0, texcolor.r), input_args.x);
            }
        )";
        const foundation::InputLayout testLayout = foundation::InputLayout {
            .attributes = {
                {"position_tx", foundation::InputAttributeFormat::FLOAT4},
            }
        };
        
        const resource::TextureInfo *tinfo = _api.resources->getTextureInfo("textures/ui/button_down");
//        struct Vtx {
//            math::vector4f postex;
//        }
//        testVData[12] = {
//            { math::vector4f(tinfo->width / 2, tinfo->height / 2, 0.5f, 0.5f) },
//            { math::vector4f(tinfo->width / 2, tinfo->height / 2, 0.5f, 0.5f) },
//            { math::vector4f(tinfo->width / 2, tinfo->height / 2, 0.5f, 0.5f) },
//            { math::vector4f(tinfo->width / 2, tinfo->height / 2, 0.5f, 0.5f) },
//            { math::vector4f(tinfo->width / 2, tinfo->height / 2, 0.5f, 0.5f) },
//            { math::vector4f(tinfo->width / 2, tinfo->height / 2, 0.5f, 0.5f) },
//            { math::vector4f(tinfo->width / 2, tinfo->height / 2, 0.5f, 0.5f) },
//            { math::vector4f(tinfo->width / 2, tinfo->height / 2, 0.5f, 0.5f) },
//            { math::vector4f(tinfo->width / 2, tinfo->height / 2, 0.5f, 0.5f) },
//            { math::vector4f(tinfo->width / 2, tinfo->height / 2, 0.5f, 0.5f) },
//            { math::vector4f(tinfo->width / 2, tinfo->height / 2, 0.5f, 0.5f) },
//            { math::vector4f(tinfo->width / 2, tinfo->height / 2, 0.5f, 0.5f) },
////            { math::vector4f(0, 0, 0, 0) },
////            { math::vector4f(tinfo->width, 0, 1.0f, 0) },
////            { math::vector4f(tinfo->width, tinfo->height, 1.0f, 1.0f) },
////            { math::vector4f(0, tinfo->height, 0, 1.0f) }
//        };
//        const std::uint32_t testIData[12] = {
//            0, 1, 2,
//            0, 2, 3,
//            0, 3, 4,
//            0, 4, 1
//        };
        
//        auto rayKoeff = [&](int index, float deg, float& koeff) -> bool {
//            float d = deg - index * 90.0f;
//            d = std::fmod(d + 180.0f, 360.0f);
//            if (d < 0.0f) d += 360.0f;
//            d -= 180.0f;
//
//            if (d < -45.0f || d >= 45.0f)
//                return false;
//
//            const float t = std::tan(d * 3.14159265358979323846f / 180.0f);
//
//            koeff = (t + 1.0f) * 0.5f;
//            return true;
//        };
//
//        float sectorStart = -45.0f, sectorEnd = 314.0f - std::numeric_limits<float>::epsilon();
//
//
//        auto lerp = [](const math::vector4f &a, const math::vector4f &b, float koeff) {
//            return a + (b - a) * koeff;
//        };
//        auto edgeToSector = [&](int triangleIndex, float midA, const math::vector4f &v0, const math::vector4f &v1, math::vector4f &o0, math::vector4f &o1) {
//            float koeff = 0;
//            if (rayKoeff(triangleIndex, sectorStart, koeff)) {
//                o0 = lerp(v0, v1, koeff);
//                o1 = rayKoeff(triangleIndex, sectorEnd, koeff) ? lerp(v0, v1, koeff) : v1;
//            }
//            else if (rayKoeff(triangleIndex, sectorEnd, koeff)) {
//                o0 = v0;
//                o1 = lerp(v0, v1, koeff);
//            }
//            else {
//                float angle = midA;
//                while (angle < sectorStart) angle += 360.0f;
//                if (angle <= sectorEnd) {
//                    o0 = v0;
//                    o1 = v1;
//                }
//            }
//        };
//
//        const float midAngles[] = {0.0f, 90.0f, 180.0f, 270.0f};
//        const math::vector4f corners[] = {
//            math::vector4f(0, 0, 0, 0),
//            math::vector4f(tinfo->width, 0, 1.0f, 0),
//            math::vector4f(tinfo->width, tinfo->height, 1.0f, 1.0f),
//            math::vector4f(0, tinfo->height, 0, 1.0f)
//        };
//
//        for (int i = 0; i < 4; i++) {
//            edgeToSector(i, midAngles[i], corners[i], corners[(i + 1) % 4], testVData[i * 3 + 1].postex, testVData[i * 3 + 2].postex);
//        }
        
        
//        edgeToSector(0, -45.0f, 45.0f, math::vector4f(0, 0, 0, 0), math::vector4f(tinfo->width, 0, 1.0f, 0), testVData[1].postex, testVData[2].postex);
//        edgeToSector(1, 45.0f, 135.0f, math::vector4f(tinfo->width, 0, 1.0f, 0), math::vector4f(tinfo->width, tinfo->height, 1.0f, 1.0f), testVData[2].postex, testVData[3].postex);
//        edgeToSector(2, 135.0f, 225.0f, math::vector4f(tinfo->width, tinfo->height, 1.0f, 1.0f), math::vector4f(0, tinfo->height, 0, 1.0f), testVData[3].postex, testVData[4].postex);
//        edgeToSector(3, 225.0f, 315.0f, math::vector4f(0, tinfo->height, 0, 1.0f), math::vector4f(0, 0, 0, 0), testVData[4].postex, testVData[1].postex);

//        if (rayKoeff(1, sectorStart, koeff)) {
//            testVData[2].postex.yw = math::vector2f(koeff * tinfo->height, koeff);
//            if (rayKoeff(1, sectorEnd, koeff)) {
//                testVData[3].postex.yw = math::vector2f(koeff * tinfo->height, koeff);
//            }
//        }
//        else if (rayKoeff(1, sectorEnd, koeff)) {
//            testVData[3].postex.yw = math::vector2f(koeff * tinfo->height, koeff);
//        }
//        else {
//            testVData[1].postex.yw = testVData[2].postex.yw = testVData[0].postex.yw;
//        }
        

        
//        auto edgeToSector = [&](int triangleIndex, int v0, int v1, float length, float sectorStart, float sectorEnd) {
//            float koeff = 0;
//            if (rayKoeff(triangleIndex, sectorStart, koeff)) {
//                testVData[v0].postex.x = koeff * length;
//                testVData[v0].postex.z = koeff;
//                if (rayKoeff(triangleIndex, sectorEnd, koeff)) {
//                    testVData[v1].postex.x = koeff * length;
//                    testVData[v1].postex.z = koeff;
//                }
//            }
//            else if (rayKoeff(triangleIndex, sectorEnd, koeff)) {
//                testVData[v1].postex.x = koeff * length;
//                testVData[v1].postex.z = koeff;
//            }
//            else {
//                testVData[v0].postex = testVData[0].postex;
//                testVData[v1].postex = testVData[0].postex;
//            }
//        };
        
        //edgeToSector(0, 1, 2, tinfo->width, sectorStart, sectorEnd);
        //edgeToSector(1, 2, 4, tinfo->height, sectorStart, sectorEnd);
        //edgeToSector(2, 4, 3, tinfo->height, sectorStart, sectorEnd);
        //edgeToSector(3, 3, 4, tinfo->height, sectorStart, sectorEnd);
        
        struct Vtx {
            math::vector2f pos;
            math::vector2f tx;
        };
        auto makeVertex = [](float x, float y, float w, float h) {
            return Vtx { math::vector2f(x, y), math::vector2f(x / w, y / h) };
        };
        auto boundaryVertex = [&](float width, float height, float angleDeg) {
            const float rad = angleDeg * (3.14159265358979323846f / 180.0f);
            const float dx = std::sin(rad);
            const float dy = -std::cos(rad);
            const float scale = 1.0f / std::max(std::abs(dx), std::abs(dy));
            const float halfW = width * 0.5f;
            const float halfH = height * 0.5f;
            return makeVertex(halfW + dx * scale * halfW, halfH + dy * scale * halfH, width, height);
        };
        std::vector<Vtx> vertices;
        std::vector<std::uint32_t> indices;
        float startDeg = 0, endDeg = 360;
        float width = tinfo->width;
        float height = tinfo->height;
        
        const float sweep = std::min(endDeg - startDeg, 360.0f);
        if (!(sweep > 0.0f)) {
            return;
        }
        endDeg = startDeg + sweep;

        // Углы прямоугольника в порядке обхода, k-й угол лежит на 45 + 90 * k градусах
        const Vtx corners[] = {
            makeVertex(width, 0.0f, width, height),
            makeVertex(width, height, width, height),
            makeVertex(0.0f, height, width, height),
            makeVertex(0.0f, 0.0f, width, height),
        };

        const auto base = static_cast<uint32_t>(vertices.size());
        vertices.push_back(makeVertex(width * 0.5f, height * 0.5f, width, height));
        vertices.push_back(boundaryVertex(width, height, startDeg));

        // Углы прямоугольника, попавшие строго внутрь сектора, становятся вершинами веера
        for (int k = static_cast<int>(std::floor((startDeg - 45.0f) / 90.0f)) + 1; 45.0f + 90.0f * static_cast<float>(k) < endDeg; ++k) {
            vertices.push_back(corners[((k % 4) + 4) % 4]);
        }

        vertices.push_back(boundaryVertex(width, height, endDeg));

        const auto last = static_cast<uint32_t>(vertices.size()) - 1;
        for (uint32_t i = base + 1; i < last; ++i) {
            indices.push_back(base);
            indices.push_back(i);
            indices.push_back(i + 1);
        }

        
//        _img2 = _api.ui->addImgCustom(std::nullopt, nullptr, ui::StageInterface::ImgCustomParams {
//            .anchorH = ui::HorizontalAnchor::CENTER,
//            .anchorV = ui::VerticalAnchor::TOP,
//            .anchorOffset = math::vector2f(0.0f, 5.0f),
//            .layout = testLayout,
//            .shaderSource = testShaderSrc,
//            .texture = "textures/ui/button_down"
//        });
//        _img2->setGeometry(foundation::RenderTopology::TRIANGLES, vertices.data(), std::uint32_t(vertices.size()), indices.data(), std::uint32_t(indices.size()));
        
        _ims = _api.ui->addExtensionElement(std::nullopt, nullptr, ui::extensions::ImgSectorParams {
            .anchorH = ui::HorizontalAnchor::CENTER,
            .anchorV = ui::VerticalAnchor::TOP,
            .anchorOffset = math::vector2f(0.0f, 5.0f),
            .texture = "textures/ui/button_down"
        });
        
        
    }
    
    RenderDevContext::~RenderDevContext() {
        _api.platform->removeEventHandler(_token);
    }
    
    void RenderDevContext::update(float dtSec) {
        _api.scene->setCameraLookAt(_orbit, math::vector3f{0, 0, 0});
        _ims->setSector(90, 360);
    }
}
