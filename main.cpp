
#include <chrono>

#include "foundation/platform/interfaces.h"
#include "foundation/rendering/interfaces.h"

#include "game_castle/castle_controller/interfaces.h"
#include "game_castle/creature_controller/interfaces.h"
#include "game_castle/projectiles/interfaces.h"

#include "foundation/orbit_camera_controller.h"

#include "thirdparty/upng/upng.h"

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

const char *g_staticMeshShader = R"(
        const {
            modelTransform : matrix4
        }
        inter {
            texcoord : float2
        }
        vssrc {
            float4 center = float4(instance_position.xyz, 1.0);
            float3 camSign = _sign(_transform(modelTransform, float4(_cameraPosition.xyz - _transform(center, modelTransform).xyz, 0)).xyz);
            float4 cube_position = float4(camSign, 0.0) * cube[vertex_ID] + center;
            out_position = _transform(cube_position, _viewProjMatrix * modelTransform);
            inter.texcoord = float2(float(instance_scale_color.w) / 255.0, 0);
        }
        fssrc {
            out_color = _tex2d(0, inter.texcoord);
        }
    )";

const char *g_dynamicMeshShader = R"(
        fixed {
            cube[12] : float4 = 
                [-0.5, 0.5, 0.5, 1.0]
                [-0.5, -0.5, 0.5, 1.0]
                [0.5, 0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0]
                [0.5, 0.5, 0.5, 1.0]
                [0.5, -0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, 0.5, 1.0]
                [-0.5, 0.5, -0.5, 1.0]
                [-0.5, 0.5, 0.5, 1.0]
        }
        const {
            modelTransform : matrix4
        }
        inout {
            texcoord : float2
        }
        vssrc {
            float4 center = float4(instance_position.xyz, 1.0);
            float3 camSign = _sign(_transform(modelTransform, float4(_cameraPosition.xyz - _transform(center, modelTransform).xyz, 0)).xyz);
            float4 cube_position = float4(camSign, 0.0) * cube[vertex_ID] + center; //
            output_position = _transform(cube_position, _transform(_viewProjMatrix, modelTransform));
            output_texcoord = float2(instance_scale_color.w / 255.0, 0);
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

datahub::DataHub<game::CastleDataHub, game::CreatureDataHub> dh;

int main(int argc, const char * argv[]) {
    srand(unsigned(std::chrono::steady_clock::now().time_since_epoch().count()));

    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);

    auto texture = rendering->createTexture(foundation::RenderingTextureFormat::RGBA8UN, 4, 4, { (const std::uint8_t *)tx });
    auto files = platform->formFileList("");
    
    auto camera = std::make_shared<gears::Camera>(platform);
    auto orbitCameraController = std::make_unique<gears::OrbitCameraController>(platform, camera);
    auto primitives = std::make_shared<gears::Primitives>(rendering);

    auto meshes = voxel::MeshFactory::instance(platform, rendering, "palette.png");
    auto environment = voxel::TiledWorld::instance(platform, meshes, primitives);
    
    auto castleController = game::CastleController::instance(platform, environment, primitives, dh);
    auto creatureController = game::CreatureController::instance(platform, meshes, environment, primitives, dh, dh);

    environment->loadSpace("forest");
    castleController->startBattle();
    creatureController->startBattle();

    {
        ((game::CreatureDataHub &)dh).enemies.add([&](game::CreatureDataHub::Enemy &data) {
            data.health = 10.0;
            data.position = environment->getHelperPosition("enemies", 0);
        });
    }

    auto uiShader = rendering->createShader("ui", g_srcUI, {
        {"ID", foundation::RenderingShaderInputFormat::VERTEX_ID},
    });

    struct {
        float p[4] = { 0, 0, 50.0, 50.0 };
        float t[4] = { 0, 0, 1, 1 };
    }
    uidata;

    math::transform3f idtransform = math::transform3f::identity();

    platform->run([&](float dtSec) {
        rendering->updateCameraTransform(camera->getPosition().flat3, camera->getForwardDirection().flat3, camera->getVPMatrix().flat16);
        rendering->prepareFrame();

        primitives->drawAxis();
        //primitives->drawCircleXZ({}, 1.0f, { 1, 0, 0, 1 });

        castleController->updateAndDraw(dtSec);
        creatureController->updateAndDraw(dtSec);
        environment->updateAndDraw(dtSec);

        //rendering->applyTextures({ palette.get() });
        //rendering->applyShader(voxelShader, idtransform.flat16);
        //rendering->drawGeometry(nullptr, mesh->getGeometry(), 0, 12, 0, mesh->getGeometry()->getCount(), foundation::RenderingTopology::TRIANGLESTRIP);

        rendering->applyShader(uiShader, &uidata);
        rendering->applyTextures({ texture.get() });
        rendering->drawGeometry(4, foundation::RenderingTopology::TRIANGLESTRIP);

        rendering->presentFrame(dtSec);
    });

    ((game::CreatureDataHub &)dh).enemies.clear();

    return 0;
}

