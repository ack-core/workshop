
#include <vector>
#include <algorithm>

#ifdef PLATFORM_IOS

#if ! __has_feature(objc_arc)
#error "ARC is off"
#endif

#include "util.h"
#include "rendering_metal.h"

namespace {
    static const MTLPrimitiveType g_topologies[] = {
        MTLPrimitiveTypePoint,
        MTLPrimitiveTypeLine,
        MTLPrimitiveTypeLineStrip,
        MTLPrimitiveTypeTriangle,
        MTLPrimitiveTypeTriangleStrip,
    };
    
    static const std::uint32_t MAX_TEXTURES = 4;
    static const std::uint32_t FRAME_CONST_BINDING_INDEX = 0;
    static const std::uint32_t DRAW_CONST_BINDING_INDEX = 1;
    static const std::uint32_t VERTEX_IN_BINDING_START = 2;
    static const std::uint32_t VERTEX_IN_VERTEX_COUNT = 3;
    
    std::uint32_t roundTo256(std::uint32_t value) {
        std::uint32_t result = ((value - std::uint32_t(1)) & ~std::uint32_t(255)) + 256;
        return result;
    }
    
    const std::string &generateRenderPipelineStateName(const foundation::MetalShader *platformShader, const foundation::MetalTarget *platformTarget, foundation::BlendType blendType) {
        static std::string buffer = std::string(1024, 0);
        strcpy(buffer.data(), "r0_b0_");
        strcpy(buffer.data() + 6, platformShader->getName().c_str());
        buffer[1] = platformTarget ? '0' + int(platformTarget->getTexture(0)->getFormat()) : int(foundation::RenderTextureFormat::RGBA8UN);
        buffer[4] = '0' + int(blendType);
        return buffer;
    }
    
    void (*initializeBlendOptions[])(MTLRenderPipelineColorAttachmentDescriptor *target) = {
        [](MTLRenderPipelineColorAttachmentDescriptor *target){},
        [](MTLRenderPipelineColorAttachmentDescriptor *target){
            target.blendingEnabled = false;
        },
        [](MTLRenderPipelineColorAttachmentDescriptor *target){
            target.blendingEnabled = true;
            target.alphaBlendOperation = MTLBlendOperationAdd;
            target.rgbBlendOperation = MTLBlendOperationAdd;
            target.sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
            target.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
            target.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            target.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        },
        [](MTLRenderPipelineColorAttachmentDescriptor *target){
            target.alphaBlendOperation = MTLBlendOperationAdd;
            target.rgbBlendOperation = MTLBlendOperationAdd;
            target.sourceAlphaBlendFactor = MTLBlendFactorOne;
            target.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
            target.destinationAlphaBlendFactor = MTLBlendFactorOne;
            target.destinationRGBBlendFactor = MTLBlendFactorOne;
        }
    };
    
    struct NativeFormat {
        const char *nativeUnpackedName;
        const char *nativePackedTypeName;
        std::uint32_t size;
        MTLVertexFormat nativeFormat;
    }
    g_formatConversionTable[] = { // index is RenderShaderInputFormat value
        {"float2",  "packed_half2",         4,  MTLVertexFormatHalf2},
        {"float4",  "packed_half4",         8,  MTLVertexFormatHalf4},
        {"float",   "packed_float",         4,  MTLVertexFormatFloat},
        {"float2",  "packed_float2",        8,  MTLVertexFormatFloat2},
        {"float3",  "packed_float3",        12, MTLVertexFormatFloat3},
        {"float4",  "packed_float4",        16, MTLVertexFormatFloat4},
        {"short2",  "packed_short2",        4,  MTLVertexFormatShort2},
        {"short4",  "packed_short4",        8,  MTLVertexFormatShort4},
        {"ushort2", "packed_ushort2",       4,  MTLVertexFormatUShort2},
        {"ushort4", "packed_ushort4",       8,  MTLVertexFormatUShort4},
        {"float2",  "rg16snorm<float2>",    4,  MTLVertexFormatShort2Normalized},
        {"float4",  "rgba16snorm<float4>",  8,  MTLVertexFormatShort4Normalized},
        {"float2",  "rg16unorm<float2>",    4,  MTLVertexFormatUShort2Normalized},
        {"float4",  "rgba16unorm<float4>",  8,  MTLVertexFormatUShort4Normalized},
        {"uchar4",  "packed_uchar4",        4,  MTLVertexFormatUChar4},
        {"float4",  "rgba8unorm<float4>",   4,  MTLVertexFormatUChar4Normalized},
        {"int",     "packed_int",           4,  MTLVertexFormatInt},
        {"int2",    "packed_int2",          8,  MTLVertexFormatInt2},
        {"int3",    "packed_int3",          12, MTLVertexFormatInt3},
        {"int4",    "packed_int4",          16, MTLVertexFormatInt4},
        {"uint",    "packed_uint",          4,  MTLVertexFormatUInt},
        {"uint2",   "packed_uint2",         8,  MTLVertexFormatUInt2},
        {"uint3",   "packed_uint3",         12, MTLVertexFormatUInt3},
        {"uint4",   "packed_uint4",         16, MTLVertexFormatUInt4}
    };
}

