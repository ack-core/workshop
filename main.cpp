
#include <chrono>

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/gears/math.h"
#include "foundation/gears/camera.h"
#include "foundation/gears/orbit_camera_controller.h"
#include "foundation/gears/primitives.h"

#include "voxel/mesh_factory.h"
#include "voxel/scene.h"

//std::uint32_t textureData[] = {
//    0xff0000ff, 0xff0000ff, 0xffffffff, 0xffffffff,
//    0xff0000ff, 0xffffffff, 0xffffffff, 0xffffffff,
//    0xffffffff, 0xffffffff, 0xffffffff, 0xffff0000,
//    0xffffffff, 0xffffffff, 0xffff0000, 0xffff0000,
//};

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

//const char *voxShaderSrc = R"(
//    fixed {
//        cube[36] : float4 =
//            [-0.5, 0.5, 0.5, -1][-0.5, -0.5, 0.5, -1][0.5, 0.5, 0.5, -1][-0.5, -0.5, 0.5, -1][0.5, 0.5, 0.5, -1][0.5, -0.5, 0.5, -1]
//            [0.5, -0.5, 0.5, -1][0.5, 0.5, 0.5, -1][0.5, -0.5, -0.5, -1][0.5, 0.5, 0.5, -1][0.5, -0.5, -0.5, -1][0.5, 0.5, -0.5, -1]
//            [0.5, 0.5, -0.5, -1][0.5, 0.5, 0.5, -1][-0.5, 0.5, -0.5, -1][0.5, 0.5, 0.5, -1][-0.5, 0.5, -0.5, -1][-0.5, 0.5, 0.5, -1]
//            [-0.5, 0.5, 0.5, +1][-0.5, -0.5, 0.5, +1][0.5, 0.5, 0.5, +1][-0.5, -0.5, 0.5, +1][0.5, 0.5, 0.5, +1][0.5, -0.5, 0.5, +1]
//            [0.5, -0.5, 0.5, +1][0.5, 0.5, 0.5, +1][0.5, -0.5, -0.5, +1][0.5, 0.5, 0.5, +1][0.5, -0.5, -0.5, +1][0.5, 0.5, -0.5, +1]
//            [0.5, 0.5, -0.5, +1][0.5, 0.5, 0.5, +1][-0.5, 0.5, -0.5, +1][0.5, 0.5, 0.5, +1][-0.5, 0.5, -0.5, +1][-0.5, 0.5, 0.5, +1]
//    }
//    const {
//        lightPosition : float3
//        lightRadius : float1
//    }
//    inout {
//        dist : float4
//    }
//    vssrc {
//        float3 cubeCenter = float3(instance_position.xyz);
//        float3 toLight = const_lightPosition - cubeCenter;
//        float3 toLightSign = _sign(toLight);
//
//        float4 cubeConst = fixed_cube[vertex_ID];
//        float3 relVertexPos = toLightSign * cubeConst.xyz;
//        float3 absVertexPos = cubeCenter + relVertexPos;
//
//        float3 vToLight = const_lightPosition - absVertexPos;
//        float3 vToLightNrm = _norm(vToLight);
//        float3 vToLightSign = _sign(vToLight);
//
//        float3 octahedron = vToLightNrm / _dot(vToLightNrm, vToLightSign);
//        octahedron.xz = cubeConst.w * octahedron.y <= 0.0 ? vToLightSign.xz * (float2(1.0, 1.0) - _abs(octahedron.zx)) : octahedron.xz;
//
//        output_dist = float4(1, 1, 1, 1);
//        output_dist[int(cubeConst.w * 0.5 + 0.5)] = _lerp(1.0, length(vToLight) / const_lightRadius, _step(0, cubeConst.w * (toLight.y) + 1.0));
//        output_position = float4((octahedron.x + octahedron.z), (octahedron.z - octahedron.x), 0.5, 1);
//    }
//    fssrc {
//        output_color = float4(input_dist.x, input_dist.y, 1, 1);
//    }
//)";

//static const char *g_billShaderSrc = R"(
//    inout {
//        texcoord : float2
//    }
//    vssrc {
//        float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
//        float4 billPos = float4(vertexCoord.xy * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 0.0);
//        float4 viewPos = _transform(float4(20, 0, 0, 1), frame_viewMatrix) + billPos * 20.0;
//        output_position = _transform(viewPos, frame_projMatrix);
//        output_texcoord = vertexCoord;
//    }
//    fssrc {
//        output_color = float4(fragment_coord, 0, 1);
//    }
//)";

static const int VMAX = 5;
static const float SPREAD0 = 0.012f;
static const float SPREAD1 = 0.028f;

