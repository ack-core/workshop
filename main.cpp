
#include "foundation/platform/interfaces.h"
#include "foundation/rendering/interfaces.h"

#include "foundation/math.h"
#include "foundation/camera.h"
#include "foundation/primitives.h"

const char *g_srcUI = R"(
        fixed {
            test[3] : float4 = 
                [1.0, 0.5, 0.25, 0.125]
                [1.0, 0.5, 0.25, 0.125]
                [1.0, 0.5, 0.25, 0.125]

            test1 : float3 = [1.0, 0.5, 0.25]
        }
        const {
            position_size : float4
        }
        vssrc {
            float2 screenTransform = float2(2.0, -2.0) / _renderTargetBounds.xy;
            float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
            vertexCoord = vertexCoord * position_size.zw;
            output_position = float4(vertexCoord.xy * screenTransform + float2(-1.0, 1.0), 0.5, 1); //
        }
        fssrc {
            output_color = float4(1.0, 1.0, 1.0, 1.0);
        }
    )";

int main(int argc, const char * argv[]) {
    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);

    auto files = platform->formFileList("");

    auto camera = std::make_unique<Camera>(platform);
    auto primitives = std::make_unique<Primitives>(rendering);

    //auto shader = rendering->createShader("ui", g_srcUI, {
    //    {"ID", foundation::RenderingShaderInputFormat::VERTEX_ID},
    //});

    //struct {
    //    float p[4] = {0, 0, 50.0, 50.0};
    //}
    //uidata;

    platform->run([&](float dtSec) {
        rendering->updateCameraTransform(camera->getPosition().flat3, camera->getForwardDirection().flat3, camera->getVPMatrix().flat16);
        rendering->prepareFrame();
        //rendering->applyShader(shader, &uidata);
        //rendering->drawGeometry(4, foundation::RenderingTopology::TRIANGLESTRIP);

        //primitives->drawLine({}, { 0, 1, 0 }, { 0, 1, 0, 1 });
        primitives->drawAxis();
        primitives->drawCircleXZ({}, 1.0f, {1, 0, 0, 1});

        rendering->presentFrame(dtSec);
    });

    return 0;
}