namespace foundation {
    MetalShader::MetalShader(const std::string &name, const InputLayout &layout, id<MTLLibrary> library, std::uint32_t constBufferLength)
        : _name(name)
        , _layout(layout)
        , _constBufferLength(constBufferLength)
        , _library(library)
    {}
    
    MetalShader::~MetalShader() {
        _library = nil;
    }
    
    const InputLayout &MetalShader::getInputLayout() const {
        return _layout;
    }
    
    id<MTLFunction> MetalShader::getVertexShader() const {
        return [_library newFunctionWithName:@"main_vertex"];
    }
    
    id<MTLFunction> MetalShader::getFragmentShader() const {
        return [_library newFunctionWithName:@"main_fragment"];
    }
    
    std::uint32_t MetalShader::getConstBufferLength() const {
        return _constBufferLength;
    }
    
    const std::string &MetalShader::getName() const {
        return _name;
    }
}

namespace foundation {
    MetalTexture::MetalTexture(id<MTLTexture> texture, RenderTextureFormat fmt, std::uint32_t w, std::uint32_t h, std::uint32_t mipCount)
        : _texture(texture)
        , _format(fmt)
        , _width(w)
        , _height(h)
        , _mipCount(mipCount)
    {
    }
    
    MetalTexture::~MetalTexture() {
        _texture = nil;
    }
    
    std::uint32_t MetalTexture::getWidth() const {
        return _width;
    }
    
    std::uint32_t MetalTexture::getHeight() const {
        return _height;
    }
    
    std::uint32_t MetalTexture::getMipCount() const {
        return _mipCount;
    }
    
    RenderTextureFormat MetalTexture::getFormat() const {
        return _format;
    }

    id<MTLTexture> MetalTexture::getNativeTexture() const {
        return _texture;
    }
}

namespace foundation {
    MetalTarget::MetalTarget(__strong id<MTLTexture> *targets, std::uint32_t count, id<MTLTexture> depth, RenderTextureFormat fmt, std::uint32_t w, std::uint32_t h)
        : _count(count)
        , _width(w)
        , _height(h)
    {
        for (unsigned i = 0; i < count; i++) {
            _textures[i] = std::make_shared<RTTexture>(*this, fmt, targets[i]);
        }
        _depth = std::make_shared<RTTexture>(*this, RenderTextureFormat::R32F, depth);
    }
    
    MetalTarget::~MetalTarget() {
        for (unsigned i = 0; i < _count; i++) {
            _textures[i] = nullptr;
        }
        _depth = nullptr;
    }
    
    std::uint32_t MetalTarget::getWidth() const {
        return _width;
    }
    
    std::uint32_t MetalTarget::getHeight() const {
        return _height;
    }
    
    RenderTextureFormat MetalTarget::getFormat() const {
        return _textures[0]->getFormat();
    }
    
    std::uint32_t MetalTarget::getTextureCount() const {
        return _count;
    }
    
    const std::shared_ptr<RenderTexture> &MetalTarget::getTexture(unsigned index) const {
        return _textures[index];
    }
    
    const std::shared_ptr<RenderTexture> &MetalTarget::getDepth() const {
        return _depth;
    }
}

namespace foundation {
    MetalData::MetalData(id<MTLBuffer> vdata, id<MTLBuffer> indexes, std::uint32_t vcnt, std::uint32_t icnt, std::uint32_t stride)
        : _vertices(vdata)
        , _indexes(indexes)
        , _vcount(vcnt)
        , _icount(icnt)
        , _stride(stride)
    {
    }
    
    MetalData::~MetalData() {
        _vertices = nil;
        _indexes = nil;
    }
    
    std::uint32_t MetalData::getVertexCount() const {
        return _vcount;
    }
    std::uint32_t MetalData::getIndexCount() const {
        return _icount;
    }
    
    std::uint32_t MetalData::getStride() const {
        return _stride;
    }
    
    id<MTLBuffer> MetalData::getVertexes() const {
        return _vertices;
    }
    id<MTLBuffer> MetalData::getIndexes() const {
        return _indexes;
    }
}

