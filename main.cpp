
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
        texcoord : float4
    }
    inout {
        texcoord : float2
    }
    vssrc {
        float2 screenTransform = float2(2.0, -2.0) / _renderTargetBounds.xy;
        float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
        output_texcoord = _lerp(vertexCoord, texcoord.xy, texcoord.zw);
        vertexCoord = vertexCoord * position_size.zw + position_size.xy;
        output_position = float4(vertexCoord.xy * screenTransform + float2(-1.0, 1.0), 0.5, 1); //
    }
    fssrc {
        output_color = _tex2d(0, input_texcoord);
    }
)";

const std::uint32_t tx[] = {
    0xff00ffff, 0xffffffff, 0xffffffff, 0xffff00ff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xff00ffff, 0xffffffff, 0xffffffff, 0xffff00ff,
};

int main(int argc, const char * argv[]) {
    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);

    auto texture = rendering->createTexture(foundation::RenderingTextureFormat::RGBA8UN, 4, 4, { (const std::uint8_t *)tx });
    auto files = platform->formFileList("");
    
    std::unique_ptr<uint8_t[]> data;
    std::size_t size;
    platform->loadFile("system/pixelperfect.png", data, size);

    auto camera = std::make_unique<Camera>(platform);
    auto primitives = std::make_unique<Primitives>(rendering);

    auto shader = rendering->createShader("ui", g_srcUI, {
        {"ID", foundation::RenderingShaderInputFormat::VERTEX_ID},
    });

    struct {
        float p[4] = {0, 0, 50.0, 50.0};
        float t[4] = {0, 0, 1, 1};
    }
    uidata;

    platform->run([&](float dtSec) {
        rendering->updateCameraTransform(camera->getPosition().flat3, camera->getForwardDirection().flat3, camera->getVPMatrix().flat16);
        rendering->prepareFrame();

        primitives->drawAxis();
        primitives->drawCircleXZ({}, 1.0f, { 1, 0, 0, 1 });

        rendering->applyShader(shader, &uidata);
        rendering->applyTextures({ texture.get() });
        rendering->drawGeometry(4, foundation::RenderingTopology::TRIANGLESTRIP);

        rendering->presentFrame(dtSec);
    });

    return 0;
}

