
#include <chrono>

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/gears/math.h"
#include "foundation/gears/camera.h"
#include "foundation/gears/orbit_camera_controller.h"

#include "voxel/mesh_factory.h"

static const char *axisShaderSrc = R"(
    fixed {
        coords[10] : float4 =
            [0.0, 0.0, 0.0, 1.0][1000.0, 0.0, 0.0, 1.0]
            [0.0, 0.0, 0.0, 1.0][0.0, 1000.0, 0.0, 1.0]
            [0.0, 0.0, 0.0, 1.0][0.0, 0.0, 1000.0, 1.0]
            [1.0, 0.0, 0.0, 1.0][1.0, 0.0, 1.0, 1.0]
            [0.0, 0.0, 1.0, 1.0][1.0, 0.0, 1.0, 1.0]
        colors[5] : float4 =
            [1.0, 0.0, 0.0, 1.0]
            [0.0, 1.0, 0.0, 1.0]
            [0.0, 0.0, 1.0, 1.0]
            [0.5, 0.5, 0.5, 1.0]
            [0.5, 0.5, 0.5, 1.0]
    }
    inout {
        color : float4
    }
    vssrc {
        output_position = _transform(fixed_coords[vertex_ID], frame_viewProjMatrix);
        output_color = fixed_colors[vertex_ID >> 1];
    }
    fssrc {
        output_color = input_color;
    }
)";

// XFace Indeces : [-y-z, -y+z, +y-z, +y+z]
// YFace Indeces : [-z-x, -z+x, +z-x, +z+x]
// ZFace Indeces : [-y-x, -y+x, +y-x, +y+x]
const char *meshShaderSrc = R"(
    fixed {
        axis[3] : float4 =
            [0.0, 0.0, 1.0, 0.0]
            [1.0, 0.0, 0.0, 0.0]
            [0.0, 1.0, 0.0, 0.0]
            
        cube[12] : float4 =
            [-0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
            [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0][0.5, 0.5, -0.5, 1.0]
            [0.5, 0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0]

        faceUV[4] : float2 = [0.0, 0.0][1.0, 0.0][0.0, 1.0][1.0, 1.0]
    }
    const {
        modelTransform : matrix4
    }
    inout {
        texcoord : float2
        koeff : float4
    }
    vssrc {
        float3 cubeCenter = float3(instance_position_color.xyz);
        float3 toCamera = frame_cameraPosition.xyz - _transform(float4(cubeCenter, 1.0), const_modelTransform).xyz;
        float3 toCamSign = _sign(_transform(const_modelTransform, float4(toCamera, 0.0)).xyz);
        float4 relVertexPos = float4(toCamSign, 1.0) * fixed_cube[vertex_ID];
        float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
        float3 faceNormal = toCamSign * fixed_axis[vertex_ID >> 2].xyz;

        int3 faceIndeces = int3(float3(2.0, 2.0, 2.0) * step(0.0, relVertexPos.yzy) + step(0.0, relVertexPos.zxx));
        
        //int xFaceIndex = int(2.0 * step(0.0, relVertexPos.y) + step(0.0, relVertexPos.z));
        //int yFaceIndex = int(2.0 * step(0.0, relVertexPos.z) + step(0.0, relVertexPos.x));
        //int zFaceIndex = int(2.0 * step(0.0, relVertexPos.y) + step(0.0, relVertexPos.x));
        
        float4 koeff = float4(0.0, 0.0, 0.0, 0.0);
        koeff = koeff + step(0.5, -faceNormal.x) * instance_lightFaceNX;
        koeff = koeff + step(0.5,  faceNormal.x) * instance_lightFacePX;
        koeff = koeff + step(0.5, -faceNormal.y) * instance_lightFaceNY;
        koeff = koeff + step(0.5,  faceNormal.y) * instance_lightFacePY;
        koeff = koeff + step(0.5, -faceNormal.z) * instance_lightFaceNZ;
        koeff = koeff + step(0.5,  faceNormal.z) * instance_lightFacePZ;
        
        float2 texcoord = float2(0.0, 0.0);
        texcoord = texcoord + step(0.5, abs(faceNormal.x)) * fixed_faceUV[faceIndeces.x];
        texcoord = texcoord + step(0.5, abs(faceNormal.y)) * fixed_faceUV[faceIndeces.y];
        texcoord = texcoord + step(0.5, abs(faceNormal.z)) * fixed_faceUV[faceIndeces.z];

        output_position = _transform(_transform(absVertexPos, const_modelTransform), frame_viewProjMatrix);
        output_texcoord = texcoord;
        output_koeff = koeff;
    }
    fssrc {
        float m0 = mix(input_koeff[0], input_koeff[1], input_texcoord.x);
        float m1 = mix(input_koeff[2], input_koeff[3], input_texcoord.x);
        float k = pow(mix(m0, m1, input_texcoord.y), 0.5);
        output_color = float4(k, k, k, 1.0);
    }
)";

