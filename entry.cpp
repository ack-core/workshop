
#include "foundation/platform.h"
#include "foundation/rendering.h"

foundation::PlatformInterfacePtr platform;
foundation::RenderingInterfacePtr rendering;
//
//math::transform3f view = math::transform3f::identity();
//math::transform3f proj = math::transform3f::identity();
//math::vector4f camPosition = math::vector4f(0, 0, 0, 1);
//math::vector4f camDirection = math::vector4f(1, 0, 0, 0);
//
const char *testShaderSrc = R"(
    const {
        p[2] : float4
    }
    fixed {
        test[2] : float4 = [-0.5, 0.5, 0.5, 1.0][-0.5, -0.5, 0.5, 1.0]
    }
    inout {
        color : float4
    }
    fndef getcolor(float3 rgb) -> float4 {
        return float4(rgb, 1.0);
    }
    vssrc {
        float4 position = const_p[vertex_ID];
        output_position = _transform(position, _transform(frame_viewMatrix, frame_projMatrix));
        output_color = getcolor(float3(1, 1, 1));
    }
    fssrc {
        output_color[0] = input_color;
    }
)";
foundation::RenderShaderPtr testShader;

//struct AsyncContext {
//    int result;
//};

extern "C" void initialize() {
    platform = foundation::PlatformInterface::instance();
    rendering = foundation::RenderingInterface::instance(platform);
    
    //char *t = nullptr;
    //float f1 = std::strtof("567.34", &t);
    platform->logMsg("Yes! %f %s\n", 104326.45, "eee!");
    
    
//    const char *text = "678     ";
//    int value = 0;
//
//    if (std::from_chars(text, text+10, value).ec == std::errc()) {
//        platform->logMsg("Yes! %d\n", value);
//    }
//    else {
//        platform->logMsg("No!");
//    }
    
    testShader = rendering->createShader("test", testShaderSrc, {}, {});

//    platform->logMsg("[PLATFORM] initializing...");
//    platform->loadFile("test1.txt", [p = platform](std::unique_ptr<std::uint8_t[]> &&data, std::size_t size) {
//        if (data) {
//            p->logMsg("-->>> %d %s", int(size), data.get());
//        }
//    });
//    platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([p = platform, x = 10, y = 20](AsyncContext &ctx) {
//        ctx.result = x + y;
//        p->logMsg("-->>> working! %d %d\n", x, y);
//    },
//    [p = platform](AsyncContext &ctx) {
//        p->logMsg("-->>> result! %d\n", ctx.result);
//    }));
    
    platform->setLoop([](float dtSec) {
//        math::vector4f p[] = {
//            {0, 0, 0.1, 1},
//            {1, 0, 0.1, 1}
//        };
//
//        rendering->updateFrameConstants(view, proj, camPosition.xyz, camDirection.xyz);
//        rendering->applyState(testShader, foundation::RenderPassCommonConfigs::CLEAR(0.2, 0.2, 0.1));
//        rendering->applyShaderConstants(p);
//        rendering->drawGeometry(nullptr, 2, foundation::RenderTopology::LINES);
//        //platform->logMsg("-->> %d %d %f", (int)platform->getScreenWidth(), (int)platform->getScreenHeight(), dtSec);
//
        //rendering->presentFrame();
    });
    
    abort();
}

extern "C" void deinitialize() {

}
