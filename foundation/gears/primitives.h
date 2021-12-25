
#pragma once

#include "math.h"
#include "foundation/rendering.h"

namespace gears {
    class Primitives {
    public:
        Primitives(const std::shared_ptr<foundation::RenderingInterface> &rendering) : _rendering(rendering) {}

        void drawLine(const math::vector3f &p1, const math::vector3f &p2, const math::color &rgba) {
            static const char *lineShader = R"(
                const {
                    position[2] : float4
                    color : float4
                }
                vssrc {
                    output_position = _transform(position[vertex_ID], _viewProjMatrix);
                }
                fssrc {
                    output_color = color;
                }
            )";

            struct {
                math::vector4f position1;
                math::vector4f position2;
                math::vector4f color;
            }
            lineData{
                {p1, 1},
                {p2, 1},
                rgba
            };

            if (_lineShader == nullptr) {
                _lineShader = _rendering->createShader("primitives_line", lineShader, {
                    {"ID", foundation::RenderingShaderInputFormat::VERTEX_ID}
                    });
            }

            _rendering->applyShader(_lineShader, &lineData);
            _rendering->drawGeometry(2, foundation::RenderingTopology::LINES);
        }

        inline void drawCircleXZ(const math::vector3f &position, float radius, const math::color &rgba) {
            static const char *circleShader = R"(
                const {
                    position_radius : float4
                    color : float4
                }
                vssrc {
                    float4 p = float4(position_radius.xyz, 1);
                    p.x = p.x + position_radius.w * _cos(6.2831853 * float(vertex_ID) / 36.0);
                    p.z = p.z + position_radius.w * _sin(6.2831853 * float(vertex_ID) / 36.0);
                    output_position = _transform(p, _viewProjMatrix);
                }
                fssrc {
                    output_color = color;
                }
            )";

            struct {
                math::vector4f positionRadius;
                math::vector4f color;
            }
            circleData{
                {position, radius},
                rgba
            };

            if (_circleShader == nullptr) {
                _circleShader = _rendering->createShader("primitives_circle", circleShader, {
                    {"ID", foundation::RenderingShaderInputFormat::VERTEX_ID}
                });
            }

            _rendering->applyShader(_circleShader, &circleData);
            _rendering->drawGeometry(37, foundation::RenderingTopology::LINESTRIP);
        }

        void drawCylinderXZ(const math::vector3f &position, float radius, float height, const math::color &rgba) {

        }

        void drawCube(const math::vector3f &p1, const math::vector3f &p2, const math::color &rgba) {

        }

        void drawAxis() {
            static const char *axisShader = R"(
                fixed {
                    coords[6] : float4 = 
                        [0.0, 0.0, 0.0, 1.0][1000.0, 0.0, 0.0, 1.0]
                        [0.0, 0.0, 0.0, 1.0][0.0, 1000.0, 0.0, 1.0]
                        [0.0, 0.0, 0.0, 1.0][0.0, 0.0, 1000.0, 1.0]
                    colors[3] : float4 = 
                        [1.0, 0.0, 0.0, 1.0]
                        [0.0, 1.0, 0.0, 1.0]
                        [0.0, 0.0, 1.0, 1.0]
                }
                inout {
                    color : float4
                }
                vssrc {
                    output_position = _transform(coords[vertex_ID], _viewProjMatrix);
                    output_color = colors[vertex_ID >> 1];
                }
                fssrc {
                    output_color = input_color;
                }
            )";

            if (_axisShader == nullptr) {
                _axisShader = _rendering->createShader("primitives_axis", axisShader, {
                    {"ID", foundation::RenderingShaderInputFormat::VERTEX_ID}
                });
            }

            _rendering->applyShader(_axisShader);
            _rendering->drawGeometry(6, foundation::RenderingTopology::LINES);
        }

    protected:
        std::shared_ptr<foundation::RenderingInterface> _rendering;
        std::shared_ptr<foundation::RenderingShader> _axisShader;
        std::shared_ptr<foundation::RenderingShader> _lineShader;
        std::shared_ptr<foundation::RenderingShader> _circleShader;
    };
}