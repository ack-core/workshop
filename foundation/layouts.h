
#pragma once

namespace {
    namespace layouts {
        const foundation::InputLayout VTXSVOX = foundation::InputLayout {
            .repeat = 12,
            .attributes = {
                {"position_color_mask", foundation::InputAttributeFormat::SHORT4},
                {"scale_reserved", foundation::InputAttributeFormat::BYTE4},
            }
        };
        const foundation::InputLayout VTXDVOX = foundation::InputLayout {
            .repeat = 12,
            .attributes = {
                {"position", foundation::InputAttributeFormat::FLOAT3},
                {"color_mask", foundation::InputAttributeFormat::BYTE4},
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
}
