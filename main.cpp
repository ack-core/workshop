
#include <chrono>

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/gears/math.h"
#include "foundation/gears/camera.h"
#include "foundation/gears/orbit_camera_controller.h"

#include "voxel/mesh_factory.h"

static const char *imageShaderSource = R"(
    const {
        position_size : float4
        texcoord : float4
    }
    inout {
        texcoord : float2
    }
    vssrc {
        float2 screenTransform = float2(2.0, -2.0) / frame_rtBounds.xy;
        float2 vertexCoord = float2(vertex_ID & 0x1, vertex_ID >> 0x1);
        output_texcoord = mix(const_texcoord.xy, const_texcoord.zw, vertexCoord);
        vertexCoord = vertexCoord * const_position_size.zw + const_position_size.xy; //
        output_position = float4(vertexCoord.xy * screenTransform + float2(-1.0, 1.0), 0.5, 1);
    }
    fssrc {
        output_color = _tex2d(0, input_texcoord);
    }
)";

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

        int3 faceIndeces = int3(float3(2.0, 2.0, 2.0) * _step(0.0, relVertexPos.yzy) + _step(0.0, relVertexPos.zxx));
        
        float4 koeff = float4(0.0, 0.0, 0.0, 0.0);
        koeff = koeff + _step(0.5, -faceNormal.x) * instance_lightFaceNX;
        koeff = koeff + _step(0.5,  faceNormal.x) * instance_lightFacePX;
        koeff = koeff + _step(0.5, -faceNormal.y) * instance_lightFaceNY;
        koeff = koeff + _step(0.5,  faceNormal.y) * instance_lightFacePY;
        koeff = koeff + _step(0.5, -faceNormal.z) * instance_lightFaceNZ;
        koeff = koeff + _step(0.5,  faceNormal.z) * instance_lightFacePZ;
        
        float2 texcoord = float2(0.0, 0.0);
        texcoord = texcoord + _step(0.5, _abs(faceNormal.x)) * fixed_faceUV[faceIndeces.x];
        texcoord = texcoord + _step(0.5, _abs(faceNormal.y)) * fixed_faceUV[faceIndeces.y];
        texcoord = texcoord + _step(0.5, _abs(faceNormal.z)) * fixed_faceUV[faceIndeces.z];

        output_position = _transform(_transform(absVertexPos, const_modelTransform), frame_viewProjMatrix);
        output_texcoord = texcoord;
        output_koeff = koeff;
    }
    fssrc {
        float m0 = _lerp(input_koeff[0], input_koeff[1], input_texcoord.x);
        float m1 = _lerp(input_koeff[2], input_koeff[3], input_texcoord.x);
        float k = _pow(_smooth(0, 1, _lerp(m0, m1, input_texcoord.y)), 0.5);
        output_color = float4(k, k, k, 1.0);
    }
)";

std::uint32_t textureData[] = {
    0xff0000ff, 0xff0000ff, 0xffffffff, 0xffffffff,
    0xff0000ff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffff0000,
    0xffffffff, 0xffffffff, 0xffff0000, 0xffff0000,
};

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
    auto imageShader = rendering->createShader("uiimage", imageShaderSource, {
        {"ID", foundation::RenderingShaderInputFormat::ID}
    });
    auto imageTexture = rendering->createTexture(foundation::RenderingTextureFormat::RGBA8UN, 4, 4, {&textureData});
    
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

    auto voxconst = math::transform3f::identity(); //.translated({0, 0, -3}).rotated({0, 1, 0}, math::PI_6);
    auto voxdata = rendering->createData(voxelMesh.frames[0].voxels.get(), voxelMesh.frames[0].voxelCount, sizeof(voxel::Voxel));
    
    struct ImgConst {
        math::vector2f position;
        math::vector2f size;
        math::vector2f texcoord0;
        math::vector2f texcoord1;
    }
    imgconst = {{0, 0}, {100, 50}, {0, 0}, {1, 1}};
    
    platform->run([&](float dtSec) {
        rendering->updateFrameConstants(camera->getPosition().flat3, camera->getForwardDirection().flat3, camera->getVPMatrix().flat16);
        rendering->beginPass("axis", axisShader, foundation::RenderingPassConfig(0.8f, 0.775f, 0.75f));
        rendering->drawGeometry(nullptr, 10, foundation::RenderingTopology::LINES);
        rendering->endPass();
        
        rendering->beginPass("vox", meshShader);
        rendering->applyShaderConstants(&voxconst);
        rendering->drawGeometry(nullptr, voxdata, 12, voxelMesh.frames[0].voxelCount, foundation::RenderingTopology::TRIANGLESTRIP);
        rendering->endPass();

        rendering->beginPass("img", imageShader);
        rendering->applyShaderConstants(&imgconst);
        rendering->applyTextures({imageTexture});
        rendering->drawGeometry(nullptr, 4, foundation::RenderingTopology::TRIANGLESTRIP);
        rendering->endPass();


        rendering->presentFrame();
    });

    return 0;
}

