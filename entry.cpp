
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "providers/texture_provider.h"
#include "providers/mesh_provider.h"

foundation::PlatformInterfacePtr platform;
foundation::RenderingInterfacePtr rendering;
resource::TextureProviderPtr textureProvider;
resource::MeshProviderPtr meshProvider;

math::transform3f view = math::transform3f::identity();
math::transform3f proj = math::transform3f::identity();

//std::size_t pointerId = foundation::INVALID_POINTER_ID;
//math::vector2f lockedCoordinates;
//math::vector3f center = { 0, 0, 0 };
//math::vector3f orbit = { 0, 120, 0 };
//math::vector3f right = { 0, 0, 1.0f };
//
////
//void setCameraLookAt(const math::vector3f &position, const math::vector3f &sceneCenter, const math::vector3f &right) {
//    //math::vector3f right = math::vector3f(0, 1, 0).cross(position - sceneCenter).normalized();
//    math::vector3f nrmlook = (sceneCenter - position).normalized();
//    math::vector3f nrmright = right.normalized();
//    math::vector3f nrmup = nrmright.cross(nrmlook).normalized();
//
//    float aspect = platform->getScreenWidth() / platform->getScreenHeight();
//    view = math::transform3f::lookAtRH(position, sceneCenter, nrmup);
//    proj = math::transform3f::perspectiveFovRH(50.0 / 180.0f * float(3.14159f), aspect, 0.1f, 10000.0f);
//}

//auto pointerHandler = [](const foundation::PlatformPointerEventArgs &args) -> bool {
//    if (args.type == foundation::PlatformPointerEventArgs::EventType::START) {
//        pointerId = args.pointerID;
//        lockedCoordinates = { args.coordinateX, args.coordinateY };
//    }
//    if (args.type == foundation::PlatformPointerEventArgs::EventType::MOVE) {
//        if (pointerId != foundation::INVALID_POINTER_ID) {
//            float dx = args.coordinateX - lockedCoordinates.x;
//            float dy = args.coordinateY - lockedCoordinates.y;
//
//            orbit.yz = orbit.yz.rotated(-dx / 200.0f);
//
//            right = math::vector3f(1, 0, 0).cross(orbit).normalized();
//            math::vector3f rotatedOrbit = orbit.rotated(right, dy / 200.0f);
//
//            if (fabs(math::vector3f(1, 0, 0).dot(rotatedOrbit.normalized())) < 0.96f) {
//                orbit = rotatedOrbit;
//            }
//
//            lockedCoordinates = { args.coordinateX, args.coordinateY };
//        }
//    }
//    if (args.type == foundation::PlatformPointerEventArgs::EventType::FINISH) {
//        pointerId = foundation::INVALID_POINTER_ID;
//    }
//    if (args.type == foundation::PlatformPointerEventArgs::EventType::CANCEL) {
//        pointerId = foundation::INVALID_POINTER_ID;
//    }
//
//    return true;
//};


const char *quadsShaderSrc1 = R"(
    fixed {
        p[4] : float4 =
            [-0.1, -0.1, 0.01, 1]
            [-0.1, 0.1, 0.01, 1]
            [0.1, -0.1, 0.01, 1]
            [0.1, 0.1, 0.01, 1]
    }
    vssrc {
        output_position = fixed_p[repeat_ID];
        output_position.x -= 0.7;
    }
    fssrc {
        output_color[0] = float4(1, 0, 0, 1);
    }
)";
const char *quadsShaderSrc2 = R"(
    fixed {
        p[4] : float4 =
            [-0.1, -0.1, 0.01, 1]
            [-0.1, 0.1, 0.01, 1]
            [0.1, -0.1, 0.01, 1]
            [0.1, 0.1, 0.01, 1]
    }
    const {
        inst[3] : float4
    }
    vssrc {
        output_position = fixed_p[repeat_ID] + const_inst[instance_ID];
    }
    fssrc {
        output_color[0] = float4(1, 1, 0, 1);
    }
)";
const char *quadsShaderSrc3 = R"(
    vssrc {
        output_position = vertex_position;
        output_position.x -= 0.2;
    }
    fssrc {
        output_color[0] = float4(0, 1, 0, 1);
    }
)";
const char *quadsShaderSrc4 = R"(
    fixed {
        p[4] : float4 =
            [-0.1, -0.1, 0.01, 1]
            [-0.1, 0.1, 0.01, 1]
            [0.1, -0.1, 0.01, 1]
            [0.1, 0.1, 0.01, 1]
    }
    vssrc {
        output_position = fixed_p[repeat_ID] + vertex_position;
    }
    fssrc {
        output_color[0] = float4(0, 1, 1, 1);
    }
)";
const char *quadsShaderSrc5 = R"(
    fixed {
        p[4] : float4 =
            [-0.1, -0.1, 0.01, 1]
            [-0.1, 0.1, 0.01, 1]
            [0.1, -0.1, 0.01, 1]
            [0.1, 0.1, 0.01, 1]
    }
    const {
        inst[3] : float4
    }
    vssrc {
        output_position = (fixed_p[repeat_ID] + vertex_position) * const_inst[instance_ID];
    }
    fssrc {
        output_color[0] = float4(1, 0, 1, 1);
    }
)";