namespace foundation {
    MetalRendering::MetalRendering(const std::shared_ptr<PlatformInterface> &platform) : _platform(platform) {
        @autoreleasepool {
            _platform->logMsg("[RENDER] Initialization : Metal");
            
            _device = MTLCreateSystemDefaultDevice();
            _commandQueue = [_device newCommandQueue];
            _constBufferSemaphore = dispatch_semaphore_create(CONSTANT_BUFFER_FRAMES_MAX);
            
            MTLSamplerDescriptor *samplerDesc = [MTLSamplerDescriptor new];
            samplerDesc.minFilter = MTLSamplerMinMagFilterNearest;
            samplerDesc.magFilter = MTLSamplerMinMagFilterNearest;
            _samplerStates[int(foundation::SamplerType::NEAREST)] = [_device newSamplerStateWithDescriptor:samplerDesc];
            
            samplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
            samplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
            _samplerStates[int(foundation::SamplerType::LINEAR)] = [_device newSamplerStateWithDescriptor:samplerDesc];
            
            MTLDepthStencilDescriptor *depthDesc = [MTLDepthStencilDescriptor new];
            depthDesc.depthCompareFunction = MTLCompareFunctionAlways;
            depthDesc.depthWriteEnabled = NO;
            _depthStates[int(foundation::DepthBehavior::DISABLED)] = [_device newDepthStencilStateWithDescriptor:depthDesc];

            depthDesc.depthCompareFunction = MTLCompareFunctionGreater;
            depthDesc.depthWriteEnabled = NO;
            _depthStates[int(foundation::DepthBehavior::TEST_ONLY)] = [_device newDepthStencilStateWithDescriptor:depthDesc];
            
            depthDesc.depthCompareFunction = MTLCompareFunctionGreater;
            depthDesc.depthWriteEnabled = YES;
            _depthStates[int(foundation::DepthBehavior::TEST_AND_WRITE)] = [_device newDepthStencilStateWithDescriptor:depthDesc];
            
            for (std::uint32_t i = 0; i < CONSTANT_BUFFER_FRAMES_MAX; i++) {
                _constantsBuffers[i] = [_device newBufferWithLength:CONSTANT_BUFFER_OFFSET_MAX options:MTLResourceStorageModeShared];
            }
            
            _currentCommandBuffer = [_commandQueue commandBuffer];
            _platform->logMsg("[RENDER] Initialization : complete");
        }
    }
    
    MetalRendering::~MetalRendering() {}
    
    void MetalRendering::updateFrameConstants(const math::transform3f &vp, const math::transform3f &svp, const math::transform3f &ivp, const math::vector3f &camPos, const math::vector3f &camDir) {
        _frameConstants.plmVPMatrix = vp;
        _frameConstants.stdVPMatrix = svp;
        _frameConstants.invVPMatrix = ivp;
        _frameConstants.cameraPosition.xyz = camPos;
        _frameConstants.cameraDirection.xyz = camDir;
    }
    
