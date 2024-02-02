
#include "foundation/platform.h"
#include "foundation/rendering.h"

foundation::PlatformInterfacePtr platform;
foundation::RenderingInterfacePtr rendering;


//
math::transform3f view = math::transform3f::identity();
math::transform3f proj = math::transform3f::identity();
math::vector3f camPosition = math::vector3f(10, 20, 30);
math::vector3f camTarget = math::vector3f(0, 0, 0);
//
//const char *axisShaderSrc = R"(
//    fixed {
//        points[6] : float4 =
//            [0.0, 0.0, 0.0, 1][1000.0, 0.0, 0.0, 1]
//            [0.0, 0.0, 0.0, 1][0.0, 1000.0, 0.0, 1]
//            [0.0, 0.0, 0.0, 1][0.0, 0.0, 1000.0, 1]
//        colors[3] : float4 =
//            [1.0, 0.0, 0.0, 1][0.0, 1.0, 0.0, 1][0.0, 0.0, 1.0, 1]
//    }
//    inout {
//        color : float4
//    }
//    vssrc {
//        output_position = _transform(fixed_points[vertex_ID], _transform(frame_viewMatrix, frame_projMatrix));
//        output_color = fixed_colors[vertex_ID / 2];
//    }
//    fssrc {
//        output_color[0] = float4(input_color.xyz * 0.5, 1.0);
//        output_color[1] = input_color;
//    }
//)";

//std::uint32_t textureData[] = {
//     0xff00ffff, 0xff0000ff, 0xff0000ff, 0xff000000,
//     0xff0000ff, 0xff0000ff, 0xff000000, 0xff000000,
//     0xff0000ff, 0xff000000, 0xff0000ff, 0xff000000,
//     0xff000000, 0xff000000, 0xff000000, 0xff0000ff,
// };
//
//foundation::RenderShaderPtr axisShader;
//foundation::RenderTexturePtr texture;
//foundation::RenderTargetPtr target;
//
//std::size_t pointerId = foundation::INVALID_POINTER_ID;
//math::vector2f lockedCoordinates;
//math::vector3f center = { 0, 0, 0 };
//math::vector3f orbit = { 10, 20, 30 };
//
//void setCameraLookAt(const math::vector3f &position, const math::vector3f &sceneCenter) {
//    math::vector3f right = math::vector3f(0, 1, 0).cross(position - sceneCenter).normalized();
//    math::vector3f nrmlook = (sceneCenter - position).normalized();
//    math::vector3f nrmright = right.normalized();
//    math::vector3f nrmup = nrmright.cross(nrmlook).normalized();
//
//    float aspect = platform->getScreenWidth() / platform->getScreenHeight();
//    view = math::transform3f::lookAtRH(position, sceneCenter, nrmup);
//    proj = math::transform3f::perspectiveFovRH(50.0 / 180.0f * float(3.14159f), aspect, 0.1f, 10000.0f);
//}

//const char *quadsShaderSrc = R"(
//    fixed {
//        p[4] : float4 =
//            [0.0, 0.0, 0.01, 1]
//            [0.0, 1.0, 0.01, 1]
//            [1.0, 0.0, 0.01, 1]
//            [1.0, 1.0, 0.01, 1]
//    }
//    inout {
//        coord : float2
//    }
//    vssrc {
//        float4 p = fixed_p[repeat_ID];
//        output_position = float4(vertex_position.xy + p.xy * vertex_position.zw, p.zw);
//        output_coord = p.xy;
//    }
//    fssrc {
//        output_color[0] = float4(1, 1, 1, 1);
//    }
//)";

const char *quadsShaderSrc1 = R"(
    fixed {
        p[4] : float4 =
            [-0.1, -0.1, 0.01, 1]
            [-0.1, 0.1, 0.01, 1]
            [0.1, -0.1, 0.01, 1]
            [0.1, 0.1, 0.01, 1]
    }
    vssrc {
        output_position = fixed_p[vertex_ID];
    }
    fssrc {
        output_color[0] = float4(1, 0, 0, 1);
    }
)";
const char *quadsShaderSrc2 = R"(
    fixed {
        p[4] : float4 =
            [0.0, 0.0, 0.01, 1]
            [0.0, 0.5, 0.01, 1]
            [0.5, 0.0, 0.01, 1]
            [0.5, 0.5, 0.01, 1]
    }
    inout {
        coord : float2
    }
    vssrc {
        float4 p = fixed_p[repeat_ID];
        output_position = float4(vertex_position.xy + p.xy * vertex_position.zw, p.zw);
        output_position.y = output_position.y + -0.5;
    }
    fssrc {
        output_color[0] = float4(0, 1, 0, 1);
    }
)";
const char *quadsShaderSrc3 = R"(
    fixed {
        p[4] : float4 =
            [0.0, 0.0, 0.01, 1]
            [0.0, 0.5, 0.01, 1]
            [0.5, 0.0, 0.01, 1]
            [0.5, 0.5, 0.01, 1]
    }
    inout {
        coord : float2
    }
    vssrc {
        float4 p = fixed_p[repeat_ID];
        output_position = float4(vertex_position.xy + p.xy * vertex_position.zw, p.zw);
        output_position.xy = output_position.xy + instance_position;
        output_coord = p.xy;
    }
    fssrc {
        output_color[0] = float4(1, 1, 0, 1);
    }
)";

