
#pragma once

namespace layouts {
    namespace {
        const foundation::InputLayout VTXMVOX = foundation::InputLayout {
            .repeat = 12,
            .attributes = {
                {"position_color_mask", foundation::InputAttributeFormat::SHORT4},
            }
        };
        const foundation::InputLayout VTXNRMUV = foundation::InputLayout {
            .attributes = {
                {"position", foundation::InputAttributeFormat::FLOAT3},
                {"normal", foundation::InputAttributeFormat::FLOAT3},
                {"uv", foundation::InputAttributeFormat::FLOAT2},
            }
        };
    }

    struct ParticlesParams {
        enum class ParticlesOrientation {
            CAMERA, AXIS, WORLD
        };
        bool looped = false;
        bool additiveBlend = false;
        float bakingTimeSec = 0.0f;
        ParticlesOrientation orientation = ParticlesOrientation::CAMERA;
        math::vector3f minXYZ = {0, 0, 0};
        math::vector3f maxXYZ = {0, 0, 0};
        math::vector2f minMaxWidth = {1, 1};
        math::vector2f minMaxHeight = {1, 1};
    };
}
