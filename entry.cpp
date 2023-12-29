
#include "foundation/platform.h"
#include "foundation/rendering.h"

foundation::PlatformInterfacePtr platform;
foundation::RenderingInterfacePtr rendering;
//
math::transform3f view = math::transform3f::identity();
math::transform3f proj = math::transform3f::identity();
math::vector3f camPosition = math::vector3f(10, 20, 30);
math::vector3f camTarget = math::vector3f(0, 0, 0);

const char *axisShaderSrc = R"(
    fixed {
        points[6] : float4 =
            [0.0, 0.0, 0.0, 1][1000.0, 0.0, 0.0, 1]
            [0.0, 0.0, 0.0, 1][0.0, 1000.0, 0.0, 1]
            [0.0, 0.0, 0.0, 1][0.0, 0.0, 1000.0, 1]
        colors[3] : float4 =
            [1.0, 0.0, 0.0, 1][0.0, 1.0, 0.0, 1][0.0, 0.0, 1.0, 1]
    }
    inout {
        color : float4
    }
    vssrc {
        output_position = _transform(fixed_points[vertex_ID], _transform(frame_viewMatrix, frame_projMatrix));
        output_color = fixed_colors[vertex_ID / 2];
    }
    fssrc {
        output_color[0] = input_color;
    }
)";


const char *testShaderSrc = R"(
    const {
        p[2] : float4
    }
    fixed {
        p[2] : float4 = [0, 0.0, 0.01, 0][0, 0.3, 0.01, 0]
    }
    inout {
        color : float4
    }
    fndef getcolor(float3 rgb) -> float4 {
        return float4(rgb, 1.0);
    }
    vssrc {
        output_position = vertex_p + fixed_p[repeat_ID]; //const_p[vertex_ID]; //
        output_color = getcolor(float3(1, 1, 1)) * vertex_c;
    }
    fssrc {
        output_color[0] = input_color;
    }
)";

const char *textureQuadShaderSrc = R"(
    fixed {
        p[4] : float4 =
            [0.0, 0.0, 0.01, 1]
            [0.0, 1.0, 0.01, 1]
            [1.0, 0.0, 0.01, 1]
            [1.0, 1.0, 0.01, 1]
    }
    inout {
        coord : float2
    }
    vssrc {
        float4 p = fixed_p[vertex_ID];
        output_position = float4(p.xy * 0.6, p.zw);
        output_coord = p.xy;
    }
    fssrc {
        output_color[0] = _tex2d(0, input_coord);
    }
)";


struct Vertex {
    math::vector4f camPosition;
    std::uint32_t color;
}
points[] = {
    {{0.0, 0, 0.1, 1}, 0xff0000ff},
    {{0.5, 0, 0.1, 1}, 0xff00ff00}
};

std::uint32_t textureData[] = {
    0xff00ffff, 0xff0000ff, 0xff0000ff, 0xff000000,
    0xff0000ff, 0xff0000ff, 0xff000000, 0xff000000,
    0xff0000ff, 0xff000000, 0xff0000ff, 0xff000000,
    0xff000000, 0xff000000, 0xff000000, 0xff0000ff,
};

foundation::RenderShaderPtr axisShader;
foundation::RenderShaderPtr testShader;
foundation::RenderShaderPtr texQuadShader;
foundation::RenderDataPtr testData;
foundation::RenderTexturePtr texture;

std::size_t pointerId = foundation::INVALID_POINTER_ID;
math::vector2f lockedCoordinates;
math::vector3f center = { 0, 0, 0 };
math::vector3f orbit = { 10, 20, 30 };