    // TODO: dedicate shader generator (DRY)
    //
    RenderShaderPtr MetalRendering::createShader(const char *name, const char *shadersrc, const InputLayout &layout) {
        std::shared_ptr<RenderShader> result;
        util::strstream input(shadersrc, strlen(shadersrc));
        const std::string indent = "    ";
        
        if (_shaderNames.find(name) == _shaderNames.end()) {
            _shaderNames.emplace(name);
        }
        else {
            _platform->logError("[MetalRendering::createShader] shader name '%s' already used\n", name);
        }
        
        static const std::string SEPARATORS = " ;,=-+*/{(\n\t\r";
        static const std::size_t TYPES_COUNT = 14;
        static const std::size_t TYPES_PASS_COUNT = 4;
        static const shaderUtils::ShaderTypeInfo TYPE_SIZE_TABLE[TYPES_COUNT] = {
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
        
        auto formFixedBlock = [&indent](util::strstream &stream, std::string &output) {
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
        };

        auto formVarsBlock = [&indent](util::strstream &stream, std::string &output, std::size_t allowedTypeCount) {
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
        };
        
        std::string nativeShader =
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
        
        std::string functions;
        std::string functionDefines;
        std::string blockName;
        std::uint32_t constBlockLength = 0;
        
        bool completed = true;
        bool fixedBlockDone = false;
        bool constBlockDone = false;
        bool inoutBlockDone = false;
        bool vssrcBlockDone = false;
        bool fssrcBlockDone = false;
        int  fndefBlockCount = 0;
        
        while (input >> blockName) {
            if (fixedBlockDone == false && blockName == "fixed" && (input >> util::sequence("{"))) {
                if (formFixedBlock(input, nativeShader) == false) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' has ill-formed 'fixed' block\n", name);
                    completed = false;
                    break;
                }

                fixedBlockDone = true;
                continue;
            }
            if (constBlockDone == false && blockName == "const" && (input >> util::sequence("{"))) {
                nativeShader += "struct _Constants {\n";
                
                if ((constBlockLength = formVarsBlock(input, nativeShader, TYPES_PASS_COUNT)) == 0) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' has ill-formed 'const' block\n", name);
                    completed = false;
                    break;
                }
                nativeShader += "};\n\n";
                constBlockDone = true;
                continue;
            }
            if (inoutBlockDone == false && blockName == "inout" && (input >> util::sequence("{"))) {
                nativeShader += "struct _InOut {\n    float4 position [[position]];\n";
                
                if (formVarsBlock(input, nativeShader, TYPES_COUNT) == 0) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' has ill-formed 'inout' block\n", name);
                    completed = false;
                    break;
                }
                nativeShader += "};\n\n";
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
                        functions += "    " + funcReturnType + " " + funcName + "(" + funcSignature + ") {\n";
                        functions += codeBlock;
                        functions += "    }\n\n";
                        
                        functionDefines += "#define " + funcName + " _fn." + funcName + "\n";
                        fndefBlockCount++;
                    }
                    else {
                        _platform->logError("[MetalRendering::createShader] shader '%s' has uncompleted 'fndef' block\n", name);
                        completed = false;
                        break;
                    }
                }
                else {
                    _platform->logError("[MetalRendering::createShader] shader '%s' has invalid 'fndef' block\n", name);
                    completed = false;
                    break;
                }
                continue;
            }
            if (vssrcBlockDone == false && blockName == "vssrc" && (input >> util::sequence("{"))) {
                if (constBlockDone == false) {
                    constBlockDone = true;
                    nativeShader += "struct _Constants {\n};\n\n";
                }
                if (inoutBlockDone == false) {
                    inoutBlockDone = true;
                    nativeShader += "struct _InOut {\n    float4 position [[position]];\n};\n\n";
                }

                std::size_t index = 0;
                std::uint32_t offset = 0;
                std::string variables;
                
                auto formInput = [&](const std::vector<InputLayout::Attribute> &desc, std::uint32_t bufferIndex, const char *prefix, const char *assign, std::string &output) {
                    offset = 0;
                    
                    for (const auto &item : desc) {
                        NativeFormat &fmt = g_formatConversionTable[int(item.format)];
                        variables += indent + "const " + fmt.nativeUnpackedName + " " + prefix + item.name + " = " + std::string(assign) + item.name + ";\n";
                        output += indent + fmt.nativePackedTypeName + " " + item.name + ";\n";
                        offset += fmt.size;
                        index++;
                    }
                };
                
                shaderUtils::replace(functions, "const_", "constants.", SEPARATORS);
                shaderUtils::replace(functions, "frame_", "framedata.", SEPARATORS);
                
                nativeShader +=
                    "struct _FN {\n    "
                    "constant const _FrameData &framedata;\n    "
                    "constant const _Constants &constants;\n    "
                    "thread const texture2d<float> &_texture0;\n    "
                    "thread const texture2d<float> &_texture1;\n    "
                    "thread const texture2d<float> &_texture2;\n    "
                    "thread const texture2d<float> &_texture3;\n    "
                    "thread const sampler &_sampler;\n\n";
                    
                nativeShader += functions;
                nativeShader += "};\n";
                nativeShader += functionDefines;
                nativeShader += "\nstruct _VSVertexIn {\n";
                formInput(layout.attributes, 0, "vertex_", "vertices[vertex_ID].", nativeShader);
                nativeShader += "};\n\nvertex _InOut main_vertex(\n";
                
                if (layout.repeat > 1) {
                    nativeShader += "    unsigned int _r_ID [[vertex_id]],\n    unsigned int _i_ID [[instance_id]],\n";
                }
                else {
                    nativeShader += "    unsigned int _v_ID [[vertex_id]],\n    unsigned int _i_ID [[instance_id]],\n";
                }
                
                nativeShader +=
                    "    constant _FrameData &framedata [[buffer(0)]],\n"
                    "    constant _Constants &constants [[buffer(1)]],\n"
                    "    device const _VSVertexIn *vertices [[buffer(2)]],\n"
                    "    constant uint &_vertexCount [[buffer(";
                    
                nativeShader += std::to_string(VERTEX_IN_VERTEX_COUNT);
                nativeShader += ")]],\n"
                    "    sampler _sampler [[sampler(0)]],\n"
                    "    texture2d<float> _texture0 [[texture(0)]],\n"
                    "    texture2d<float> _texture1 [[texture(1)]],\n"
                    "    texture2d<float> _texture2 [[texture(2)]],\n"
                    "    texture2d<float> _texture3 [[texture(3)]])\n{\n";
                
                if (layout.repeat > 1) {
                    nativeShader += "    const int repeat_ID = _r_ID;\n";
                    nativeShader += "    const int vertex_ID = _i_ID % _vertexCount;\n";
                    nativeShader += "    const int instance_ID = _i_ID / _vertexCount;\n";
                }
                else {
                    nativeShader += "    const int repeat_ID = 0;\n";
                    nativeShader += "    const int vertex_ID = _v_ID;\n";
                    nativeShader += "    const int instance_ID = _i_ID;\n";
                }
                
                nativeShader += variables;
                nativeShader += "    _FN _fn {framedata, constants, _texture0, _texture1, _texture2, _texture3, _sampler};\n    _InOut output;\n\n";

                std::string codeBlock;
                
                if (shaderUtils::formCodeBlock(indent, input, codeBlock) == false) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' has uncompleted 'vssrc' block\n", name);
                    completed = false;
                    break;
                }
                
                shaderUtils::replace(codeBlock, "output_", "output.", SEPARATORS);
                shaderUtils::replace(codeBlock, "const_", "constants.", SEPARATORS);
                shaderUtils::replace(codeBlock, "frame_", "framedata.", SEPARATORS);
                
                nativeShader += codeBlock;
                nativeShader += "\n    (void)repeat_ID; (void)vertex_ID; (void)instance_ID;(void)_fn;\n";
                nativeShader += "    return output;\n}\n\n";
                vssrcBlockDone = true;
                continue;
            }
            if (fssrcBlockDone == false && blockName == "fssrc" && (input >> util::sequence("{"))) {
                if (vssrcBlockDone == false) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' : 'vssrc' block must be defined before 'fssrc'\n", name);
                    completed = false;
                    break;
                }
                
                nativeShader +=
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

                nativeShader += "    constant _FrameData &framedata [[buffer(0)]],\n";
                nativeShader += "    constant _Constants &constants [[buffer(1)]])\n{\n";
                nativeShader += "    float2 fragment_coord = input.position.xy / framedata.rtBounds.xy;\n";
                nativeShader += "    float4 output_color[4] = {};\n    _FN _fn {framedata, constants, _texture0, _texture1, _texture2, _texture3, _sampler};\n\n";
                
                std::string codeBlock;
                
                if (shaderUtils::formCodeBlock(indent, input, codeBlock) == false) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' has uncompleted 'fssrc' block\n", name);
                    completed = false;
                    break;
                }
                
                shaderUtils::replace(codeBlock, "const_", "constants.", SEPARATORS);
                shaderUtils::replace(codeBlock, "frame_", "framedata.", SEPARATORS);
                shaderUtils::replace(codeBlock, "input_", "input.", SEPARATORS);
                
                nativeShader += codeBlock;
                nativeShader += "\n    return _Output {output_color[0], output_color[1], output_color[2], output_color[3]};\n";
                nativeShader += "    (void)fragment_coord;(void)_fn;\n";
                nativeShader += "}\n";
                fssrcBlockDone = true;
                continue;
            }
            
            _platform->logError("[MetalRendering::createShader] shader '%s' has unexpected '%s' block\n", name, blockName.data());
        }
        
        nativeShader = shaderUtils::makeLines(nativeShader);
        //printf("---------- begin ----------\n%s\n----------- end -----------\n", nativeShader.data());
        
        if (completed && vssrcBlockDone && fssrcBlockDone) {
            @autoreleasepool {
                NSError *nsError = nil;
                MTLCompileOptions* compileOptions = [MTLCompileOptions new];
                compileOptions.languageVersion = MTLLanguageVersion2_0;
                compileOptions.fastMathEnabled = true;
                
                id<MTLLibrary> library = [_device newLibraryWithSource:[NSString stringWithUTF8String:nativeShader.data()] options:compileOptions error:&nsError];
                
                if (library) {
                    result = std::make_shared<MetalShader>(name, layout, library, constBlockLength);
                }
                else {
                    const char *errorDesc = [[nsError localizedDescription] UTF8String];
                    _platform->logError("[MetalRendering::createShader] '%s' generated code:\n--------------------\n%s\n--------------------\n%s\n", name, nativeShader.data(), errorDesc);
                }
            }
        }
        else if(vssrcBlockDone == false) {
            _platform->logError("[MetalRendering::createShader] shader '%s' missing 'vssrc' block\n", name);
        }
        else if(fssrcBlockDone == false) {
            _platform->logError("[MetalRendering::createShader] shader '%s' missing 'fssrc' block\n", name);
        }

        return result;
    }
    
    namespace {
        static MTLPixelFormat nativeTextureFormat[] = {
            MTLPixelFormatR8Unorm,
            MTLPixelFormatR16Float,
            MTLPixelFormatR32Float,
            MTLPixelFormatRG8Unorm,
            MTLPixelFormatRG16Float,
            MTLPixelFormatRG32Float,
            MTLPixelFormatRGBA8Unorm,
            MTLPixelFormatRGBA16Float,
            MTLPixelFormatRGBA32Float
        };
    }
        
    RenderTexturePtr MetalRendering::createTexture(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, const std::initializer_list<const void *> &mipsData) {
        auto getTexture2DPitch = [](RenderTextureFormat fmt, std::size_t width) {
            switch (fmt) {
            case RenderTextureFormat::R8UN:
                return width;
            case RenderTextureFormat::R16F:
                return width * sizeof(std::uint16_t);
            case RenderTextureFormat::R32F:
                return width * sizeof(float);
            case RenderTextureFormat::RG8UN:
                return width * 2;
            case RenderTextureFormat::RG16F:
                return width * 2 * sizeof(std::uint16_t);
            case RenderTextureFormat::RG32F:
                return width * 2 * sizeof(float);
            case RenderTextureFormat::RGBA8UN:
                return width * 4;
            case RenderTextureFormat::RGBA16F:
                return width * 4 * sizeof(std::uint16_t);
            case RenderTextureFormat::RGBA32F:
                return width * 4 * sizeof(float);
            default:
                return std::size_t(0);
            }
        };
        
        @autoreleasepool {
            size_t mipmapLevelCount = std::max(size_t(1), mipsData.size());
            
            MTLTextureDescriptor *desc = [MTLTextureDescriptor new];
            desc.pixelFormat = nativeTextureFormat[int(format)];
            desc.width = w;
            desc.height = h;
            desc.mipmapLevelCount = mipmapLevelCount;
            desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
            id<MTLTexture> texture = [_device newTextureWithDescriptor:desc];
            
            unsigned counter = 0;
            for (auto &item : mipsData) {
                MTLRegion region = {{ 0, 0, 0 }, {w >> counter, h >> counter, 1}};
                [texture replaceRegion:region mipmapLevel:counter withBytes:item bytesPerRow:getTexture2DPitch(format, w)];
            }
            
            return std::make_shared<MetalTexture>(texture, format, w, h, mipmapLevelCount);
        }
    }
    
    RenderTargetPtr MetalRendering::createRenderTarget(RenderTextureFormat format, std::uint32_t count, std::uint32_t w, std::uint32_t h, bool withZBuffer) {
        @autoreleasepool {
            MTLTextureDescriptor *desc = [MTLTextureDescriptor new];
            id<MTLTexture> targets[RenderTarget::MAX_TEXTURE_COUNT] = {nullptr, nullptr, nullptr, nullptr};
            
            desc.pixelFormat = nativeTextureFormat[int(format)];
            desc.width = w;
            desc.height = h;
            desc.mipmapLevelCount = 1;
            desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite | MTLTextureUsageRenderTarget;
            desc.storageMode = MTLStorageModePrivate;

            for (unsigned i = 0; i < count; i++) {
                targets[i] = [_device newTextureWithDescriptor:desc];
            }

            id<MTLTexture> depth = nil;
            
            if (withZBuffer) {
                desc.pixelFormat = MTLPixelFormatDepth32Float;
                desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
                desc.storageMode = MTLStorageModePrivate;
                desc.width = w;
                desc.height = h;
                desc.mipmapLevelCount = 1;
                depth = [_device newTextureWithDescriptor:desc];
            }
            
            return std::make_shared<MetalTarget>(targets, count, depth, format, w, h);
        }
    }
    
    RenderDataPtr MetalRendering::createData(const void *data, const InputLayout &layout, std::uint32_t vcnt, const std::uint32_t *indexes, std::uint32_t icnt) {
        std::uint32_t stride = 0;
        for (const InputLayout::Attribute &item : layout.attributes) {
            stride += g_formatConversionTable[int(item.format)].size;
        }
        if (indexes && layout.repeat > 1) {
            _platform->logError("[MetalRendering::createData] Vertex repeat is incompatible with indexed data");
            return nullptr;
        }
        @autoreleasepool {
            id<MTLBuffer> vbuffer = [_device newBufferWithBytes:data length:(vcnt * stride) options:MTLResourceStorageModeShared];
            id<MTLBuffer> ibuffer = nil;
            
            if (indexes) {
                ibuffer = [_device newBufferWithBytes:indexes length:(icnt * sizeof(std::uint32_t)) options:MTLResourceStorageModeShared];
            }
            
            return std::make_shared<MetalData>(vbuffer, ibuffer, vcnt, icnt, stride);
        }
    }
    
    float MetalRendering::getBackBufferWidth() const {
        return _platform->getScreenWidth();
    }
    
    float MetalRendering::getBackBufferHeight() const {
        return _platform->getScreenHeight();
    }
    
    math::transform3f MetalRendering::getStdVPMatrix() const {
        return _frameConstants.stdVPMatrix;
    }
    
    void MetalRendering::forTarget(const RenderTargetPtr &target, const RenderTexturePtr &depth, const std::optional<math::color> &rgba, util::callback<void(RenderingInterface &)> &&pass) {
        if (_view == nil) {
            _view = (__bridge MTKView *)_platform->attachNativeRenderingContext((__bridge void *)_device);
        }
        if (_view) {
            MTLRenderPassDescriptor *renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
            MTLViewport viewPort {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
            
            if (depth) {
                renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionLoad;
                renderPassDescriptor.depthAttachment.texture = static_cast<const MetalTexBase *>(depth.get())->getNativeTexture();
            }
            else {
                renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
                renderPassDescriptor.depthAttachment.clearDepth = 0.0;
            }
            
            MTLLoadAction colorClearAction = MTLLoadActionLoad;
            MTLClearColor colorClearValue = MTLClearColorMake(0, 0, 0, 0);
            
            if (rgba.has_value()) {
                colorClearAction = MTLLoadActionClear;
                colorClearValue = MTLClearColorMake(rgba->r, rgba->g, rgba->b, rgba->a);
            }
            
            if (target) {
                for (std::uint32_t i = 0; i < target->getTextureCount(); i++) {
                    renderPassDescriptor.colorAttachments[i].texture = static_cast<const MetalTexBase *>(target->getTexture(i).get())->getNativeTexture();
                    renderPassDescriptor.colorAttachments[i].storeAction = MTLStoreActionStore;
                    renderPassDescriptor.colorAttachments[i].loadAction = colorClearAction;
                    renderPassDescriptor.colorAttachments[i].clearColor = colorClearValue;
                }
                
                renderPassDescriptor.depthAttachment.texture = static_cast<const MetalTexBase *>(target->getDepth().get())->getNativeTexture();
                renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
                
                viewPort.width = _frameConstants.rtBounds.x = float(target->getWidth());
                viewPort.height = _frameConstants.rtBounds.y = float(target->getHeight());
            }
            else {
                renderPassDescriptor.colorAttachments[0].texture = _view.currentDrawable.texture;
                renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
                renderPassDescriptor.colorAttachments[0].loadAction = colorClearAction;
                renderPassDescriptor.colorAttachments[0].clearColor = colorClearValue;
                renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
                
                if (depth == nullptr) {
                    renderPassDescriptor.depthAttachment.texture = _view.depthStencilTexture;
                }
                
                viewPort.width = _frameConstants.rtBounds.x = _platform->getScreenWidth();
                viewPort.height = _frameConstants.rtBounds.y = _platform->getScreenHeight();
            }
            
            _finishRenderCommandEncoder();
            _currentRenderCommandEncoder = [_currentCommandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
            _currentTarget = target;
            _isForTarget = true;
            
            _appendConstantBuffer(&_frameConstants, sizeof(FrameConstants), FRAME_CONST_BINDING_INDEX);
            [_currentRenderCommandEncoder setViewport:viewPort];
            
            pass(*this);
            
            [_currentRenderCommandEncoder endEncoding];
            _currentRenderCommandEncoder = nil;
            _currentTarget = nullptr;
            _isForTarget = false;
        }
    }
    
    void MetalRendering::applyShader(const RenderShaderPtr &shader, foundation::RenderTopology topology, BlendType blendType, DepthBehavior depthBehavior) {
        if (_isForTarget) {
            if (_currentRenderCommandEncoder == nil) {
                MTLRenderPassDescriptor *renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
                
                renderPassDescriptor.colorAttachments[0].texture = _view.currentDrawable.texture;
                renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
                renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
                renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
                renderPassDescriptor.depthAttachment.texture = _view.depthStencilTexture;
                renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
                renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
                renderPassDescriptor.depthAttachment.clearDepth = 0.0f;
                
                _currentRenderCommandEncoder = [_currentCommandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
                
                MTLViewport viewPort {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
                viewPort.width = _frameConstants.rtBounds.x = _platform->getScreenWidth();
                viewPort.height = _frameConstants.rtBounds.y = _platform->getScreenHeight();
                _appendConstantBuffer(&_frameConstants, sizeof(FrameConstants), FRAME_CONST_BINDING_INDEX);
                [_currentRenderCommandEncoder setViewport:viewPort];
            }
            
            const MetalShader *platformShader = static_cast<const MetalShader *>(shader.get());
            const MetalTarget *platformTarget = static_cast<const MetalTarget *>(_currentTarget.get());
            
            if (platformShader) {
                id<MTLRenderPipelineState> state = nil;
                const std::string &stateName = generateRenderPipelineStateName(platformShader, platformTarget, blendType);
                
                auto index = _renderPipelineStates.find(stateName);
                if (index == _renderPipelineStates.end()) {
                    MTLRenderPipelineDescriptor *desc = [MTLRenderPipelineDescriptor new];
                    desc.vertexFunction = platformShader->getVertexShader();
                    desc.fragmentFunction = platformShader->getFragmentShader();
                    
                    if (platformTarget) {
                        for (std::uint32_t i = 0; i < platformTarget->getTextureCount(); i++) {
                            desc.colorAttachments[i].pixelFormat = nativeTextureFormat[int(platformTarget->getTexture(0)->getFormat())];
                            initializeBlendOptions[int(blendType)](desc.colorAttachments[i]);
                        }
                        if (platformTarget->getDepth()) {
                            desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
                        }
                    }
                    else {
                        desc.colorAttachments[0].pixelFormat = _view.colorPixelFormat;
                        desc.depthAttachmentPixelFormat = _view.depthStencilPixelFormat;
                        initializeBlendOptions[int(blendType)](desc.colorAttachments[0]);
                    }
                    
                    NSError *error;
                    if ((state = [_device newRenderPipelineStateWithDescriptor:desc error:&error]) != nil) {
                        _renderPipelineStates.emplace(stateName, state);
                    }
                    else {
                        _platform->logError("[MetalRendering::applyState] %s\n", [[error localizedDescription] UTF8String]);
                    }
                }
                else {
                    state = index->second;
                }
                
                if (state) {
                    [_currentRenderCommandEncoder setRenderPipelineState:state];
                    [_currentRenderCommandEncoder setDepthStencilState:_depthStates[int(depthBehavior)]];
                    
                    _currentTopology = topology;
                    _currentShader = shader;
                }
            }
        }
    }
        
    void MetalRendering::applyShaderConstants(const void *constants) {
        if (_currentRenderCommandEncoder) {
            const MetalShader *platformShader = static_cast<const MetalShader *>(_currentShader.get());
            const std::uint32_t constBufferLength = platformShader->getConstBufferLength();
            
            if (platformShader && constBufferLength) {
                _appendConstantBuffer(constants, constBufferLength, DRAW_CONST_BINDING_INDEX);
            }
        }
    }
    
    void MetalRendering::applyTextures(const std::initializer_list<std::pair<const RenderTexturePtr, SamplerType>> &textures) {
        if (_currentRenderCommandEncoder && _currentShader) {
            if (textures.size() <= MAX_TEXTURES) {
                const NSRange range {0, textures.size()};
                __unsafe_unretained id<MTLTexture> texarray[MAX_TEXTURES] = {nil};
                __unsafe_unretained id<MTLSamplerState> smarray[MAX_TEXTURES] = {nil};
                
                std::uint32_t index = 0;
                for (auto &item : textures) {
                    texarray[index] = item.first ? static_cast<const MetalTexBase *>(item.first.get())->getNativeTexture() : nil;
                    smarray[index] = _samplerStates[int(item.second)];
                    index++;
                }
                
                [_currentRenderCommandEncoder setFragmentSamplerStates:smarray withRange:range];
                [_currentRenderCommandEncoder setFragmentTextures:texarray withRange:range];
                [_currentRenderCommandEncoder setVertexSamplerStates:smarray withRange:range];
                [_currentRenderCommandEncoder setVertexTextures:texarray withRange:range];
            }
        }
    }
    
    void MetalRendering::draw(std::uint32_t vertexCount) {
        if (_currentRenderCommandEncoder && _currentShader) {
            const InputLayout &layout = _currentShader->getInputLayout();
            const MTLPrimitiveType topology = g_topologies[int(_currentTopology)];
            
            [_currentRenderCommandEncoder setVertexBuffer:nil offset:0 atIndex:VERTEX_IN_BINDING_START];
            
            if (layout.repeat > 1) {
                [_currentRenderCommandEncoder setVertexBytes:&vertexCount length:sizeof(std::uint32_t) atIndex:VERTEX_IN_VERTEX_COUNT];
                [_currentRenderCommandEncoder drawPrimitives:topology vertexStart:0 vertexCount:layout.repeat instanceCount:vertexCount];
            }
            else {
                [_currentRenderCommandEncoder drawPrimitives:topology vertexStart:0 vertexCount:vertexCount];
            }
        }
    }
    
    void MetalRendering::draw(const RenderDataPtr &inputData, std::uint32_t instanceCount) {
        if (_currentRenderCommandEncoder && _currentShader) {
            const InputLayout &layout = _currentShader->getInputLayout();
            const MetalData *implData = static_cast<const MetalData *>(inputData.get());
            const MTLPrimitiveType topology = g_topologies[int(_currentTopology)];
            
            std::uint32_t vcnt = 1;
            std::uint32_t icnt = 0;
            id<MTLBuffer> vbuffer = nil;
            id<MTLBuffer> ibuffer = nil;
            
            if (implData) {
                vcnt = implData->getVertexCount();
                icnt = implData->getIndexCount();
                vbuffer = implData->getVertexes();
                ibuffer = implData->getIndexes();
            }

            //[_currentRenderCommandEncoder setTriangleFillMode:MTLTriangleFillModeLines];
            [_currentRenderCommandEncoder setVertexBuffer:vbuffer offset:0 atIndex:VERTEX_IN_BINDING_START];

            if (layout.repeat > 1) {
                [_currentRenderCommandEncoder setVertexBytes:&vcnt length:sizeof(std::uint32_t) atIndex:VERTEX_IN_VERTEX_COUNT];
                [_currentRenderCommandEncoder drawPrimitives:topology vertexStart:0 vertexCount:layout.repeat instanceCount:vcnt * instanceCount];
            }
            else {
                if (ibuffer) {
                    [_currentRenderCommandEncoder drawIndexedPrimitives:topology indexCount:icnt indexType:MTLIndexTypeUInt32 indexBuffer:ibuffer indexBufferOffset:0 instanceCount:instanceCount];
                }
                else {
                    [_currentRenderCommandEncoder drawPrimitives:topology vertexStart:0 vertexCount:vcnt instanceCount:instanceCount];
                }
            }
        }
    }
        
    void MetalRendering::presentFrame() {
        _finishRenderCommandEncoder();
        
        if (_view && _currentCommandBuffer) {
            MetalRendering *m = this;
            
            [_currentCommandBuffer presentDrawable:_view.currentDrawable];
            [_currentCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
                dispatch_semaphore_signal(m->_constBufferSemaphore);
            }];
            [_currentCommandBuffer commit];
            _currentCommandBuffer = [_commandQueue commandBuffer];
            
            _constantsBuffersIndex = (_constantsBuffersIndex + 1) % CONSTANT_BUFFER_FRAMES_MAX;
            _constantsBufferOffset = 0;
            dispatch_semaphore_wait(_constBufferSemaphore, DISPATCH_TIME_FOREVER);
        }
    }
    
    void MetalRendering::_appendConstantBuffer(const void *buffer, std::uint32_t size, std::uint32_t index) {
        std::uint32_t roundedSize = roundTo256(size);
        
        if (_constantsBufferOffset + roundedSize < CONSTANT_BUFFER_OFFSET_MAX) {
            std::uint8_t *constantsMemory = static_cast<std::uint8_t *>([_constantsBuffers[_constantsBuffersIndex] contents]);
            std::memcpy(constantsMemory + _constantsBufferOffset, buffer, size);
            
            [_currentRenderCommandEncoder setVertexBuffer:_constantsBuffers[_constantsBuffersIndex] offset:_constantsBufferOffset atIndex:index];
            [_currentRenderCommandEncoder setFragmentBuffer:_constantsBuffers[_constantsBuffersIndex] offset:_constantsBufferOffset atIndex:index];
            
            _constantsBufferOffset += roundedSize;
            return true;
        }
        else {
            _platform->logError("[MetalRendering::_appendConstantBuffer] Out of constants buffer length\n");
        }
        return false;
    }
    
    void MetalRendering::_finishRenderCommandEncoder() {
        if (_currentRenderCommandEncoder) {
            [_currentRenderCommandEncoder endEncoding];
            _currentRenderCommandEncoder = nil;
            _currentShader = nullptr;
        }
    }
}

namespace foundation {
    std::shared_ptr<RenderingInterface> RenderingInterface::instance(const std::shared_ptr<PlatformInterface> &platform) {
        return std::make_shared<MetalRendering>(platform);
    }
}

#endif // PLATFORM_IOS