//struct Voxel {
//    std::int16_t positionX, positionY, positionZ, colorIndex;
//    std::uint8_t lightFaceNX[4];
//    std::uint8_t lightFacePX[4];
//    std::uint8_t lightFaceNY[4];
//    std::uint8_t lightFacePY[4];
//    std::uint8_t lightFaceNZ[4];
//    std::uint8_t lightFacePZ[4];
//}
//voxels[] = {
//    {0,1,0,0,    {0,80,180,255}, {255,255,255,255}, {255,255,255,255}, {0,80,180,255}, {255,255,255,255}, {0,80,180,255}},
//    {0,-1,0,0,   {255,255,255,255}, {0,80,180,255}, {255,255,255,255}, {255,255,255,255}, {255,255,255,255}, {255,255,255,255}},
//};

int main(int argc, const char * argv[]) {
    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);
    auto camera = std::make_shared<gears::Camera>(platform);
    auto meshFactory = voxel::MeshFactory::instance(platform);
    
    gears::OrbitCameraController cameraController (platform, camera);
    cameraController.setEnabled(true);

    auto axisShader = rendering->createShader("primitives_axis", axisShaderSrc, {
        {"ID", foundation::RenderingShaderInputFormat::ID}
    });
    
    voxel::Mesh voxelMesh;
    bool loadResult = meshFactory->createMesh("8x8x8.vox", {-8, -4, -8}, voxelMesh);
    
    auto meshShader = rendering->createShader("dynamic_voxel_mesh", meshShaderSrc,
        { // vertex
            {"ID", foundation::RenderingShaderInputFormat::ID}
        },
        { // instance
            {"position_color", foundation::RenderingShaderInputFormat::SHORT4},
            {"lightFaceNX", foundation::RenderingShaderInputFormat::BYTE4_NRM},
            {"lightFacePX", foundation::RenderingShaderInputFormat::BYTE4_NRM},
            {"lightFaceNY", foundation::RenderingShaderInputFormat::BYTE4_NRM},
            {"lightFacePY", foundation::RenderingShaderInputFormat::BYTE4_NRM},
            {"lightFaceNZ", foundation::RenderingShaderInputFormat::BYTE4_NRM},
            {"lightFacePZ", foundation::RenderingShaderInputFormat::BYTE4_NRM},
        }
    );
//
    auto voxconst = math::transform3f::identity(); //.translated({0, 0, -3}).rotated({0, 1, 0}, math::PI_6);
    auto voxdata = rendering->createData(voxelMesh.frames[0].voxels.get(), voxelMesh.frames[0].voxelCount, sizeof(voxel::Voxel));
    
    platform->run([&](float dtSec) {
        rendering->updateFrameConstants(camera->getPosition().flat3, camera->getForwardDirection().flat3, camera->getVPMatrix().flat16);
        rendering->beginPass("axis", axisShader, foundation::RenderingPassConfig(0.8f, 0.775f, 0.75f));
        rendering->drawGeometry(nullptr, 10, foundation::RenderingTopology::LINES);
        rendering->endPass();
        
        rendering->beginPass("vox", meshShader);
        rendering->applyShaderConstants(&voxconst);
        rendering->drawGeometry(nullptr, voxdata, 12, voxelMesh.frames[0].voxelCount, foundation::RenderingTopology::TRIANGLESTRIP);
        rendering->endPass();
        
        rendering->presentFrame();
    });

    return 0;
}

