
#include "platform/interfaces.h"
#include "rendering/interfaces.h"

const char *g_src = R"(
        const {
            position_size : float4
        }
        vssrc {
            float2 screenTransform = float2(2.0, -2.0) / _renderTargetBounds.xy;
            float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
            vertexCoord = vertexCoord * position_size.zw;
            output_position = float4(vertexCoord.xy, 0.01, 1); //
        }
        fssrc {
            output_color = float4(1.0, 1.0, 1.0, 1.0);
        }
    )";

int main(int argc, const char * argv[]) {
    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);

    auto shader = rendering->createShader("ui", g_src, {
        {"ID", foundation::RenderingShaderInputFormat::VERTEX_ID},
    });

    struct {
        float p[4] = {0, 0, 0.5, 0.5};
    }
    uidata;

    platform->run([&](float dtSec) {
        rendering->prepareFrame();
        rendering->applyShader(shader, &uidata);
        rendering->drawGeometry(4, foundation::RenderingTopology::TRIANGLESTRIP);
        rendering->presentFrame(dtSec);
    });

    return 0;
}

