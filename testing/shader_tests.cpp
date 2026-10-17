
#include "math_tests.h"

namespace test {
    bool shader_bad_args_test(const API& api) {
        // Shader should not be crated if attributes are empty and repeat <= 1
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

    bool shader_variable_types_test(const API& api) {
        // float1-4, int1-4, uint1-4 types and basic operations should be available in vertex and fragment code
        //
        const char* shaderSrc = R"(
            vssrc {
                float1 f1 = 0.5;
                float2 f2 = float2(f1, 0.25) * 2.0;
                float3 f3 = float3(f2, f1) + float3(0.1, 0.2, 0.3);
                float4 f4 = float4(f3.xy - f2, f3.z / 2.0, 1.0);
                int1 i1 = repeat_ID + 1;
                int2 i2 = int2(i1, -i1) * 2;
                int3 i3 = int3(i2, i1 % 2) - int3(1, 1, 1);
                int4 i4 = int4(i3, 1) >> 1;
                uint1 u1 = uint1(repeat_ID) & uint1(0x1);
                uint2 u2 = uint2(u1, u1 << uint1(1));
                uint3 u3 = uint3(u2, u1 | uint1(0x2));
                uint4 u4 = uint4(u3, u3.x ^ u3.y);
                output_position = f4 + float4(float(i4.x), float(i4.y), float(u4.x), float(u4.w)) * 0.1;
            }
            fssrc {
                float1 f1 = 0.5;
                float2 f2 = float2(f1, f1 * 0.5);
                float3 f3 = float3(f2 + float2(0.1, 0.1), f1 - 0.25);
                float4 f4 = float4(f3, 1.0);
                int1 i1 = 3;
                int2 i2 = int2(i1, i1 / 2);
                int3 i3 = int3(i2 + int2(1, 1), i1 * 2);
                int4 i4 = int4(i3, i1 - 1);
                uint1 u1 = uint1(5);
                uint2 u2 = uint2(u1, u1 >> uint1(1));
                uint3 u3 = uint3(u2 & uint2(0x3, 0x3), u1 + uint1(1));
                uint4 u4 = uint4(u3, u3.z - u3.x);
                output_color[0] = f4 * float4(float(i4.x), float(i4.w), float(u4.y), float(u4.w)) * 0.1;
            }
        )";
        const foundation::InputLayout layout = foundation::InputLayout {
            .repeat = 4
        };
        foundation::RenderShaderPtr shader = api.rendering->createShader(shaderSrc, layout);
        EXPECT_NOT_EQ(shader, nullptr);
        return true;
    }

