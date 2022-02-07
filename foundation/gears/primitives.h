
#pragma once

#include "math.h"
#include "foundation/rendering.h"

namespace gears {
    class Primitives {
    public:
        Primitives(const std::shared_ptr<foundation::RenderingInterface> &rendering) : _rendering(rendering) {}

        void drawAxis(const foundation::RenderPassConfig &cfg) {
            static const char *axisShaderSrc = R"(
                fixed {
                    coords[10] : float4 =
                        [0.0, 0.0, 0.0, 1.0][1000.0, 0.0, 0.0, 1.0]
                        [0.0, 0.0, 0.0, 1.0][0.0, 1000.0, 0.0, 1.0]
                        [0.0, 0.0, 0.0, 1.0][0.0, 0.0, 1000.0, 1.0]
                        [1.0, 0.0, 0.0, 1.0][1.0, 0.0, 1.0, 1.0]
                        [0.0, 0.0, 1.0, 1.0][1.0, 0.0, 1.0, 1.0]
                    colors[5] : float4 =
                        [1.0, 0.0, 0.0, 1.0]
                        [0.0, 1.0, 0.0, 1.0]
                        [0.0, 0.0, 1.0, 1.0]
                        [0.5, 0.5, 0.5, 1.0]
                        [0.5, 0.5, 0.5, 1.0]
                }
                inout {
                    color : float4
                }
                vssrc {
                    output_position = _transform(fixed_coords[vertex_ID], frame_viewProjMatrix);
                    output_color = fixed_colors[vertex_ID >> 1];
                }
                fssrc {
                    output_color = input_color;
                }
            )";

            if (_axisShader == nullptr) {
                _axisShader = _rendering->createShader("primitives_axis", axisShaderSrc, {
                    {"ID", foundation::RenderShaderInputFormat::ID}
                });
            }
            
            _rendering->beginPass("primitives_axis", _axisShader, cfg);
            _rendering->drawGeometry(nullptr, 10, foundation::RenderTopology::LINES);
            _rendering->endPass();
        }
        
        void drawSphere(const math::vector3f &position, float radius) {
            static const char *sphereShaderSrc = R"(
                const {
                    position_radius : float4
                }
                inout {
                    color : float4
                }
                vssrc {
                    float f = vertex_ID;
                    float v = f - 6.0 * floor(f / 6.0);
                    f = (f - v) / 6.0;
                    float a = f - 16.0 * floor(f / 16.0);
                    f = (f - a) / 16.0;
                    float b = f - 8.0;
                    a = a + (v - 2.0 * floor(v / 2.0));
                    b = b + (v == 2.0 || v >= 4.0 ? 1.0 : 0.0);
                    a = a / 16.0 * 6.28318;
                    b = b / 16.0 * 6.28318;
                    float3 position = 1.0f * float3(cos(a) * cos(b), sin(b), sin(a) * cos(b));
                    output_color = float4(normalize(position), 1.0);
                    output_position = _transform(float4(position, 1.0), frame_viewProjMatrix);
                }
                fssrc {
                    output_color = input_color;
                }
            )";
            
            if (_sphereShader == nullptr) {
                _sphereShader = _rendering->createShader("primitives_sphere", sphereShaderSrc, {
                    {"ID", foundation::RenderShaderInputFormat::ID}
                });
            }
            
            math::vector4f position_radius (position, radius);

            _rendering->beginPass("primitives_sphere", _sphereShader);
            _rendering->applyShaderConstants(&position_radius);
            _rendering->drawGeometry(nullptr, 1149, foundation::RenderTopology::TRIANGLES);
            _rendering->endPass();
        
        }
        
        void drawTexturedRectangle(float x, float y, float w, float h, const foundation::RenderTexturePtr &texture) {
            static const char *texturedRectShaderSrc = R"(
                const {
                    position_size : float4
                    color : float4
                }
                inout {
                    texcoord : float2
                }
                vssrc {
                    float2 screenTransform = float2(2.0, -2.0) / frame_rtBounds.xy;
                    float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
                    output_texcoord = vertexCoord; //mix(const_texcoord.xy, const_texcoord.zw, vertexCoord);
                    vertexCoord = vertexCoord * const_position_size.zw + const_position_size.xy; //
                    output_position = float4(vertexCoord.xy * screenTransform + float2(-1.0, 1.0), 0.5, 1);
                }
                fssrc {
                    output_color = _tex2d(0, input_texcoord);// * const_color;
                }
            )";
            
            if (_texturedRectangleShader == nullptr) {
                _texturedRectangleShader = _rendering->createShader("primitives_axis", texturedRectShaderSrc, {
                    {"ID", foundation::RenderShaderInputFormat::ID}
                });
            }
            
            struct ImgConst {
                math::vector2f position;
                math::vector2f size;
                math::vector4f color;
            }
            imgconst = {{x, y}, {w, h}, {0.5, 0.5, 0.5, 1}};
            
            _rendering->beginPass("primitives_txrect", _texturedRectangleShader);
            _rendering->applyShaderConstants(&imgconst);
            _rendering->applyTextures(&texture, 1);
            _rendering->drawGeometry(nullptr, 4, foundation::RenderTopology::TRIANGLESTRIP);
            _rendering->endPass();
        }
        
    protected:
        std::shared_ptr<foundation::RenderingInterface> _rendering;
        std::shared_ptr<foundation::RenderShader> _axisShader;
        std::shared_ptr<foundation::RenderShader> _sphereShader;
        std::shared_ptr<foundation::RenderShader> _texturedRectangleShader;
    };
}
