
#include <chrono>

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/gears/math.h"
#include "foundation/gears/camera.h"
#include "foundation/gears/orbit_camera_controller.h"
#include "foundation/gears/primitives.h"

#include "voxel/mesh_factory.h"
#include "voxel/scene.h"

std::uint32_t textureData[] = {
    0xff0000ff, 0xff0000ff, 0xffffffff, 0xffffffff,
    0xff0000ff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffff0000,
    0xffffffff, 0xffffffff, 0xffff0000, 0xffff0000,
};

//const char *voxShaderSrc = R"(
//    fixed {
//        cube[12] : float4 =
//            [-0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
//            [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0][0.5, 0.5, -0.5, 1.0]
//            [0.5, 0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0]
//    }
//    const {
//        lightPosition : float3
//        lightRadius : float1
//    }
//    inout {
//        direction : float3
//    }
//    vssrc {
//        //float3 cubeCenter = float3(instance_position.xyz);
//        //float3 toCamSign = _sign(frame_cameraPosition.xyz - cubeCenter);
//        //float4 relVertexPos = float4(toCamSign, 1.0) * fixed_cube[vertex_ID];
//        //float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
//
//        float3 toLight = _norm(const_lightPosition - float3(instance_position.xyz));
//        float3 toLightSign = _sign(toLight);
//
//        float3 octahedron = toLight / _dot(toLight, toLightSign);
//        octahedron.xz = octahedron.y <= 0.0 ? toLightSign.xz * (float2(1.0, 1.0) - _abs(octahedron.zx)) : octahedron.xz;
//
//        output_position = float4((octahedron.x + octahedron.z), (octahedron.x - octahedron.z), 0.5, 1);
//        output_direction = toLight;
//    }
//    fssrc {
//        output_color = float4(input_direction.x, input_direction.y, input_direction.z, 1);
//    }
//)";


const char *voxShaderSrc = R"(
    fixed {
        cube[12] : float4 =
            [-0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
            [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0][0.5, 0.5, -0.5, 1.0]
            [0.5, 0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0]
    }
    const {
        lightPosition : float3
        lightRadius : float1
    }
    inout {
        dist : float1
    }
    vssrc {
        float3 cubeCenter = float3(instance_position.xyz);
        float3 toLightSign = _sign(const_lightPosition - float3(instance_position.xyz));
        float4 relVertexPos = float4(toLightSign, 1.0) * fixed_cube[vertex_ID];
        float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
        
        float3 vToLight = const_lightPosition - absVertexPos.xyz;
        float3 vToLightNrm = _norm(vToLight);
        float3 vToLightSign = _sign(vToLight);
        
        float3 octahedron = vToLightNrm / _dot(vToLightNrm, vToLightSign);
        octahedron.xz = octahedron.y <= 0.0 ? vToLightSign.xz * (float2(1.0, 1.0) - _abs(octahedron.zx)) : octahedron.xz;
        
        output_dist = clamp((length(vToLight) / const_lightRadius), 0.0, 1.0);
        output_position = float4((octahedron.x + octahedron.z), (octahedron.x - octahedron.z), 0.5, 1);
    }
    fssrc {
        output_color = float4(input_dist, 0, 0, 1);
    }
)";

int main(int argc, const char * argv[]) {
    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);
    
    auto camera = std::make_shared<gears::Camera>(platform->getScreenWidth(), platform->getScreenHeight());
    auto primitives = std::make_shared<gears::Primitives>(rendering);
    
    auto meshFactory = voxel::MeshFactory::instance(platform);
    auto scene = voxel::Scene::instance(meshFactory, rendering, "");
    scene->addStaticModel("walls.vox", {-36, -16, -36});
    
    gears::OrbitCameraController cameraController (platform, camera);
    cameraController.setEnabled(true);
    
    auto voxShader = rendering->createShader("vox", voxShaderSrc,
        { // vertex
            {"ID", foundation::RenderShaderInputFormat::ID}
        },
        { // instance
            {"position", foundation::RenderShaderInputFormat::SHORT4},
        }
    );
//    voxel::VoxelPosition positions[] = {
//        {1, -1, 1, 0},
//        {2, -1, 1, 0}
//    };
    
    struct Light {
        math::vector3f position;
        float radius;
    }
    light {
        {0.0f, 0.0f, 0.0f}, 30.0f
    };
    
    auto imageTexture = rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, 4, 4, {&textureData});
    auto rt0 = rendering->createRenderTarget(foundation::RenderTextureFormat::R32F, 256, 256, false);
    //auto voxData = rendering->createData(&positions, 2, sizeof(voxel::VoxelPosition));
    
    platform->run([&](float dtSec) {
        rendering->updateFrameConstants(camera->getPosition().flat3, camera->getForwardDirection().flat3, camera->getVPMatrix().flat16);
        primitives->drawAxis(foundation::RenderPassConfig(0.8f, 0.775f, 0.75f));

        rendering->beginPass("vox", voxShader, foundation::RenderPassConfig(rt0, 1.0f, 1.0f, 1.0f)); //
        rendering->applyShaderConstants(&light);
        rendering->drawGeometry(nullptr, scene->getPositions(), 12, scene->getPositions()->getCount(), foundation::RenderTopology::TRIANGLESTRIP);
        rendering->endPass();

        
        
        scene->updateAndDraw(dtSec);
        //primitives->drawSphere({}, 1.0f);
        primitives->drawTexturedRectangle(0, 0, 512, 512, rt0);
        
        

        rendering->presentFrame();
    });

    return 0;
}

