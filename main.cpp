
#include "platform/interfaces.h"
#include "rendering/interfaces.h"

const char *g_src = R"(
        const {
            position_size : float4
            texcoord : float4
        }
        cross {
            texcoord : float2
        }
        vssrc {
            float2 screenTransform = float2(2.0, -2.0) / _renderTargetBounds.xy;
            float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
            output_texcoord = _lerp(vertexCoord, texcoord.xy, texcoord.zw);
            vertexCoord = vertexCoord * position_size.zw + position_size.xy;
            output_position = float4(vertexCoord.xy * screenTransform + float2(-1.0, 1.0), 0, 1); //
        }
        fssrc {
            output_color = _tex2d(0, input_texcoord);
        }
    )";

int main(int argc, const char * argv[]) {
    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);

    auto shader = rendering->createShader(g_src, {
        {"ID", foundation::RenderingShaderInputFormat::VERTEX_ID},
    });

    platform->run([&](float dtSec) {
        rendering->prepareFrame();

        rendering->presentFrame(dtSec);
    });

    return 0;
}