foundation::RenderShaderPtr quadsShader1;
foundation::RenderShaderPtr quadsShader2;
foundation::RenderShaderPtr quadsShader3;
foundation::RenderShaderPtr quadsShader4;
foundation::RenderShaderPtr quadsShader5;

foundation::RenderDataPtr testData3;
foundation::RenderDataPtr testData4;
foundation::RenderDataPtr testData5;

extern "C" void initialize() {
    platform = foundation::PlatformInterface::instance();
    rendering = foundation::RenderingInterface::instance(platform);
    textureProvider = resource::TextureProvider::instance(platform, rendering);
    meshProvider = resource::MeshProvider::instance(platform);
    
    //platform->addPointerEventHandler(std::move(pointerHandler));
    
    quadsShader1 = rendering->createShader("quads1", quadsShaderSrc1, foundation::InputLayout {
        .repeat = 4
    });
    quadsShader2 = rendering->createShader("quads2", quadsShaderSrc2, foundation::InputLayout {
        .repeat = 4
    });
    quadsShader3 = rendering->createShader("quads3", quadsShaderSrc3, foundation::InputLayout {
        .attributes = {
            {"position", foundation::InputAttributeFormat::FLOAT4}
        }
    });
    quadsShader4 = rendering->createShader("quads4", quadsShaderSrc4, foundation::InputLayout {
        .repeat = 4,
        .attributes = {
            {"position", foundation::InputAttributeFormat::FLOAT4}
        }
    });
    quadsShader5 = rendering->createShader("quads5", quadsShaderSrc5, foundation::InputLayout {
        .repeat = 4,
        .attributes = {
            {"position", foundation::InputAttributeFormat::FLOAT4}
        }
    });

    math::vector4f positions3[4] = {
        math::vector4f(-0.1, -0.1, 0.01, 1),
        math::vector4f(-0.1, 0.1, 0.01, 1),
        math::vector4f(0.1, -0.1, 0.01, 1),
        math::vector4f(0.1, 0.1, 0.01, 1)
    };
    testData3 = rendering->createData(positions3, quadsShader3->getInputLayout(), 4);

    math::vector4f positions4[2] = {
        math::vector4f(0.0, -0.3, 0.0, 0),
        math::vector4f(0.0, +0.3, 0.0, 0)
    };
    testData4 = rendering->createData(positions4, quadsShader4->getInputLayout(), 2);

    math::vector4f positions5[2] = {
        math::vector4f(0.5, -0.3, 0.0, 0),
        math::vector4f(0.5, +0.3, 0.0, 0)
    };
    testData5 = rendering->createData(positions5, quadsShader5->getInputLayout(), 2);

    platform->setLoop([](float dtSec) {
        rendering->applyState(quadsShader1, foundation::RenderTopology::TRIANGLESTRIP, foundation::RenderPassCommonConfigs::CLEAR(0, 0, 0));
        rendering->draw();
        
        rendering->applyState(quadsShader2, foundation::RenderTopology::TRIANGLESTRIP, foundation::RenderPassCommonConfigs::DEFAULT());
        math::vector4f instpos[3] = {
            math::vector4f(-0.5, -0.3, 0, 0),
            math::vector4f(-0.5, +0.0, 0, 0),
            math::vector4f(-0.5, +0.3, 0, 0)
        };
        rendering->applyShaderConstants(instpos);
        rendering->draw(nullptr, 3);
        
        rendering->applyState(quadsShader3, foundation::RenderTopology::TRIANGLESTRIP, foundation::RenderPassCommonConfigs::DEFAULT());
        rendering->draw(testData3);
        
        rendering->applyState(quadsShader4, foundation::RenderTopology::TRIANGLESTRIP, foundation::RenderPassCommonConfigs::DEFAULT());
        rendering->draw(testData4);
        
        rendering->applyState(quadsShader5, foundation::RenderTopology::TRIANGLESTRIP, foundation::RenderPassCommonConfigs::DEFAULT());
        math::vector4f instmul[3] = {
            math::vector4f(1.0, 0.1, 1, 1),
            math::vector4f(1.0, 0.3, 1, 1),
            math::vector4f(1.0, 0.9, 1, 1)
        };
        rendering->applyShaderConstants(instmul);
        rendering->draw(testData5, 3);
        
        rendering->presentFrame();
    });
}

extern "C" void deinitialize() {

}
