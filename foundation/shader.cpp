
#include "shader.h"
#include "util.h"
#include "rendering.h"

namespace {
    const std::string indent = "    ";
    const char *SEPARATORS = " ;,=-+*/{(\n\t\r";

    const std::size_t TYPES_COUNT = 14;
    const std::size_t TYPES_PASS_COUNT = 4;

#ifdef PLATFORM_IOS
    const shaderUtils::ShaderTypeInfo TYPE_SIZE_TABLE[TYPES_COUNT] = {
        // Passing from App side
        {"float4",  "float4",   16},
        {"int4",    "int4",     16},
        {"uint4",   "uint4",    16},
        {"matrix4", "float4x4", 64},
        // Internal shader types
        {"float1",  "float",    4},
        {"float2",  "float2",   8},
        {"float3",  "float3",   12},
        {"int1",    "int",      4},
        {"int2",    "int2",     8},
        {"int3",    "int3",     12},
        {"uint1",   "uint",     4},
        {"uint2",   "uint2",    8},
        {"uint3",   "uint3",    12},
        {"matrix3", "float3x3", 36},
    };
#endif
#ifdef PLATFORM_WASM
    const shaderUtils::ShaderTypeInfo TYPE_SIZE_TABLE[TYPES_COUNT] = {
        // Passing from App side
        {"float4",  "vec4",   16},
        {"int4",    "ivec4",  16},
        {"uint4",   "uvec4",  16},
        {"matrix4", "mat4",   64},
        // Internal shader types
        {"float1",  "float",  4},
        {"float2",  "vec2",   8},
        {"float3",  "vec3",   12},
        {"int1",    "int",    4},
        {"int2",    "ivec2",  8},
        {"int3",    "ivec3",  12},
        {"uint1",   "uint",   4},
        {"uint2",   "uvec2",  8},
        {"uint3",   "uvec3",  12},
        {"matrix3", "mat3",   36},
    };
#endif
}

namespace foundation {
    struct NativeVertexFormat {
        const char *shaderTypeName;
        const char *nativeTypeName;
        std::uint32_t size;
        std::uint32_t components;
    };

#ifdef PLATFORM_IOS
    const NativeVertexFormat VTX_CONVERSION_TABLE[] = { // index is InputAttributeFormat value
        {"float2",  "packed_half2",          4, 2},
        {"float4",  "packed_half4",          8, 4},
        {"float",   "packed_float",          4, 1},
        {"float2",  "packed_float2",         8, 2},
        {"float3",  "packed_float3",        12, 3},
        {"float4",  "packed_float4",        16, 4},
        {"short2",  "packed_short2",         4, 2},
        {"short4",  "packed_short4",         8, 4},
        {"ushort2", "packed_ushort2",        4, 2},
        {"ushort4", "packed_ushort4",        8, 4},
        {"float2",  "rg16snorm<float2>",     4, 2},
        {"float4",  "rgba16snorm<float4>",   8, 4},
        {"float2",  "rg16unorm<float2>",     4, 2},
        {"float4",  "rgba16unorm<float4>",   8, 4},
        {"uchar4",  "packed_uchar4",         4, 4},
        {"float4",  "rgba8unorm<float4>",    4, 4},
        {"int",     "packed_int",            4, 1},
        {"int2",    "packed_int2",           8, 2},
        {"int3",    "packed_int3",          12, 3},
        {"int4",    "packed_int4",          16, 4},
        {"uint",    "packed_uint",           4, 1},
        {"uint2",   "packed_uint2",          8, 2},
        {"uint3",   "packed_uint3",         12, 3},
        {"uint4",   "packed_uint4",         16, 4}
    };
#endif
#ifdef PLATFORM_WASM
    const NativeVertexFormat VTX_CONVERSION_TABLE[] = { // index is InputAttributeFormat value
        {"float2",  "vec2",  4,  2},
        {"float4",  "vec4",  8,  4},
        {"float",   "float", 4,  1},
        {"float2",  "vec2",  8,  2},
        {"float3",  "vec3",  12, 3},
        {"float4",  "vec4",  16, 4},
        {"short2",  "ivec2", 4,  2},
        {"short4",  "ivec4", 8,  4},
        {"ushort2", "uvec2", 4,  2},
        {"ushort4", "uvec4", 8,  4},
        {"float2",  "vec2",  4,  2},
        {"float4",  "vec4",  8,  4},
        {"float2",  "vec2",  4,  2},
        {"float4",  "vec4",  8,  4},
        {"uchar4",  "uvec4", 4,  4},
        {"float4",  "vec4",  4,  4},
        {"int",     "int",   4,  1},
        {"int2",    "ivec2", 8,  2},
        {"int3",    "ivec3", 12, 3},
        {"int4",    "ivec4", 16, 4},
        {"uint",    "uint",  4,  1},
        {"uint2",   "uvec2", 8,  2},
        {"uint3",   "uvec3", 12, 3},
        {"uint4",   "uvec4", 16, 4},
    };
#endif
}