    bool shader_fixed_block_test(const API& api) {
        // 'fixed' block with different configurations should be accessible from shader code
        //
        const char* shaderSrc0 = R"(
            fixed {
                position : float4 = [0.0, 0.0, 0.5, 1.0]
            }
            vssrc {
                output_position = fixed_position;
            }
            fssrc {
                output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
            }
        )";
        const char* shaderSrc1 = R"(
            fixed {
                quad[4] : float4 = [-0.5, 0.5, 0, 0][0.5, 0.5, 1, 0][-0.5, -0.5, 0, 1][0.5, -0.5, 1, 1]
            }
            vssrc {
                output_position = float4(fixed_quad[repeat_ID].xy, 0.5, 1.0);
            }
            fssrc {
                output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
            }
        )";
        const char* shaderSrc2 = R"(
            fixed {
                lines[4] : float3 =
                    [0, 0, 0][1, 0, 0]
                    [0, 1, 1][1, 1, 1]
                scale : float1 = [0.5]
                offset[2] : float2 = [0.1, 0.1][-0.1, -0.1]
                index[4] : int1 = [0][1][0][1]
                mask : uint2 = [1, 0]
            }
            vssrc {
                float3 position = fixed_lines[repeat_ID] * fixed_scale;
                position.xy += fixed_offset[fixed_index[repeat_ID]] * float(fixed_mask.x);
                output_position = float4(position, 1.0);
            }
            fssrc {
                output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
            }
        )";
        const foundation::InputLayout layout = foundation::InputLayout {
            .repeat = 4
        };
        foundation::RenderShaderPtr shader0 = api.rendering->createShader(shaderSrc0, layout);
        EXPECT_NOT_EQ(shader0, nullptr);
        foundation::RenderShaderPtr shader1 = api.rendering->createShader(shaderSrc1, layout);
        EXPECT_NOT_EQ(shader1, nullptr);
        foundation::RenderShaderPtr shader2 = api.rendering->createShader(shaderSrc2, layout);
        EXPECT_NOT_EQ(shader2, nullptr);
        return true;
    }

    bool shader_const_block_test(const API& api) {
        // 'const' block allows only float4, int4, uint4, matrix4 types (and arrays of them)
        //
        const char* shaderSrc0 = R"(
            const {
                position : float4
            }
            vssrc {
                output_position = const_position;
            }
            fssrc {
                output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
            }
        )";
        const char* shaderSrc1 = R"(
            const {
                transform : matrix4
                color : float4
                positions[4] : float4
                index : int4
                mask : uint4
            }
            vssrc {
                float4 position = const_positions[(repeat_ID + const_index.x) & 0x3] * float(const_mask.x);
                output_position = _transform(position, const_transform);
            }
            fssrc {
                output_color[0] = const_color;
            }
        )";
        const char* badShaderSrc0 = R"(
            const {
                position : float3
            }
            vssrc {
                output_position = float4(const_position, 1.0);
            }
            fssrc {
                output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
            }
        )";
        const foundation::InputLayout layout = foundation::InputLayout {
            .repeat = 4
        };
        foundation::RenderShaderPtr shader0 = api.rendering->createShader(shaderSrc0, layout);
        EXPECT_NOT_EQ(shader0, nullptr);
        foundation::RenderShaderPtr shader1 = api.rendering->createShader(shaderSrc1, layout);
        EXPECT_NOT_EQ(shader1, nullptr);

        foundation::RenderShaderPtr badShader0 = api.rendering->createShader(badShaderSrc0, layout);
        EXPECT_EQ(badShader0, nullptr);
        return true;
    }

    bool shader_inout_block_test(const API& api) {
        // 'inout' block passes values from vertex to fragment shader
        //
        const char* shaderSrc0 = R"(
            inout {
                color : float4
            }
            vssrc {
                output_position = float4(repeat_ID & 0x1, repeat_ID >> 0x1, 0.1, 1.0);
                output_color = float4(1.0, 1.0, 1.0, 1.0);
            }
            fssrc {
                output_color[0] = input_color;
            }
        )";
        const char* shaderSrc1 = R"(
            inout {
                uv : float2
                normal : float3
                alpha : float1
                color : float4
            }
            vssrc {
                output_position = float4(repeat_ID & 0x1, repeat_ID >> 0x1, 0.1, 1.0);
                output_uv = output_position.xy;
                output_normal = float3(0.0, 0.0, 1.0);
                output_alpha = 0.5;
                output_color = float4(1.0, 1.0, 1.0, 1.0);
            }
            fssrc {
                output_color[0] = float4(input_color.xyz * input_normal * float3(input_uv, 1.0), input_alpha);
            }
        )";
        const char* shaderSrc2 = R"(
            inout {
                index : int1
                color : float4
                mask : uint4
            }
            vssrc {
                output_position = float4(repeat_ID & 0x1, repeat_ID >> 0x1, 0.1, 1.0);
                output_index = repeat_ID;
                output_color = float4(1.0, 1.0, 1.0, 1.0);
                output_mask = uint4(1, 0, 1, 0);
            }
            fssrc {
                output_color[0] = input_color * float4(float(input_index), float(input_mask.x), 1.0, 1.0);
            }
        )";
        const char* badShaderSrc0 = R"(
            inout {
                color : float5
            }
            vssrc {
                output_position = float4(repeat_ID & 0x1, repeat_ID >> 0x1, 0.1, 1.0);
            }
            fssrc {
                output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
            }
        )";
        const foundation::InputLayout layout = foundation::InputLayout {
            .repeat = 4
        };
        foundation::RenderShaderPtr shader0 = api.rendering->createShader(shaderSrc0, layout);
        EXPECT_NOT_EQ(shader0, nullptr);
        foundation::RenderShaderPtr shader1 = api.rendering->createShader(shaderSrc1, layout);
        EXPECT_NOT_EQ(shader1, nullptr);
        foundation::RenderShaderPtr shader2 = api.rendering->createShader(shaderSrc2, layout);
        EXPECT_NOT_EQ(shader2, nullptr);

        foundation::RenderShaderPtr badShader0 = api.rendering->createShader(badShaderSrc0, layout);
        EXPECT_EQ(badShader0, nullptr);
        return true;
    }
}

namespace test {
    void registerShaderTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(shader_bad_args_test);
        REGISTER_UNIT_TEST(shader_single_vertex_attribute_test);
        REGISTER_UNIT_TEST(shader_several_vertex_attributes_test);
        REGISTER_UNIT_TEST(shader_ids_accessibility_test);
        REGISTER_UNIT_TEST(shader_variable_types_test);
        REGISTER_UNIT_TEST(shader_fixed_block_test);
        REGISTER_UNIT_TEST(shader_const_block_test);
        REGISTER_UNIT_TEST(shader_inout_block_test);
    }
}
