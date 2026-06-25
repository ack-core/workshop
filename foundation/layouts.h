
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
        const foundation::InputLayout VTXUIUV = foundation::InputLayout {
            .repeat = 4,
            .attributes = {
                {"position_size", foundation::InputAttributeFormat::FLOAT4},
                {"uv", foundation::InputAttributeFormat::FLOAT4},
                {"color", foundation::InputAttributeFormat::FLOAT4},
                {"args", foundation::InputAttributeFormat::FLOAT4},
            }
        };
    }
}