foundation::RenderShaderPtr quadsShader1;
foundation::RenderShaderPtr quadsShader2;
foundation::RenderShaderPtr quadsShader3;

foundation::RenderDataPtr testData;
foundation::RenderDataPtr instData;


extern "C" void initialize() {
    platform = foundation::PlatformInterface::instance();
    rendering = foundation::RenderingInterface::instance(platform);
    
//    platform->addPointerEventHandler(
//        [](const foundation::PlatformPointerEventArgs &args) -> bool {
//            if (args.type == foundation::PlatformPointerEventArgs::EventType::START) {
//                pointerId = args.pointerID;
//                lockedCoordinates = { args.coordinateX, args.coordinateY };
//            }
//            if (args.type == foundation::PlatformPointerEventArgs::EventType::MOVE) {
//                if (pointerId != foundation::INVALID_POINTER_ID) {
//                    float dx = args.coordinateX - lockedCoordinates.x;
//                    float dy = args.coordinateY - lockedCoordinates.y;
//
//                    orbit.xz = orbit.xz.rotated(dx / 100.0f);
//
//                    math::vector3f right = math::vector3f(0, 1, 0).cross(orbit).normalized();
//                    math::vector3f rotatedOrbit = orbit.rotated(right, dy / 100.0f);
//
//                    if (fabs(math::vector3f(0, 1, 0).dot(rotatedOrbit.normalized())) < 0.96f) {
//                        orbit = rotatedOrbit;
//                    }
//
//                    setCameraLookAt(center + orbit, center);
//                    lockedCoordinates = { args.coordinateX, args.coordinateY };
//                }
//            }
//            if (args.type == foundation::PlatformPointerEventArgs::EventType::FINISH) {
//                pointerId = foundation::INVALID_POINTER_ID;
//            }
//            if (args.type == foundation::PlatformPointerEventArgs::EventType::CANCEL) {
//                pointerId = foundation::INVALID_POINTER_ID;
//            }
//
//            return true;
//        }
//    );
    
    //texture = rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, 4, 4, {textureData});
    //target = rendering->createRenderTarget(foundation::RenderTextureFormat::RGBA8UN, 2, 64, 64, true);
    
    math::vector4f positions[3] = {
        math::vector4f(0.0, 0.0, 0.1, 0.1),
        math::vector4f(0.1, 0.1, 0.2, 0.2),
        math::vector4f(0.3, 0.3, 0.3, 0.3)
    };
    math::vector2f inst[2] = {
        math::vector2f(-0.5, 0.0),
        math::vector2f(0.5, 0.0)
    };

    quadsShader1 = rendering->createShader("texquad1", quadsShaderSrc1, foundation::InputLayout {});
    quadsShader2 = rendering->createShader("texquad2", quadsShaderSrc2, foundation::InputLayout {
        .vertexRepeat = 4,
        .vertexAttributes = {
            {"position", foundation::InputAttributeFormat::FLOAT4}
        }
    });
    quadsShader3 = rendering->createShader("texquad3", quadsShaderSrc3, foundation::InputLayout {
        .vertexRepeat = 4,
        .vertexAttributes = {
            {"position", foundation::InputAttributeFormat::FLOAT4}
        },
        .instanceAttributes = {
            {"position", foundation::InputAttributeFormat::FLOAT2}
        }
    });
    
    testData = rendering->createData(positions, quadsShader2->getInputLayout().vertexAttributes, 3);
    instData = rendering->createData(inst, quadsShader3->getInputLayout().instanceAttributes, 2);
    
//    axisShader = rendering->createShader("axis", axisShaderSrc, {});
//
//    setCameraLookAt(camPosition, camTarget);
    
    platform->setLoop([](float dtSec) {
//        rendering->updateFrameConstants(view, proj, camPosition, (camTarget - camPosition));
//        rendering->applyState(axisShader, foundation::RenderPassCommonConfigs::CLEAR(target, 0.4, 0.4, 0.4));
//        rendering->draw(6, foundation::RenderTopology::LINES);

        rendering->applyState(quadsShader3, foundation::RenderPassCommonConfigs::CLEAR(0.3, 0.3, 0.3));
        rendering->drawInstanced(testData, instData, foundation::RenderTopology::TRIANGLESTRIP);

        rendering->applyState(quadsShader2, foundation::RenderPassCommonConfigs::DEFAULT());
        rendering->draw(testData, foundation::RenderTopology::TRIANGLESTRIP);

        rendering->applyState(quadsShader1, foundation::RenderPassCommonConfigs::DEFAULT());
        rendering->draw(4, foundation::RenderTopology::TRIANGLESTRIP);

        rendering->presentFrame();


//        rendering->applyTextures({
//            //{&texture, foundation::SamplerType::LINEAR}
//            {&target->getTexture(1), foundation::SamplerType::LINEAR}
//        });

        //rendering->draw(testData, foundation::RenderTopology::TRIANGLESTRIP);
    });
    
    //abort();
}

extern "C" void deinitialize() {

}
