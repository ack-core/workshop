
#include "math_tests.h"

namespace test {
    bool shader_bad_args_test(const API& api) {
        // Shader should not be crated if 
        //
        const char* shaderSrc0 = R"()";
        const char* shaderSrc1 = R"(
            vssrc {
                output_position = float4(0, 0, 0, 1); 
            }
            fssrc {
                output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
            }
        )";
        const foundation::InputLayout layout0 = foundation::InputLayout{
            .repeat = 3
        };
        const foundation::InputLayout layout1 = foundation::InputLayout{};
        foundation::RenderShaderPtr shader0 = api.rendering->createShader(shaderSrc0, layout0);
        EXPECT_EQ(shader0, nullptr);
        foundation::RenderShaderPtr shader1 = api.rendering->createShader(shaderSrc1, layout1);
        EXPECT_EQ(shader1, nullptr);
        return true;
    }

    bool shader_single_vertex_attribute_test(const API &api) {
        // Whatever vertex geometry comes as is with white color
        //
        const char* shaderSrc = R"(
            vssrc {
                output_position = vertex_mypos1; 
            }
            fssrc {
                output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
            }
        )";
        const foundation::InputLayout layout = foundation::InputLayout {
            .attributes = {
                {"mypos1", foundation::InputAttributeFormat::FLOAT4},
            }
        };
        foundation::RenderShaderPtr shader = api.rendering->createShader(shaderSrc, layout);
        EXPECT_NOT_EQ(shader, nullptr);
        return true;
    }

    bool shader_several_vertex_attributes_test(const API& api) {
        // Several vertex attributes accessible from shader code
        //
        const char* shaderSrc = R"(
            vssrc {
                output_position = float4(vertex_mypos1.xy + vertex_myoffset.xy, vertex_mypos1.zw); 
            }
            fssrc {
                output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
            }
        )";
        const foundation::InputLayout layout = foundation::InputLayout {
            .attributes = {
                {"mypos1", foundation::InputAttributeFormat::FLOAT4},
                {"myoffset", foundation::InputAttributeFormat::FLOAT2},
            }
        };
        foundation::RenderShaderPtr shader = api.rendering->createShader(shaderSrc, layout);
        EXPECT_NOT_EQ(shader, nullptr);
        return true;
    }

    bool shader_ids_accessibility_test(const API& api) {
        // repear_ID, vertex_ID and instance_ID should be always available in shader code
        //
        const char* shaderSrc = R"(
            vssrc {
                output_position = float4(repeat_ID & 0x1, repeat_ID >> 0x1, 0.1, 1.0); 
                output_position.xy += float2(vertex_ID * 10, instance_ID * 10); 
            }
            fssrc {
                output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
            }
        )";
        const foundation::InputLayout layout = foundation::InputLayout {
            .repeat = 3
        };
        foundation::RenderShaderPtr shader = api.rendering->createShader(shaderSrc, layout);
        EXPECT_NOT_EQ(shader, nullptr);
        return true;
    }
}

namespace test {
    void registerShaderTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(shader_bad_args_test);
        REGISTER_UNIT_TEST(shader_single_vertex_attribute_test);
        REGISTER_UNIT_TEST(shader_several_vertex_attributes_test);
        REGISTER_UNIT_TEST(shader_ids_accessibility_test);
    }
}