void setCameraLookAt(const math::vector3f &position, const math::vector3f &sceneCenter) {
    math::vector3f right = math::vector3f(0, 1, 0).cross(position - sceneCenter).normalized();
    math::vector3f nrmlook = (sceneCenter - position).normalized();
    math::vector3f nrmright = right.normalized();
    math::vector3f nrmup = nrmright.cross(nrmlook).normalized();
    
    float aspect = platform->getScreenWidth() / platform->getScreenHeight();
    view = math::transform3f::lookAtRH(position, sceneCenter, nrmup);
    proj = math::transform3f::perspectiveFovRH(50.0 / 180.0f * float(3.14159f), aspect, 0.1f, 10000.0f);
}

extern "C" void initialize() {
    platform = foundation::PlatformInterface::instance();
    rendering = foundation::RenderingInterface::instance(platform);
    
    platform->addPointerEventHandler(
        [](const foundation::PlatformPointerEventArgs &args) -> bool {
            if (args.type == foundation::PlatformPointerEventArgs::EventType::START) {
                pointerId = args.pointerID;
                lockedCoordinates = { args.coordinateX, args.coordinateY };
            }
            if (args.type == foundation::PlatformPointerEventArgs::EventType::MOVE) {
                if (pointerId != foundation::INVALID_POINTER_ID) {
                    float dx = args.coordinateX - lockedCoordinates.x;
                    float dy = args.coordinateY - lockedCoordinates.y;
                    
                    orbit.xz = orbit.xz.rotated(dx / 100.0f);
                    
                    math::vector3f right = math::vector3f(0, 1, 0).cross(orbit).normalized();
                    math::vector3f rotatedOrbit = orbit.rotated(right, dy / 100.0f);
                    
                    if (fabs(math::vector3f(0, 1, 0).dot(rotatedOrbit.normalized())) < 0.96f) {
                        orbit = rotatedOrbit;
                    }
                    
                    setCameraLookAt(center + orbit, center);
                    lockedCoordinates = { args.coordinateX, args.coordinateY };
                }
            }
            if (args.type == foundation::PlatformPointerEventArgs::EventType::FINISH) {
                pointerId = foundation::INVALID_POINTER_ID;
            }
            if (args.type == foundation::PlatformPointerEventArgs::EventType::CANCEL) {
                pointerId = foundation::INVALID_POINTER_ID;
            }
            
            return true;
        }
    );
    
    texture = rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, 4, 4, {textureData});
    texQuadShader = rendering->createShader("texquad", textureQuadShaderSrc, {});
    axisShader = rendering->createShader("axis", axisShaderSrc, {});
    /*
    testShader = rendering->createShader("test", testShaderSrc, foundation::InputLayout {
        .vertexRepeat = 1,
        .vertexAttributes = {
            {"p", foundation::InputAttributeFormat::FLOAT4},
            {"c", foundation::InputAttributeFormat::BYTE4_NRM}
        }
    });
    testData = rendering->createData(points, testShader->getInputLayout().vertexAttributes, 2);
    */

    setCameraLookAt(camPosition, camTarget);
    
    platform->setLoop([](float dtSec) {
        rendering->updateFrameConstants(view, proj, camPosition, (camTarget - camPosition));
        //rendering->applyState(testShader, foundation::RenderPassCommonConfigs::CLEAR(0.1, 0.1, 0.2));
        rendering->applyState(axisShader, foundation::RenderPassCommonConfigs::CLEAR(0.1, 0.1, 0.2));
        rendering->draw(6, foundation::RenderTopology::LINES);

        rendering->applyState(texQuadShader, foundation::RenderPassCommonConfigs::DEFAULT());
        rendering->applyTextures({
            {&texture, foundation::SamplerType::LINEAR}
        });
        //rendering->applyShaderConstants(points);
        rendering->draw(4, foundation::RenderTopology::TRIANGLESTRIP);
        //rendering->draw(testData, foundation::RenderTopology::LINES);
//        //platform->logMsg("-->> %d %d %f", (int)platform->getScreenWidth(), (int)platform->getScreenHeight(), dtSec);
//
        rendering->presentFrame();
    });
    
    //abort();
}

extern "C" void deinitialize() {

}