math::vector3f vs[VMAX] = {
    {0.0, 1.0, 0.0},

    {+1.0000f * SPREAD0, 1.0, +0.0000f * SPREAD0},
    {+0.0000f * SPREAD0, 1.0, +1.0000f * SPREAD0},
    {-1.0000f * SPREAD0, 1.0, -0.0000f * SPREAD0},
    {-0.0000f * SPREAD0, 1.0, -1.0000f * SPREAD0},

//    {+0.7071f * SPREAD0, 1.0, +0.7071f * SPREAD0},
//    {+0.7071f * SPREAD0, 1.0, -0.7071f * SPREAD0},
//    {-0.7071f * SPREAD0, 1.0, +0.7071f * SPREAD0},
//    {-0.7071f * SPREAD0, 1.0, -0.7071f * SPREAD0},
//
//    {+1.0000f * SPREAD1, 1.0, +0.0000f * SPREAD1},
//    {+0.0000f * SPREAD1, 1.0, +1.0000f * SPREAD1},
//    {-1.0000f * SPREAD1, 1.0, -0.0000f * SPREAD1},
//    {-0.0000f * SPREAD1, 1.0, -1.0000f * SPREAD1},
//
//    {+0.7071f * SPREAD1, 1.0, +0.7071f * SPREAD1},
//    {+0.7071f * SPREAD1, 1.0, -0.7071f * SPREAD1},
//    {-0.7071f * SPREAD1, 1.0, +0.7071f * SPREAD1},
//    {-0.7071f * SPREAD1, 1.0, -0.7071f * SPREAD1},

};

int main(int argc, const char * argv[]) {
//    for (int i = 9; i < VMAX; i++) {
//        vs[i] = vs[i].rotated({0, 1, 0}, M_PI / 8.0);
//    }
//    srand(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
//    float cosAngle = ::cos(M_PI / 32.0f);
//    printf("cos angle = %lf\n", cosAngle);
//
//    for (int i = 0; i < VMAX; i++) {
//        float rnd0 = float(rand() % 1000) / 1000.0f;
//        float rnd1 = float(rand() % 1000) / 1000.0f;
//        float y = rnd0 * (1.0f - cosAngle) + cosAngle;
//        float phi = rnd1 * 2.0f * M_PI;
//
//        float x = sqrt(1.0f - y * y) * cos(phi);
//        float z = sqrt(1.0f - y * y) * sin(phi);
//
//        vs[i] = math::vector3f(x, y, z);
//
//        math::vector3f v = vs[i]; //.normalized();
//        printf("[%2.8f, %2.8f, %2.8f]\n", v.x, v.y, v.z);
//    }
    
    
    math::vector2f a = math::vector2f{1.0, 1.0}.normalized();
    
    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);
    
    auto camera = std::make_shared<gears::Camera>(platform->getScreenWidth(), platform->getScreenHeight());
    auto primitives = std::make_shared<gears::Primitives>(rendering);
    
    auto meshFactory = voxel::MeshFactory::instance(platform);
    auto scene = voxel::Scene::instance(meshFactory, platform, rendering, "palette.png");

    scene->addStaticModel("walls.vox", {-30, -16, -36});
    scene->addLightSource({10.0, 0, -5}, 40.0f, math::color(1.0, 1.0, 1.0, 1.0));
    
    gears::OrbitCameraController cameraController (platform, camera);
    cameraController.setEnabled(true);
        
    platform->run([&](float dtSec) {
        rendering->updateFrameConstants(
            camera->getVPMatrix().flat16,
            camera->getInvViewProjMatrix().flat16,
            camera->getViewMatrix().flat16,
            camera->getProjMatrix().flat16,
            camera->getPosition().flat3,
            camera->getForwardDirection().flat3
        );
        //primitives->drawAxis(foundation::RenderPassConfig(0.8f, 0.775f, 0.75f));
        
//        rendering->beginPass("bill", billShader);
//        rendering->drawGeometry(nullptr, 4, foundation::RenderTopology::TRIANGLESTRIP);
//        rendering->endPass();
        
        //rendering->beginPass("vox", voxShader); //
        //rendering->beginPass("vox", voxShader, foundation::RenderPassConfig(rt0, foundation::BlendType::MINVALUE, 1.0f, 1.0f, 1.0f)); //
        //rendering->applyShaderConstants(&light);
        //rendering->drawGeometry(nullptr, voxData, 24, 2, foundation::RenderTopology::TRIANGLESTRIP);
        //rendering->drawGeometry(nullptr, scene->getPositions(), 36, scene->getPositions()->getCount(), foundation::RenderTopology::TRIANGLES);
        //rendering->endPass();
        
        
        
        scene->updateAndDraw(dtSec);
        //primitives->drawSphere({}, 1.0f);
        //primitives->drawTexturedRectangle(16, 16, 128, 128, rt0);
        
//        math::vector3f north = math::vector3f{0.0f, 1.0f, 0.0f}.normalized();
//        math::vector3f direction = math::vector3f{1.0f, -1.0f, 1.0f}.normalized();
//        math::vector3f axis = math::vector3f{0.0f, 1.0f, 0.0f}.cross(direction).normalized();
//        float angle = ::acos(direction.dot({0.0f, 1.0f, 0.0f}));
////        math::transform3f rotation = math::transform3f(axis, -angle);
//
//
//        for (int i = 0; i < VMAX; i++) {
//            math::vector3f v = vs[i];
//            //math::vector3f v1 = v * ::cos(angle) + axis.cross(v) * ::sin(angle) + axis * axis.dot(v) * (1.0f - ::cos(angle));
//
//            primitives->drawLine({0, 0, 0}, v, {1.0, 0, 0, 1.0});
//        }
        

        rendering->presentFrame();
    });

    return 0;
}

