
#include <chrono>

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/gears/math.h"
#include "foundation/gears/camera.h"

static const char *sh_ref = R"(
    using namespace metal;
    
    struct _FrameData {
        float4x4 viewProjMatrix;
        float4 cameraPosition;
        float4 cameraDirection;
        float4 renderTargetBounds;
    };
    constant float4 const_multiplier[3] = {
        float4(0.3f, 0.3f, 0.3f, 1.0f),
        float4(0.3f, 0.3f, 0.3f, 1.0f),
        float4(0.3f, 0.3f, 0.3f, 1.0f),
    };
    constant float4 const_mul00000 = {
        float4(0.3f, 0.3f, 0.3f, 1.0f),
    };
    struct _Constants {
        float4 tmp;
    };
    struct _VSIn {
        float3 position [[attribute(0)]];
        float4 color [[attribute(1)]];
    };
    struct _InOut {
        float4 position [[position]];
        float4 color;
    };
    vertex _InOut main_vertex(unsigned int vid [[vertex_id]], _VSIn input [[stage_in]], constant _FrameData &framedata [[buffer(1)]], constant _Constants &constants [[buffer(2)]]) {
        _InOut result;
        result.position = framedata.viewProjMatrix * float4(input.position, 1.0);
        result.color = input.color + const_multiplier[vid >> 1];
        return result;
    }
    fragment float4 main_fragment(_InOut input [[stage_in]]) {
        return input.color;
    }
)";

int main(int argc, const char * argv[]) {
    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);
    gears::Camera camera (platform);
    
    static const char *axisShaderSrc = R"(
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
            output_position = _transform(fixed_coords[vertex_ID], frame_viewProjMatrix);
            output_color = fixed_colors[vertex_ID >> 1];
        }
        fssrc {
            output_color = input_color;
        }
    )";
    auto axisShader = rendering->createShader("primitives_axis", axisShaderSrc, {
        {"ID", foundation::RenderingShaderInputFormat::ID}
    });
    
    static const char *circleShaderSrc = R"(
        const {
            position_radius : float4
            color : float4
        }
        vssrc {
            float4 p = float4(const_position_radius.xyz, 1);
            p.x = p.x + const_position_radius.w * _cos(6.2831853 * float(vertex_ID) / 36.0);
            p.z = p.z + const_position_radius.w * _sin(6.2831853 * float(vertex_ID) / 36.0);
            output_position = _transform(p, frame_viewProjMatrix);
        }
        fssrc {
            output_color = const_color;
        }
    )";

    struct {
        math::vector4f positionRadius;
        math::vector4f color;
    }
    circleData{
        {2.5f, 0.0f, 1.0f, 1.0f},
        {1.0f, 0.5f, 0.0f, 1.0f}
    };

    auto circleShader = rendering->createShader("primitives_circle", circleShaderSrc, {
        {"ID", foundation::RenderingShaderInputFormat::ID}
    });
  
    static const char *testShaderSrc = R"(
        fixed {
            multiplier : float4 = [0.3, 0.3, 0.3, 1.0]
        }
        inout {
            color : float4
        }
        vssrc {
            float4 p = float4(vertex_position, 1.0);
            p.z = p.z + 2.0 * instance_ID;
            output_position = _transform(p, frame_viewProjMatrix);
            output_color = vertex_color + fixed_multiplier;
        }
        fssrc {
            output_color = input_color;
        }
    )";

    auto testShader = rendering->createShader("name", testShaderSrc, {
        {"position", foundation::RenderingShaderInputFormat::FLOAT3},
        {"color", foundation::RenderingShaderInputFormat::BYTE4_NRM}
    }, {
        {"ID", foundation::RenderingShaderInputFormat::ID},
    });

    struct Vertex {
        math::vector3f position;
        std::uint32_t color;
    };

    Vertex triangle[] = {
        {{0.0f, 0.0f, 0.0f}, 0xff000000},
        {{1.0f, 0.0f, 0.0f}, 0xff0000ff},
        {{0.0f, 1.0f, 0.0f}, 0xff00ff00},

        {{0.0f, 0.0f, 0.0f}, 0xff000000},
        {{0.0f, 1.0f, 0.0f}, 0xff00ff00},
        {{0.0f, 0.0f, 1.0f}, 0xffff0000},

        {{0.0f, 0.0f, 0.0f}, 0xff000000},
        {{0.0f, 0.0f, 1.0f}, 0xffff0000},
        {{1.0f, 0.0f, 0.0f}, 0xff0000ff},
    };

    auto geometry = rendering->createData(triangle, 9, sizeof(Vertex));
    
    platform->run([&](float dtSec) {
        rendering->updateFrameConstants(camera.getPosition().flat3, camera.getForwardDirection().flat3, camera.getVPMatrix().flat16);
        rendering->beginPass("axis", axisShader, nullptr);
        rendering->drawGeometry(nullptr, 6, foundation::RenderingTopology::LINES);
        rendering->endPass();

        rendering->beginPass("circle", circleShader, &circleData);
        rendering->drawGeometry(nullptr, 37, foundation::RenderingTopology::LINESTRIP);
        rendering->endPass();

        rendering->beginPass("test", testShader, nullptr);
        rendering->drawGeometry(geometry, nullptr, 9, 3, foundation::RenderingTopology::TRIANGLES);
        rendering->endPass();

        rendering->presentFrame();
    });

    return 0;
}