namespace foundation {
#ifdef PLATFORM_IOS
    static const std::uint32_t VS_INPUT_VERTEX_COUNT = 3;

    std::string transformCode(const std::string &src) {
        std::string result = src;
        shaderUtils::replace(result, "output_", "output.", SEPARATORS);
        shaderUtils::replace(result, "const_", "constants.", SEPARATORS);
        shaderUtils::replace(result, "frame_", "framedata.", SEPARATORS);
        shaderUtils::replace(result, "input_", "input.", SEPARATORS);
        return result;
    }
    std::uint32_t formVarsBlock(util::strstream &stream, std::string &output, std::size_t allowedTypeCount) {
        std::string varname, arg;
        std::uint32_t totalLength = 0;
        
        while (stream >> varname && varname[0] != '}') {
            if (stream >> util::sequence(":") >> arg) {
                std::size_t elementSize = 0, elementCount = shaderUtils::getArrayMultiply(varname);
                std::string nativeTypeName;
                
                if (shaderGetTypeSize(arg, TYPE_SIZE_TABLE, allowedTypeCount, nativeTypeName, elementSize)) {
                    output += indent + nativeTypeName + " " + varname + ";\n";
                    totalLength += elementSize * elementCount;
                    continue;
                }
            }
            
            return std::uint32_t(0);
        }
        
        return totalLength;
    }
    void formEmptyFixedBlock(std::string &output) {
        output = "\n";
    }
    void formEmptyConstBlock(std::string &output) {
        output = "struct _Constants {\n};\n\n";
    }
    void formEmptyInoutBlock(std::string &output) {
        output = "struct _InOut {\n    float4 position [[position]];\n};\n\n";
    }
    bool formFixedBlock(util::strstream &stream, std::string &output) {
        std::string varname, arg;
        
        while (stream >> varname && varname[0] != '}') {
            if (stream >> util::sequence(":") >> arg >> util::sequence("=")) {
                std::size_t elementSize = 0, elementCount = shaderUtils::getArrayMultiply(varname);
                std::string nativeTypeName;
                
                if (shaderUtils::shaderGetTypeSize(arg, TYPE_SIZE_TABLE, TYPES_COUNT, nativeTypeName, elementSize)) {
                    output += "constant " + nativeTypeName + " fixed_" + varname + " = {\n";
                    for (std::size_t i = 0; i < elementCount; i++) {
                        output += indent + nativeTypeName + "(";

                        if (stream >> util::braced(output, '[', ']')) {
                            output += "),\n";
                        }
                        else return false;
                    }
                    
                    output += "};\n\n";
                    continue;
                }
            }
            
            return false;
        }
        
        return true;
    }
    bool formConstBlock(util::strstream &stream, std::string &output) {
        std::uint32_t constBlockLength = 0;
        output = "struct _Constants {\n";
        
        if ((constBlockLength = formVarsBlock(stream, output, TYPES_PASS_COUNT)) == 0) {
            return false;
        }
        
        output += "};\n\n";
        return true;
    }
    bool formInoutBlock(util::strstream &stream, std::string &outputVS, std::string &outputFS) {
        outputVS += "struct _InOut {\n    float4 position [[position]];\n";
        
        if (formVarsBlock(stream, outputVS, TYPES_COUNT) == 0) {
            return false;
        }
        outputVS += "};\n\n";
        outputFS.clear();
        return true;
    }
    void addFNDefBlock(const std::string &r, const std::string &name, const std::string &sgn, const std::string &cb, std::string &functions, std::string &funcdefs) {
        functions += "    " + r + " " + name + "(" + sgn + ") {\n";
        functions += cb;
        functions += "    }\n\n";
        funcdefs += "#define " + name + " _fn." + name + "\n";
    }
    std::string formInput(const std::vector<InputLayout::Attribute> &desc, const char *prefix, const char *assign, std::string &output) {
        std::string variables;
        std::size_t index = 0;
        std::uint32_t offset = 0;
        offset = 0;
        
        for (const auto &item : desc) {
            const NativeVertexFormat &fmt = VTX_CONVERSION_TABLE[int(item.format)];
            variables += indent + "const " + fmt.shaderTypeName + " " + prefix + item.name + " = " + std::string(assign) + item.name + ";\n";
            output += indent + fmt.nativeTypeName + " " + item.name + ";\n";
            offset += fmt.size;
            index++;
        }
        
        return variables;
    }
    void formVSBlock(const InputLayout &layout, const std::string &fixed, const std::string &consts, const std::string &inoutVS, const std::string &funcs, const std::string &cb, const std::string &fdefs, std::string &vsout) {
        vsout =
            "#include <metal_stdlib>\n"
            "using namespace metal;\n"
            "\n"
            "#define _sign(a) (2.0 * step(0.0, a) - 1.0)\n"
            "#define _sin(a) sin(a)\n"
            "#define _cos(a) cos(a)\n"
            "#define _abs(a) abs(a)\n"
            "#define _sat(a) saturate(a)\n"
            "#define _frac(a) fract(a)\n"
            "#define _transform(a, b) ((b) * (a))\n"
            "#define _dot(a, b) dot((a), (b))\n"
            "#define _cross(a, b) cross((a), (b))\n"
            "#define _len(a) length(a)\n"
            "#define _pow(a, b) pow((a), (b))\n"
            "#define _floor(a) floor(a)\n"
            "#define _clamp(a) clamp(a, 0.0, 1.0)\n"
            "#define _norm(a) normalize(a)\n"
            "#define _lerp(a, b, k) mix((a), (b), k)\n"
            "#define _select(a, b, k) select((a), (b), k)\n"
            "#define _step(k, a) step((k), (a))\n"
            "#define _smooth(a, b, k) smoothstep((a), (b), (k))\n"
            "#define _min(a, b) min((a), (b))\n"
            "#define _max(a, b) max((a), (b))\n"
            "#define _tex2d(i, a) _texture##i.sample(_sampler, a)\n"
            "#define _discard() discard_fragment()\n"
            "\n"
            "struct _FrameData {\n"
            "    float4x4 plmVPMatrix;\n"
            "    float4x4 stdVPMatrix;\n"
            "    float4x4 invVPMatrix;\n"
            "    float4 cameraPosition;\n"
            "    float4 cameraDirection;\n"
            "    float4 rtBounds;\n"
            "};\n\n";

        vsout += fixed;
        vsout += consts;
        vsout += inoutVS;

        vsout +=
            "struct _FN {\n    "
            "constant const _FrameData &framedata;\n    "
            "constant const _Constants &constants;\n    "
            "thread const texture2d<float> &_texture0;\n    "
            "thread const texture2d<float> &_texture1;\n    "
            "thread const texture2d<float> &_texture2;\n    "
            "thread const texture2d<float> &_texture3;\n    "
            "thread const sampler &_sampler;\n\n";        
        
        vsout += transformCode(funcs);
        vsout += "};\n";
        vsout += fdefs;
        
        vsout += "\nstruct _VSVertexIn {\n";
        std::string variables = formInput(layout.attributes, "vertex_", "vertices[vertex_ID].", vsout);
        vsout += "};\n\nvertex _InOut main_vertex(\n";
        
        if (layout.repeat > 1) {
            vsout += "    unsigned int _r_ID [[vertex_id]],\n    unsigned int _i_ID [[instance_id]],\n";
        }
        else {
            vsout += "    unsigned int _v_ID [[vertex_id]],\n    unsigned int _i_ID [[instance_id]],\n";
        }
        
        vsout +=
            "    constant _FrameData &framedata [[buffer(0)]],\n"
            "    constant _Constants &constants [[buffer(1)]],\n"
            "    device const _VSVertexIn *vertices [[buffer(2)]],\n"
            "    constant uint &_vertexCount [[buffer(";
            
        vsout += std::to_string(VS_INPUT_VERTEX_COUNT);
        vsout += ")]],\n"
            "    sampler _sampler [[sampler(0)]],\n"
            "    texture2d<float> _texture0 [[texture(0)]],\n"
            "    texture2d<float> _texture1 [[texture(1)]],\n"
            "    texture2d<float> _texture2 [[texture(2)]],\n"
            "    texture2d<float> _texture3 [[texture(3)]])\n{\n";
        
        if (layout.repeat > 1) {
            vsout += "    const int repeat_ID = _r_ID;\n";
            vsout += "    const int vertex_ID = _i_ID % _vertexCount;\n";
            vsout += "    const int instance_ID = _i_ID / _vertexCount;\n";
        }
        else {
            vsout += "    const int repeat_ID = 0;\n";
            vsout += "    const int vertex_ID = _v_ID;\n";
            vsout += "    const int instance_ID = _i_ID;\n";
        }
        
        vsout += variables;
        vsout += "    _FN _fn {framedata, constants, _texture0, _texture1, _texture2, _texture3, _sampler};\n    _InOut output;\n\n";
        vsout += transformCode(cb);
        vsout += "\n    (void)repeat_ID; (void)vertex_ID; (void)instance_ID;(void)_fn;\n";
        vsout += "    return output;\n}\n\n";
    }
    void formFSBlock(const std::string &cb, std::string &fsout) {
        fsout +=
            "struct _Output {\n"
            "    float4 c0[[color(0)]];\n"
            "    float4 c1[[color(1)]];\n"
            "    float4 c2[[color(2)]];\n"
            "    float4 c3[[color(3)]];\n"
            "};\n\n"
            "fragment _Output main_fragment(\n    "
            "_InOut input [[stage_in]],\n    "
            "sampler _sampler [[sampler(0)]],\n    "
            "texture2d<float> _texture0 [[texture(0)]],\n    "
            "texture2d<float> _texture1 [[texture(1)]],\n    "
            "texture2d<float> _texture2 [[texture(2)]],\n    "
            "texture2d<float> _texture3 [[texture(3)]],\n"
            "";

        fsout += "    constant _FrameData &framedata [[buffer(0)]],\n";
        fsout += "    constant _Constants &constants [[buffer(1)]])\n{\n";
        fsout += "    float2 fragment_coord = input.position.xy / framedata.rtBounds.xy;\n";
        fsout += "    float4 output_color[4] = {};\n    _FN _fn {framedata, constants, _texture0, _texture1, _texture2, _texture3, _sampler};\n\n";
        fsout += transformCode(cb);
        fsout += "\n    return _Output {output_color[0], output_color[1], output_color[2], output_color[3]};\n";
        fsout += "    (void)fragment_coord;(void)_fn;\n";
        fsout += "}\n";
    }
#endif
#ifdef PLATFORM_WASM

#endif

}

