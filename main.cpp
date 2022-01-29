
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

//            [-0.5, -0.5, 0.5][-0.5, 0.5, 0.5][0.5, -0.5, 0.5][0.5, 0.5, 0.5]
//            [0.5, -0.5, 0.5][0.5, 0.5, 0.5][0.5, -0.5, -0.5][0.5, 0.5, -0.5]
//            [0.5, 0.5, -0.5][0.5, 0.5, 0.5][-0.5, 0.5, -0.5][-0.5, 0.5, 0.5]

const char *voxShaderSrc = R"(
    fixed {
        cube[36] : float4 =
            [-0.5, 0.5, 0.5, -1][-0.5, -0.5, 0.5, -1][0.5, 0.5, 0.5, -1][-0.5, -0.5, 0.5, -1][0.5, 0.5, 0.5, -1][0.5, -0.5, 0.5, -1]
            [0.5, -0.5, 0.5, -1][0.5, 0.5, 0.5, -1][0.5, -0.5, -0.5, -1][0.5, 0.5, 0.5, -1][0.5, -0.5, -0.5, -1][0.5, 0.5, -0.5, -1]
            [0.5, 0.5, -0.5, -1][0.5, 0.5, 0.5, -1][-0.5, 0.5, -0.5, -1][0.5, 0.5, 0.5, -1][-0.5, 0.5, -0.5, -1][-0.5, 0.5, 0.5, -1]
            [-0.5, 0.5, 0.5, +1][-0.5, -0.5, 0.5, +1][0.5, 0.5, 0.5, +1][-0.5, -0.5, 0.5, +1][0.5, 0.5, 0.5, +1][0.5, -0.5, 0.5, +1]
            [0.5, -0.5, 0.5, +1][0.5, 0.5, 0.5, +1][0.5, -0.5, -0.5, +1][0.5, 0.5, 0.5, +1][0.5, -0.5, -0.5, +1][0.5, 0.5, -0.5, +1]
            [0.5, 0.5, -0.5, +1][0.5, 0.5, 0.5, +1][-0.5, 0.5, -0.5, +1][0.5, 0.5, 0.5, +1][-0.5, 0.5, -0.5, +1][-0.5, 0.5, 0.5, +1]
    }
    const {
        lightPosition : float3
        lightRadius : float1
    }
    inout {
        dist : float4
    }
    vssrc {
        float3 cubeCenter = float3(instance_position.xyz);
        float3 toLight = const_lightPosition - cubeCenter;
        float3 toLightSign = _sign(toLight);
        
        float4 cubeConst = fixed_cube[vertex_ID];
        float3 relVertexPos = toLightSign * cubeConst.xyz;
        float3 absVertexPos = cubeCenter + relVertexPos;
        
        float3 vToLight = const_lightPosition - absVertexPos;
        float3 vToLightNrm = _norm(vToLight);
        float3 vToLightSign = _sign(vToLight);
        
        float3 octahedron = vToLightNrm / _dot(vToLightNrm, vToLightSign);
        octahedron.xz = cubeConst.w * octahedron.y <= 0.0 ? vToLightSign.xz * (float2(1.0, 1.0) - _abs(octahedron.zx)) : octahedron.xz;
        
        output_dist = float4(1, 1, 1, 1);
        output_dist[int(cubeConst.w * 0.5 + 0.5)] = _lerp(1.0, length(vToLight) / const_lightRadius, _step(0, cubeConst.w * (toLight.y) + 1.0));
        output_position = float4((octahedron.x + octahedron.z), (octahedron.z - octahedron.x), 0.5, 1);
    }
    fssrc {
        output_color = float4(input_dist.x, input_dist.y, 1, 1);
    }
)";

int main(int argc, const char * argv[]) {
    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);
    
    auto camera = std::make_shared<gears::Camera>(platform->getScreenWidth(), platform->getScreenHeight());
    auto primitives = std::make_shared<gears::Primitives>(rendering);
    
    auto meshFactory = voxel::MeshFactory::instance(platform);
    auto scene = voxel::Scene::instance(meshFactory, rendering, "");
    scene->addStaticModel("walls.vox", {-30, -16, -30});
    
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
    voxel::VoxelPosition positions[] = {
        {1, -1, 1, 0},
        {1, 1, 1, 0}
    };
    
    struct Light {
        math::vector3f position;
        float radius;
    }
    light {
        {0.0f, 0.0f, 0.0f}, 60.0f
    };
    
    auto imageTexture = rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, 4, 4, {&textureData});
    auto rt0 = rendering->createRenderTarget(foundation::RenderTextureFormat::RGBA32F, 512, 512, false);
    auto voxData = rendering->createData(&positions, 2, sizeof(voxel::VoxelPosition));
    
    platform->run([&](float dtSec) {
        rendering->updateFrameConstants(camera->getPosition().flat3, camera->getForwardDirection().flat3, camera->getVPMatrix().flat16);
        primitives->drawAxis(foundation::RenderPassConfig(0.8f, 0.775f, 0.75f));

        //rendering->beginPass("vox", voxShader); //
        rendering->beginPass("vox", voxShader, foundation::RenderPassConfig(rt0, foundation::BlendType::MINVALUE, 1.0f, 1.0f, 1.0f)); //
        rendering->applyShaderConstants(&light);
        //rendering->drawGeometry(nullptr, voxData, 24, 2, foundation::RenderTopology::TRIANGLESTRIP);
        rendering->drawGeometry(nullptr, scene->getPositions(), 36, scene->getPositions()->getCount(), foundation::RenderTopology::TRIANGLES);
        rendering->endPass();

        
        
        scene->updateAndDraw(rt0, dtSec);
        //primitives->drawSphere({}, 1.0f);
        primitives->drawTexturedRectangle(16, 16, 512, 512, rt0);
        
        

        rendering->presentFrame();
    });

    return 0;
}

