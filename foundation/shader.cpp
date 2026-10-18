
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
#ifdef PLATFORM_WINDOWS
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
}

namespace {
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
#ifdef PLATFORM_WINDOWS
    const NativeVertexFormat VTX_CONVERSION_TABLE[] = { // index is InputAttributeFormat value
        {"float2",  "float2", 4,  2},
        {"float4",  "float4", 8,  4},
        {"float",   "float",  4,  1},
        {"float2",  "float2", 8,  2},
        {"float3",  "float3", 12, 3},
        {"float4",  "float4", 16, 4},
        {"int2",    "int2",   4,  2},
        {"int4",    "int4",   8,  4},
        {"uint2",   "uint2",  4,  2},
        {"uint4",   "uint4",  8,  4},
        {"float2",  "float2", 4,  2},
        {"float4",  "float4", 8,  4},
        {"float2",  "float2", 4,  2},
        {"float4",  "float4", 8,  4},
        {"uint4",   "uint4",  4,  4},
        {"float4",  "float4", 4,  4},
        {"int",     "int",    4,  1},
        {"int2",    "int2",   8,  2},
        {"int3",    "int3",   12, 3},
        {"int4",    "int4",   16, 4},
        {"uint",    "uint",   4,  1},
        {"uint2",   "uint2",  8,  2},
        {"uint3",   "uint3",  12, 3},
        {"uint4",   "uint4",  16, 4},
    };
#endif
}

namespace foundation {
    std::uint32_t InputLayout::getStride() const {
        std::uint32_t stride = 0;
        for (const InputLayout::Attribute &item : attributes) {
            stride += VTX_CONVERSION_TABLE[int(item.format)].size;
        }
        return stride;
    }
}

namespace {
#ifdef PLATFORM_IOS
    static const std::uint32_t VS_INPUT_VERTEX_COUNT = 3;