namespace foundation {
    std::pair<std::string, std::string> makePlatformShaderSource(const char *src, const InputLayout &layout, std::string &error) {
        util::strstream input = util::strstream(src, strlen(src));
        
        bool completed = true;
        bool fixedBlockDone = false;
        bool constBlockDone = false;
        bool inoutBlockDone = false;
        bool vssrcBlockDone = false;
        bool fssrcBlockDone = false;
        
        std::string blockName;
        std::uint32_t constBlockLength = 0;
        
        std::string shaderBlockFixed;
        std::string shaderBlockConsts;
        std::string shaderBlockInout;
        std::string shaderBlockFunctions;
        std::string shaderBlockFuncdefs;
        
        std::string resultvs, resultfs;
        error.clear();
        
        while (input >> blockName) {
            if (fixedBlockDone == false && blockName == "fixed" && (input >> util::sequence("{"))) {
                if (formFixedBlock(input, shaderBlockFixed) == false) {
                    error = "shader has ill-formed 'fixed' block";
                    completed = false;
                    break;
                }

                fixedBlockDone = true;
                continue;
            }
            if (constBlockDone == false && blockName == "const" && (input >> util::sequence("{"))) {
                if (formConstBlock(input, shaderBlockConsts) == false) {
                    error = "shader has ill-formed 'const' block";
                    completed = false;
                    break;
                }
                constBlockDone = true;
                continue;
            }
            if (inoutBlockDone == false && blockName == "inout" && (input >> util::sequence("{"))) {
                if (formInoutBlock(input, shaderBlockInout) == false) {
                    error = "shader has ill-formed 'inout' block";
                    completed = false;
                    break;
                }
                inoutBlockDone = true;
                continue;
            }
            if (blockName == "fndef") {
                std::string funcName;
                std::string funcSignature;
                std::string funcReturnType;
                
                if (input >> util::word(funcName) >> util::braced(funcSignature, '(', ')') >> util::sequence("->") >> funcReturnType >> util::sequence("{")) {
                    std::string codeBlock;
                    
                    if (shaderUtils::formCodeBlock("        ", input, codeBlock)) {
                        addFNDefBlock(funcReturnType, funcName, funcSignature, codeBlock, shaderBlockFunctions, shaderBlockFuncdefs);
                    }
                    else {
                        error = "shader has uncompleted 'fndef' block";
                        completed = false;
                        break;
                    }
                }
                else {
                    error = "shader has invalid 'fndef' block";
                    completed = false;
                    break;
                }
                continue;
            }
            if (vssrcBlockDone == false && blockName == "vssrc" && (input >> util::sequence("{"))) {
                if (constBlockDone == false) {
                    constBlockDone = true;
                    formEmptyFixedBlock(shaderBlockFixed);
                }
                if (constBlockDone == false) {
                    constBlockDone = true;
                    formEmptyConstBlock(shaderBlockConsts);
                }
                if (inoutBlockDone == false) {
                    inoutBlockDone = true;
                    formEmptyInoutBlock(shaderBlockInout);
                }
                
                std::string codeBlock;
                if (shaderUtils::formCodeBlock(indent, input, codeBlock) == false) {
                    error = "shader has uncompleted 'vssrc' block";
                    completed = false;
                    break;
                }
                
                formVSBlock(layout, shaderBlockFixed, shaderBlockConsts, shaderBlockInout, shaderBlockFunctions, codeBlock, shaderBlockFuncdefs, resultvs);
                vssrcBlockDone = true;
                continue;
            }
            if (fssrcBlockDone == false && blockName == "fssrc" && (input >> util::sequence("{"))) {
                if (vssrcBlockDone == false) {
                    error = "'vssrc' block must be defined before 'fssrc'\n";
                    completed = false;
                    break;
                }

                std::string codeBlock;
                if (shaderUtils::formCodeBlock(indent, input, codeBlock) == false) {
                    error = "shader has uncompleted 'fssrc' block";
                    completed = false;
                    break;
                }


                fssrcBlockDone = true;
                continue;
            }
            
            error = "shader has unexpected block\n";
        }
        
        if (error.empty()) {
            return std::make_pair(std::move(resultvs), std::move(resultfs));
        }
        return {};
    }
}