    void transformTypes(std::string &code) {
        shaderUtils::replace(code, "float1", "float", SEPARATORS, SEPARATORS);
        shaderUtils::replace(code, "int1", "int", SEPARATORS, SEPARATORS);
        shaderUtils::replace(code, "uint1", "uint", SEPARATORS, SEPARATORS);
    }
    std::string transformCode(const std::string &src) {
        std::string result = src;
        transformTypes(result);
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
    void formEmptyInoutBlock(std::string &outputVS, std::string &outputFS) {
        outputVS = "struct _InOut {\n    float4 position [[position]];\n};\n\n";
        outputFS.clear();
    }
    bool formFixedBlock(util::strstream &stream, std::string &output) {
        std::string varname, arg;
        
        while (stream >> varname && varname[0] != '}') {
            if (stream >> util::sequence(":") >> arg >> util::sequence("=")) {
                std::size_t elementSize = 0, elementCount = shaderUtils::getArrayMultiply(varname);
                std::string nativeTypeName;
                
                if (shaderUtils::shaderGetTypeSize(arg, TYPE_SIZE_TABLE, TYPES_COUNT, nativeTypeName, elementSize)) {
                    if (varname.find('[') == std::string::npos) {
                        output += "constant " + nativeTypeName + " fixed_" + varname + " = " + nativeTypeName + "(";

                        if (stream >> util::braced(output, '[', ']')) {
                            output += ");\n\n";
                            continue;
                        }
                        else return false;
                    }

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
    std::uint32_t formConstBlock(util::strstream &stream, std::string &output) {
        std::uint32_t constBlockLength = 0;
        output = "struct _Constants {\n";
        
        if ((constBlockLength = formVarsBlock(stream, output, TYPES_PASS_COUNT)) == 0) {
            return 0;
        }
        
        output += "};\n\n";
        return constBlockLength;
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
    std::string formInput(const std::vector<foundation::InputLayout::Attribute> &desc, const char *prefix, const char *assign, std::string &output) {
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
    void formVSBlock(const foundation::InputLayout &layout, const std::string &fixed, const std::string &consts, const std::string &inoutVS, const std::string &funcs, const std::string &cb, const std::string &fdefs, std::string &vsout) {
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
    void formFSBlock(const std::string &fixed, const std::string &consts, const std::string &inoutFS, const std::string &funcs, const std::string &cb, const std::string &fdefs, std::string &fsout) {
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
        
        std::string codeBlock = cb;
        transformTypes(codeBlock);
        shaderUtils::replace(codeBlock, "const_", "constants.", SEPARATORS);
        shaderUtils::replace(codeBlock, "frame_", "framedata.", SEPARATORS);
        shaderUtils::replace(codeBlock, "input_", "input.", SEPARATORS);
        fsout += codeBlock;
        
        fsout += "\n    return _Output {output_color[0], output_color[1], output_color[2], output_color[3]};\n";
        fsout += "    (void)fragment_coord;(void)_fn;\n";
        fsout += "}\n";
    }
#endif
#ifdef PLATFORM_WASM
static const std::string SHADER_DEFINES =
    "#version 300 es\n"
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
    "#define _select(a, b, k) (k ? (b) : (a))\n"
    "#define _step(k, a) step((k), (a))\n"
    "#define _smooth(a, b, k) smoothstep((a), (b), (k))\n"
    "#define _min(a, b) min((a), (b))\n"
    "#define _max(a, b) max((a), (b))\n"
    "#define _tex2d(i, a) texture(_samplers[i], a)\n"
    "#define _discard() discard"
    "\n"
    "precision mediump float;\n"
    "\n"
    "layout(std140) uniform _FrameData {\n"
    "    mediump mat4 plmVPMatrix;\n"
    "    mediump mat4 stdVPMatrix;\n"
    "    mediump mat4 invVPMatrix;\n"
    "    mediump vec4 cameraPosition;\n"
    "    mediump vec4 cameraDirection;\n"
    "    mediump vec4 rtBounds;\n"
    "}\n"
    "framedata;\n\n"
    "uniform sampler2D _samplers[4];\n\n";

std::string transformCode(const std::string &src) {
    std::string result = src;
    shaderUtils::replace(result, "float", "vec", SEPARATORS, "234");
    shaderUtils::replace(result, "int", "ivec", SEPARATORS, "234");
    shaderUtils::replace(result, "uint", "uvec", SEPARATORS, "234");
    shaderUtils::replace(result, "float1", "float", SEPARATORS, SEPARATORS);
    shaderUtils::replace(result, "int1", "int", SEPARATORS, SEPARATORS);
    shaderUtils::replace(result, "uint1", "uint", SEPARATORS, SEPARATORS);
    shaderUtils::replace(result, "const_", "constants.", SEPARATORS);
    shaderUtils::replace(result, "frame_", "framedata.", SEPARATORS);
    return result;
}
std::uint32_t formVarsBlock(util::strstream &stream, std::string &output, std::size_t allowedTypeCount, std::string *varyingsFS = nullptr) {
    std::string varname, arg;
    std::uint32_t totalLength = 0;

    while (stream >> varname && varname[0] != '}') {
        if (stream >> util::sequence(":") >> arg) {
            std::size_t elementSize = 0, elementCount = shaderUtils::getArrayMultiply(varname);
            std::string nativeTypeName;

            if (shaderGetTypeSize(arg, TYPE_SIZE_TABLE, allowedTypeCount, nativeTypeName, elementSize)) {
                if (varyingsFS) {
                    // integer varyings must be flat
                    const std::string flat = nativeTypeName[0] == 'i' || nativeTypeName[0] == 'u' ? "flat " : "";
                    const std::string decl = "mediump " + nativeTypeName + " passing_" + varname + ";\n";
                    output += flat + "out " + decl;
                    *varyingsFS += flat + "in " + decl;
                }
                else {
                    output += indent + "mediump " + nativeTypeName + " " + varname + ";\n";
                }
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
    output = "\n";
}
void formEmptyInoutBlock(std::string &outputVS, std::string &outputFS) {
    outputVS = "\n";
    outputFS = "\n";
}
bool formFixedBlock(util::strstream &stream, std::string &output) {
    std::string varname, arg;
    
    while (stream >> varname && varname[0] != '}') {
        if (stream >> util::sequence(":") >> arg >> util::sequence("=")) {
            std::size_t elementSize = 0, elementCount = shaderUtils::getArrayMultiply(varname);
            std::string nativeTypeName;
            
            if (shaderUtils::shaderGetTypeSize(arg, TYPE_SIZE_TABLE, TYPES_COUNT, nativeTypeName, elementSize)) {
                if (varname.find('[') == std::string::npos) {
                    output += "const mediump " + nativeTypeName + " fixed_" + varname + " = " + nativeTypeName + "(";

                    if (stream >> util::braced(output, '[', ']')) {
                        output += ");\n\n";
                        continue;
                    }
                    else return false;
                }

                output += "const mediump " + nativeTypeName + " fixed_" + varname + " = " + nativeTypeName + "[](\n";
                for (std::size_t i = 0; i < elementCount; i++) {
                    output += indent + nativeTypeName + "(";

                    if (stream >> util::braced(output, '[', ']')) {
                        output += i == elementCount -1 ? ")\n" : "),\n";
                    }
                    else return false;
                }

                output += ");\n\n";
                continue;
            }
        }
        
        return false;
    }
    
    return true;
}
std::uint32_t formConstBlock(util::strstream &stream, std::string &output) {
    std::uint32_t constBlockLength = 0;
    output = "layout(std140) uniform _Constants {\n";
    
    if ((constBlockLength = formVarsBlock(stream, output, TYPES_PASS_COUNT)) == 0) {
        return 0;
    }
    
    output += "}\nconstants;\n\n";
    return constBlockLength;
}
bool formInoutBlock(util::strstream &stream, std::string &outputVS, std::string &outputFS) {
    outputVS.clear();
    outputFS.clear();

    if (formVarsBlock(stream, outputVS, TYPES_COUNT, &outputFS) == 0) {
        return false;
    }

    outputVS += "\n";
    outputFS += "\n";
    return true;
}
void addFNDefBlock(const std::string &r, const std::string &name, const std::string &sgn, const std::string &cb, std::string &functions, std::string &funcdefs) {
    functions += r + " " + name + "(" + sgn + ") {\n";
    functions += cb;
    functions += "}\n\n";
    funcdefs.clear();
}
void formVSBlock(const foundation::InputLayout &layout, const std::string &fixed, const std::string &consts, const std::string &inoutVS, const std::string &funcs, const std::string &cb, const std::string &fdefs, std::string &vsout) {
    vsout = SHADER_DEFINES;
    vsout += "#define output_position gl_Position\n\n";
    vsout += fixed;
    vsout += consts;
    vsout += transformCode(funcs);
    vsout += inoutVS;
    
    for (std::size_t i = 0; i < layout.attributes.size(); i++) {
        const foundation::InputLayout::Attribute &attribute = layout.attributes[i];
        const std::string type = std::string(VTX_CONVERSION_TABLE[int(attribute.format)].nativeTypeName);
        vsout += "layout(location = " + std::to_string(i) + ") in " + type + " vertex_" + std::string(attribute.name) + ";\n";
    }
    if (layout.attributes.size()) {
        vsout += "\n";
    }
    
    if (layout.repeat > 1) {
        vsout += "uniform int _instance_count;\n";
        vsout += "\n#define repeat_ID gl_VertexID\n";
        vsout += "\nvoid main() {\n    int vertex_ID = gl_InstanceID % _instance_count;\n    int instance_ID = gl_InstanceID / _instance_count;\n";
    }
    else {
        vsout += "\n#define repeat_ID 0\n#define vertex_ID gl_VertexID\n#define instance_ID gl_InstanceID\n";
        vsout += "\nvoid main() {\n";
    }
    
    std::string codeBlock = transformCode(cb);
    shaderUtils::replace(codeBlock, "output_", "passing_", SEPARATORS, {"output_position"});
    vsout += codeBlock;
    
    vsout += "    gl_Position.y *= -1.0;\n";
    vsout += "}\n\n";    
}
void formFSBlock(const std::string &fixed, const std::string &consts, const std::string &inoutFS, const std::string &funcs, const std::string &cb, const std::string &fdefs, std::string &fsout) {
    fsout = SHADER_DEFINES + fixed + consts;
    fsout += transformCode(funcs);
    fsout += inoutFS;
    std::string codeBlock = transformCode(cb);
    shaderUtils::replace(codeBlock, "input_", "passing_", SEPARATORS, {"input_position"});
    
    fsout += "out vec4 output_color[4];\n\n";
    fsout += "void main() {\n";
    fsout += codeBlock;
    fsout += "}\n\n";
}

#endif
#ifdef PLATFORM_WINDOWS
static const std::string SHADER_DEFINES =
    "#define _sign(a) (2.0 * step(0.0, a) - 1.0)\n"
    "#define _sin(a) sin(a)\n"
    "#define _cos(a) cos(a)\n"
    "#define _abs(a) abs(a)\n"
    "#define _sat(a) saturate(a)\n"
    "#define _frac(a) frac(a)\n"
    "#define _transform(a, b) mul((b), (a))\n"
    "#define _dot(a, b) dot((a), (b))\n"
    "#define _cross(a, b) cross((a), (b))\n"
    "#define _len(a) length(a)\n"
    "#define _pow(a, b) pow((a), (b))\n"
    "#define _floor(a) floor(a)\n"
    "#define _clamp(a) saturate(a)\n"
    "#define _norm(a) normalize(a)\n"
    "#define _lerp(a, b, k) lerp((a), (b), k)\n"
    "#define _select(a, b, k) ((k) ? (b) : (a))\n"
    "#define _step(k, a) step((k), (a))\n"
    "#define _smooth(a, b, k) smoothstep((a), (b), (k))\n"
    "#define _min(a, b) min((a), (b))\n"
    "#define _max(a, b) max((a), (b))\n"
    "\n"
    "cbuffer _FrameData : register(b0) {\n"
    "    float4x4 frame_plmVPMatrix;\n"
    "    float4x4 frame_stdVPMatrix;\n"
    "    float4x4 frame_invVPMatrix;\n"
    "    float4 frame_cameraPosition;\n"
    "    float4 frame_cameraDirection;\n"
    "    float4 frame_rtBounds;\n"
    "};\n\n"
    "Texture2D _texture0 : register(t0);\n"
    "Texture2D _texture1 : register(t1);\n"
    "Texture2D _texture2 : register(t2);\n"
    "Texture2D _texture3 : register(t3);\n"
    "SamplerState _sampler0 : register(s0);\n"
    "SamplerState _sampler1 : register(s1);\n"
    "SamplerState _sampler2 : register(s2);\n"
    "SamplerState _sampler3 : register(s3);\n\n";

std::string transformCode(const std::string &src) {
    std::string result = src;
    // fxc doesn't support macros with empty parameter list
    shaderUtils::replace(result, "_discard()", "discard", SEPARATORS);
    shaderUtils::replace(result, "output_", "output.", SEPARATORS);
    shaderUtils::replace(result, "input_", "input.", SEPARATORS);
    return result;
}
// withSemantics == true: 'inout' struct members with interpolation semantics
std::uint32_t formVarsBlock(util::strstream &stream, std::string &output, std::size_t allowedTypeCount, const std::string &prefix, bool withSemantics) {
    std::string varname, arg;
    std::uint32_t totalLength = 0;
    std::uint32_t semanticIndex = 0;

    while (stream >> varname && varname[0] != '}') {
        if (stream >> util::sequence(":") >> arg) {
            std::size_t elementSize = 0, elementCount = shaderUtils::getArrayMultiply(varname);
            std::string nativeTypeName;

            if (shaderGetTypeSize(arg, TYPE_SIZE_TABLE, allowedTypeCount, nativeTypeName, elementSize)) {
                if (withSemantics) {
                    // integer values can't be interpolated
                    const std::string qualifier = nativeTypeName[0] == 'i' || nativeTypeName[0] == 'u' ? "nointerpolation " : "";
                    output += indent + qualifier + nativeTypeName + " " + prefix + varname + " : TEXCOORD" + std::to_string(semanticIndex) + ";\n";
                    semanticIndex += std::uint32_t((elementSize + 15) / 16 * elementCount);
                }
                else {
                    output += indent + nativeTypeName + " " + prefix + varname + ";\n";
                }
                totalLength += std::uint32_t(elementSize * elementCount);
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
    output = "\n";
}
void formEmptyInoutBlock(std::string &outputVS, std::string &outputFS) {
    outputVS = "struct _InOut {\n    float4 position : SV_Position;\n};\n\n";
    outputFS = outputVS;
}
bool formFixedBlock(util::strstream &stream, std::string &output) {
    std::string varname, arg;

    while (stream >> varname && varname[0] != '}') {
        if (stream >> util::sequence(":") >> arg >> util::sequence("=")) {
            std::size_t elementSize = 0, elementCount = shaderUtils::getArrayMultiply(varname);
            std::string nativeTypeName;

            if (shaderUtils::shaderGetTypeSize(arg, TYPE_SIZE_TABLE, TYPES_COUNT, nativeTypeName, elementSize)) {
                if (varname.find('[') == std::string::npos) {
                    output += "static const " + nativeTypeName + " fixed_" + varname + " = " + nativeTypeName + "(";

                    if (stream >> util::braced(output, '[', ']')) {
                        output += ");\n\n";
                        continue;
                    }
                    else return false;
                }

                output += "static const " + nativeTypeName + " fixed_" + varname + " = {\n";
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
std::uint32_t formConstBlock(util::strstream &stream, std::string &output) {
    std::uint32_t constBlockLength = 0;
    output = "cbuffer _Constants : register(b1) {\n";

    if ((constBlockLength = formVarsBlock(stream, output, TYPES_PASS_COUNT, "const_", false)) == 0) {
        return 0;
    }

    output += "};\n\n";
    return constBlockLength;
}
bool formInoutBlock(util::strstream &stream, std::string &outputVS, std::string &outputFS) {
    outputVS = "struct _InOut {\n    float4 position : SV_Position;\n";

    if (formVarsBlock(stream, outputVS, TYPES_COUNT, "", true) == 0) {
        return false;
    }

    outputVS += "};\n\n";
    outputFS = outputVS;
    return true;
}
void addFNDefBlock(const std::string &r, const std::string &name, const std::string &sgn, const std::string &cb, std::string &functions, std::string &funcdefs) {
    functions += r + " " + name + "(" + sgn + ") {\n";
    functions += cb;
    functions += "}\n\n";
    funcdefs.clear();
}
void formVSBlock(const foundation::InputLayout &layout, const std::string &fixed, const std::string &consts, const std::string &inoutVS, const std::string &funcs, const std::string &cb, const std::string &fdefs, std::string &vsout) {
    vsout = SHADER_DEFINES;
    vsout += "#define _tex2d(i, a) _texture##i.SampleLevel(_sampler##i, a, 0)\n\n";
    vsout += fixed;
    vsout += consts;
    vsout += inoutVS;

    if (layout.repeat > 1) {
        vsout += "cbuffer _DrawData : register(b2) {\n    uint _vertexCount;\n};\n\n";
    }

    std::string variables;

    if (layout.attributes.size()) {
        vsout += "struct _VSIn {\n";

        for (std::size_t i = 0; i < layout.attributes.size(); i++) {
            const foundation::InputLayout::Attribute &attribute = layout.attributes[i];
            const NativeVertexFormat &fmt = VTX_CONVERSION_TABLE[int(attribute.format)];
            vsout += indent + fmt.nativeTypeName + " " + attribute.name + " : VTX" + std::to_string(i) + ";\n";
            variables += indent + "const " + fmt.shaderTypeName + " vertex_" + attribute.name + " = _vertex." + attribute.name + ";\n";
        }

        vsout += "};\n\n";
    }

    vsout += transformCode(funcs);
    vsout += "_InOut main(\n";

    if (layout.attributes.size()) {
        vsout += "    _VSIn _vertex,\n";
    }

    vsout += "    uint _v_ID : SV_VertexID,\n    uint _i_ID : SV_InstanceID)\n{\n";

    if (layout.repeat > 1) {
        vsout += "    const int repeat_ID = _v_ID;\n";
        vsout += "    const int vertex_ID = _i_ID % _vertexCount;\n";
        vsout += "    const int instance_ID = _i_ID / _vertexCount;\n";
    }
    else {
        vsout += "    const int repeat_ID = 0;\n";
        vsout += "    const int vertex_ID = _v_ID;\n";
        vsout += "    const int instance_ID = _i_ID;\n";
    }

    vsout += variables;
    vsout += "    _InOut output = (_InOut)0;\n\n";
    vsout += transformCode(cb);
    vsout += "\n    return output;\n}\n\n";
}
void formFSBlock(const std::string &fixed, const std::string &consts, const std::string &inoutFS, const std::string &funcs, const std::string &cb, const std::string &fdefs, std::string &fsout) {
    fsout = SHADER_DEFINES;
    fsout += "#define _tex2d(i, a) _texture##i.Sample(_sampler##i, a)\n\n";
    fsout += fixed;
    fsout += consts;
    fsout += inoutFS;
    fsout += transformCode(funcs);
    fsout +=
        "struct _Output {\n"
        "    float4 c0 : SV_Target0;\n"
        "    float4 c1 : SV_Target1;\n"
        "    float4 c2 : SV_Target2;\n"
        "    float4 c3 : SV_Target3;\n"
        "};\n\n"
        "_Output main(_InOut input) {\n"
        "    float2 fragment_coord = input.position.xy / frame_rtBounds.xy;\n"
        "    float4 output_color[4] = {(float4)0, (float4)0, (float4)0, (float4)0};\n\n";

    std::string codeBlock = cb;
    shaderUtils::replace(codeBlock, "_discard()", "discard", SEPARATORS);
    shaderUtils::replace(codeBlock, "input_", "input.", SEPARATORS);
    fsout += codeBlock;

    fsout += "\n    _Output _out = {output_color[0], output_color[1], output_color[2], output_color[3]};\n";
    fsout += "    return _out;\n}\n\n";
}

#endif
}

namespace foundation {
    std::tuple<std::string, std::string, std::uint32_t> makePlatformShaderSource(const char *src, const InputLayout &layout, std::string &error) {
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
        std::string shaderBlockInoutVS;
        std::string shaderBlockInoutFS;
        std::string shaderBlockFunctions;
        std::string shaderBlockFuncdefs;
        
        std::string resultvs, resultfs;
        error.clear();
        
        if (!layout.attributes.empty() || layout.repeat > 1) {
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
                    if ((constBlockLength = formConstBlock(input, shaderBlockConsts)) == 0) {
                        error = "shader has ill-formed 'const' block";
                        completed = false;
                        break;
                    }
                    constBlockDone = true;
                    continue;
                }
                if (inoutBlockDone == false && blockName == "inout" && (input >> util::sequence("{"))) {
                    if (formInoutBlock(input, shaderBlockInoutVS, shaderBlockInoutFS) == false) {
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
                    if (fixedBlockDone == false) {
                        fixedBlockDone = true;
                        formEmptyFixedBlock(shaderBlockFixed);
                    }
                    if (constBlockDone == false) {
                        constBlockDone = true;
                        formEmptyConstBlock(shaderBlockConsts);
                    }
                    if (inoutBlockDone == false) {
                        inoutBlockDone = true;
                        formEmptyInoutBlock(shaderBlockInoutVS, shaderBlockInoutFS);
                    }

                    std::string codeBlock;
                    if (shaderUtils::formCodeBlock(indent, input, codeBlock) == false) {
                        error = "shader has uncompleted 'vssrc' block";
                        completed = false;
                        break;
                    }

                    formVSBlock(layout, shaderBlockFixed, shaderBlockConsts, shaderBlockInoutVS, shaderBlockFunctions, codeBlock, shaderBlockFuncdefs, resultvs);
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

                    formFSBlock(shaderBlockFixed, shaderBlockConsts, shaderBlockInoutFS, shaderBlockFunctions, codeBlock, shaderBlockFuncdefs, resultfs);
                    fssrcBlockDone = true;
                    continue;
                }

                error = "shader has unexpected block\n";
            }
        }
        else {
            error = "layout should either have vertex attributes or repeat > 1";
        }
        
        if (error.empty()) {
            return std::make_tuple(std::move(resultvs), std::move(resultfs), constBlockLength);
        }
        return {};
    }
}


