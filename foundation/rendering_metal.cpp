
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
    
    std::uint32_t roundTo256(std::uint32_t value) {
        std::uint32_t result = ((value - std::uint32_t(1)) & ~std::uint32_t(255)) + 256;
        return result;
    }
    
    std::string generateRenderPipelineStateName(const foundation::MetalShader &shader, const foundation::RenderPassConfig &passConfig) {
        std::string result = "r0_b0_" + shader.getName();
        result[1] = passConfig.target ? '0' + int(passConfig.target->getFormat()) : 'X';
        result[4] += int(passConfig.blendType);
        return result;
    }
    
    void initializeBlendOptions(MTLRenderPipelineColorAttachmentDescriptor *target, const foundation::RenderPassConfig &cfg) {
        switch (cfg.blendType) {
            case foundation::BlendType::DISABLED:
            {
                target.blendingEnabled = false;
                break;
            }
            case foundation::BlendType::MIXING:
            {
                target.blendingEnabled = true;
                target.alphaBlendOperation = MTLBlendOperationAdd;
                target.rgbBlendOperation = MTLBlendOperationAdd;
                target.sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
                target.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
                target.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
                target.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
                break;
            }
            case foundation::BlendType::ADDITIVE:
            {
                target.blendingEnabled = YES;
                target.alphaBlendOperation = MTLBlendOperationAdd;
                target.rgbBlendOperation = MTLBlendOperationAdd;
                target.sourceAlphaBlendFactor = MTLBlendFactorOne;
                target.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
                target.destinationAlphaBlendFactor = MTLBlendFactorOne;
                target.destinationRGBBlendFactor = MTLBlendFactorOne;
                break;
            }
            case foundation::BlendType::AGREGATION:
            {
                target.blendingEnabled = YES;
                target.alphaBlendOperation = MTLBlendOperationAdd;
                target.rgbBlendOperation = MTLBlendOperationAdd;
                target.sourceAlphaBlendFactor = MTLBlendFactorOne;
                target.sourceRGBBlendFactor = MTLBlendFactorOne;
                target.destinationAlphaBlendFactor = MTLBlendFactorOne;
                target.destinationRGBBlendFactor = MTLBlendFactorOne;
                break;
            }
            case foundation::BlendType::MAXVALUE:
            {
                target.blendingEnabled = YES;
                target.alphaBlendOperation = MTLBlendOperationMax;
                target.rgbBlendOperation = MTLBlendOperationMax;
                target.sourceAlphaBlendFactor = MTLBlendFactorOne;
                target.sourceRGBBlendFactor = MTLBlendFactorOne;
                target.destinationAlphaBlendFactor = MTLBlendFactorOne;
                target.destinationRGBBlendFactor = MTLBlendFactorOne;
                break;
            }
            case foundation::BlendType::MINVALUE:
            {
                target.blendingEnabled = YES;
                target.alphaBlendOperation = MTLBlendOperationMin;
                target.rgbBlendOperation = MTLBlendOperationMin;
                target.sourceAlphaBlendFactor = MTLBlendFactorOne;
                target.sourceRGBBlendFactor = MTLBlendFactorOne;
                target.destinationAlphaBlendFactor = MTLBlendFactorOne;
                target.destinationRGBBlendFactor = MTLBlendFactorOne;
                break;
            }
            default:
            break;
        }
    }
    
    struct NativeFormat {
        const char *nativeTypeName;
        std::uint32_t size;
        MTLVertexFormat nativeFormat;
    }
    g_formatConversionTable[] = { // index is RenderShaderInputFormat value
        {"packed_half2",         4,  MTLVertexFormatHalf2},
        {"packed_half4",         8,  MTLVertexFormatHalf4},
        {"packed_float",         4,  MTLVertexFormatFloat},
        {"packed_float2",        8,  MTLVertexFormatFloat2},
        {"packed_float3",        12, MTLVertexFormatFloat3},
        {"packed_float4",        16, MTLVertexFormatFloat4},
        {"packed_short2",        4,  MTLVertexFormatShort2},
        {"packed_short4",        8,  MTLVertexFormatShort4},
        {"packed_ushort2",       4,  MTLVertexFormatUShort2},
        {"packed_ushort4",       8,  MTLVertexFormatUShort4},
        {"rg16snorm<float2>",    4,  MTLVertexFormatShort2Normalized},
        {"rgba16snorm<float4>",  8,  MTLVertexFormatShort4Normalized},
        {"rg16unorm<float2>",    4,  MTLVertexFormatUShort2Normalized},
        {"rgba16unorm<float4>",  8,  MTLVertexFormatUShort4Normalized},
        {"packed_uchar4",        4,  MTLVertexFormatUChar4},
        {"rgba8unorm<float4>",   4,  MTLVertexFormatUChar4Normalized},
        {"packed_int",           4,  MTLVertexFormatInt},
        {"packed_int2",          8,  MTLVertexFormatInt2},
        {"packed_int3",          12, MTLVertexFormatInt3},
        {"packed_int4",          16, MTLVertexFormatInt4},
        {"packed_uint",          4,  MTLVertexFormatUInt},
        {"packed_uint2",         8,  MTLVertexFormatUInt2},
        {"packed_uint3",         12, MTLVertexFormatUInt3},
        {"packed_uint4",         16, MTLVertexFormatUInt4}
    };
}

namespace foundation {
    MetalShader::MetalShader(const std::string &name, InputLayout &&layout, id<MTLLibrary> library, std::uint32_t constBufferLength)
        : _name(name)
        , _layout(std::move(layout))
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
    MetalTarget::MetalTarget(__strong id<MTLTexture> *targets, unsigned count, id<MTLTexture> depth, RenderTextureFormat fmt, std::uint32_t w, std::uint32_t h)
        : _depth(depth)
        , _format(fmt)
        , _depthFormat(depth ? depth.pixelFormat : MTLPixelFormatInvalid)
        , _count(count)
        , _width(w)
        , _height(h)
    {
        for (unsigned i = 0; i < count; i++) {
            _textures[i] = std::make_shared<RTTexture>(*this, targets[i]);
        }
    }
    
    MetalTarget::~MetalTarget() {
        for (unsigned i = 0; i < _count; i++) {
            _textures[i] = nil;
        }
        
        _depth = nil;
    }
    
    std::uint32_t MetalTarget::getWidth() const {
        return _width;
    }
    
    std::uint32_t MetalTarget::getHeight() const {
        return _height;
    }
    
    RenderTextureFormat MetalTarget::getFormat() const {
        return _format;
    }
    
    bool MetalTarget::hasDepthBuffer() const {
        return _depth != nil;
    }

    std::uint32_t MetalTarget::getTextureCount() const {
        return _count;
    }
    
    const std::shared_ptr<RenderTexture> &MetalTarget::getTexture(unsigned index) const {
        return _textures[index];
    }

    MTLPixelFormat MetalTarget::getDepthFormat() const {
        return _depthFormat;
    }
}

namespace foundation {
    MetalData::MetalData(id<MTLBuffer> buffer, std::uint32_t stride) : _buffer(buffer), _count(std::uint32_t(_buffer.length)), _stride(stride) {
    
    }
    
    MetalData::~MetalData() {
        _buffer = nil;
    }
    
    std::uint32_t MetalData::getCount() const {
        return _count;
    }
    
    std::uint32_t MetalData::getStride() const {
        return _stride;
    }
    
    id<MTLBuffer> MetalData::get() const {
        return _buffer;
    }
}

namespace foundation {
    MetalRendering::MetalRendering(const std::shared_ptr<PlatformInterface> &platform) : _platform(platform) {
        @autoreleasepool {
            _platform->logMsg("[RENDER] Initialization : Metal");
            
            _device = MTLCreateSystemDefaultDevice();
            _commandQueue = [_device newCommandQueue];
            _constBufferSemaphore = dispatch_semaphore_create(CONSTANT_BUFFER_FRAMES_MAX);
            
            MTLDepthStencilDescriptor *depthDesc = [MTLDepthStencilDescriptor new];
            depthDesc.depthCompareFunction = MTLCompareFunctionGreater;
            depthDesc.depthWriteEnabled = NO;
            _zBehaviorStates[int(foundation::ZBehaviorType::TEST_ONLY)] = [_device newDepthStencilStateWithDescriptor:depthDesc];
            
            depthDesc.depthCompareFunction = MTLCompareFunctionGreater;
            depthDesc.depthWriteEnabled = YES;
            _zBehaviorStates[int(foundation::ZBehaviorType::TEST_AND_WRITE)] = [_device newDepthStencilStateWithDescriptor:depthDesc];
            
            for (std::uint32_t i = 0; i < CONSTANT_BUFFER_FRAMES_MAX; i++) {
                _constantsBuffers[i] = [_device newBufferWithLength:CONSTANT_BUFFER_OFFSET_MAX options:MTLResourceStorageModeShared];
            }
            
            _platform->logMsg("[RENDER] Initialization : complete");
        }
    }
    
    MetalRendering::~MetalRendering() {}
    
    void MetalRendering::updateFrameConstants(const math::transform3f &view, const math::transform3f &proj, const math::vector3f &camPos, const math::vector3f &camDir) {
        _frameConstants.viewMatrix = view;
        _frameConstants.projMatrix = proj;
        _frameConstants.cameraPosition.xyz = camPos;
        _frameConstants.cameraDirection.xyz = camDir;
    }
    
    math::vector2f MetalRendering::getScreenCoordinates(const math::vector3f &worldPosition) {
        const math::transform3f vp = _frameConstants.viewMatrix * _frameConstants.projMatrix;
        math::vector4f tpos = math::vector4f(worldPosition, 1.0f);
        
        tpos = tpos.transformed(vp);
        tpos.x /= tpos.w;
        tpos.y /= tpos.w;
        tpos.z /= tpos.w;
        
        math::vector2f result = math::vector2f(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
        
        if (tpos.z > 0.0f) {
            result.x = (tpos.x + 1.0f) * 0.5f * _platform->getScreenWidth();
            result.y = (1.0f - tpos.y) * 0.5f * _platform->getScreenHeight();
        }
        
        return result;
    }
    
    math::vector3f MetalRendering::getWorldDirection(const math::vector2f &screenPosition) {
        const math::transform3f inv = (_frameConstants.viewMatrix * _frameConstants.projMatrix).inverted();
        
        float relScreenPosX = 2.0f * screenPosition.x / _platform->getScreenWidth() - 1.0f;
        float relScreenPosY = 1.0f - 2.0f * screenPosition.y / _platform->getScreenHeight();
        
        const math::vector4f worldPos = math::vector4f(relScreenPosX, relScreenPosY, 0.0f, 1.0f).transformed(inv);
        return math::vector3f(worldPos.x / worldPos.w, worldPos.y / worldPos.w, worldPos.z / worldPos.w).normalized();
    }
    
    RenderShaderPtr MetalRendering::createShader(const char *name, const char *shadersrc, InputLayout &&layout) {
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
            "#define _fract(a) fract(a)\n"
            "#define _floor(a) floor(a)\n"
            "#define _clamp(a) clamp(a, 0.0, 1.0)\n"
            "#define _norm(a) normalize(a)\n"
            "#define _lerp(a, b, k) mix((a), (b), k)\n"
            "#define _step(k, a) step((k), (a))\n"
            "#define _smooth(a, b, k) smoothstep((a), (b), (k))\n"
            "#define _min(a, b) min((a), (b))\n"
            "#define _max(a, b) max((a), (b))\n"
            "#define _tex2nearest(i, a) _texture##i.sample(_nearestSampler, a)\n"
            "#define _tex2linear(i, a) _texture##i.sample(_linearSampler, a)\n"
            "#define _tex2raw(i, a) _texture##i.read(ushort2(a.x * (_texture##i.get_width() - 1), a.y * (_texture##i.get_height() - 1)), 0)\n"
            "\n"
            "const sampler _nearestSampler(mag_filter::nearest, min_filter::nearest);\n"
            "const sampler _linearSampler(mag_filter::linear, min_filter::linear);\n"
            "\n"
            "struct _FrameData {\n"
            "    float4x4 viewMatrix;\n"
            "    float4x4 projMatrix;\n"
            "    float4 cameraPosition;\n"
            "    float4 cameraDirection;\n"
            "    float4 rtBounds;\n"
            "};\n\n";
        
        std::string blockName;
        std::uint32_t constBlockLength = 0;
        
        bool completed = true;
        bool fixedBlockDone = false;
        bool constBlockDone = false;
        bool inoutBlockDone = false;
        bool vssrcBlockDone = false;
        bool fssrcBlockDone = false;
        
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
                    
                    if (shaderUtils::formCodeBlock(indent, input, codeBlock)) {
                        nativeShader += funcReturnType + " " + funcName + "(" + funcSignature + ") {\n";
                        nativeShader += codeBlock;
                        nativeShader += "}\n\n";
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
                if (inoutBlockDone == false) {
                    inoutBlockDone = true;
                    nativeShader += "struct _InOut {\n    float4 position [[position]];\n};\n";
                }

                std::size_t index = 0;
                std::uint32_t offset = 0;
                
                auto formInput = [&](const std::vector<InputLayout::Attribute> &desc, std::uint32_t bufferIndex, std::string &output) {
                    offset = 0;
                                
                    for (const auto &item : desc) {
                        NativeFormat &fmt = g_formatConversionTable[int(item.format)];
                        output += indent + fmt.nativeTypeName + " " + item.name + ";\n";
                        offset += fmt.size;
                        index++;
                    }
                };
                
                nativeShader += "struct _VSVertexIn {\n";
                formInput(layout.vertexAttributes, 0, nativeShader);
                nativeShader += "};\n";
                nativeShader += "struct _VSInstanceIn {\n";
                formInput(layout.instanceAttributes, 1, nativeShader);
                nativeShader += "};\n\n";
                
                nativeShader +=
                    "vertex _InOut main_vertex(\n"
                    "    unsigned int _v_ID [[vertex_id]],\n"
                    "    unsigned int instance_ID [[instance_id]],\n";

                nativeShader += "    constant _FrameData &framedata [[buffer(0)]],\n";
                nativeShader += constBlockDone ? "    constant _Constants &constants [[buffer(1)]],\n" : "";
                nativeShader += "    device const _VSVertexIn *vertices [[buffer(2)]],\n";
                nativeShader += "    device const _VSInstanceIn *instances [[buffer(3)]],\n";
                nativeShader +=
                    "    "
                    "texture2d<float> _texture0 [[texture(0)]],\n    "
                    "texture2d<float> _texture1 [[texture(1)]],\n    "
                    "texture2d<float> _texture2 [[texture(2)]],\n    "
                    "texture2d<float> _texture3 [[texture(3)]])\n{\n";
                
                nativeShader += std::string("    const uint repeat_ID = _v_ID % ") + std::to_string(layout.vertexRepeat) + ";\n";
                nativeShader += std::string("    const uint vertex_ID = _v_ID / ") + std::to_string(layout.vertexRepeat) + ";\n";
                nativeShader += "    _InOut output;\n";

                std::string codeBlock;
                
                if (shaderUtils::formCodeBlock(indent, input, codeBlock) == false) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' has uncompleted 'vssrc' block\n", name);
                    completed = false;
                    break;
                }
                
                shaderUtils::replace(codeBlock, "vertex_", "vertices[vertex_ID].", SEPARATORS, {"vertex_ID"});
                shaderUtils::replace(codeBlock, "instance_", "instances[instance_ID].", SEPARATORS, {"instance_ID"});
                shaderUtils::replace(codeBlock, "output_", "output.", SEPARATORS);
                shaderUtils::replace(codeBlock, "const_", "constants.", SEPARATORS);
                shaderUtils::replace(codeBlock, "frame_", "framedata.", SEPARATORS);
                
                nativeShader += codeBlock;
                nativeShader += "    (void)repeat_ID;\n";
                nativeShader += "    (void)vertex_ID;\n";
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
                    "};\n"
                    "fragment _Output main_fragment(\n    "
                    "_InOut input [[stage_in]],\n    "
                    "texture2d<float> _texture0 [[texture(0)]],\n    "
                    "texture2d<float> _texture1 [[texture(1)]],\n    "
                    "texture2d<float> _texture2 [[texture(2)]],\n    "
                    "texture2d<float> _texture3 [[texture(3)]],\n"
                    "";

                nativeShader += "    constant _FrameData &framedata [[buffer(0)]]";
                nativeShader += constBlockDone ? ",\n    constant _Constants &constants [[buffer(1)]])\n{\n    " : ")\n{\n    ";
                nativeShader += "float2 fragment_coord = input.position.xy / framedata.rtBounds.xy;\n    ";
                nativeShader += "float4 output_color[4] = {};\n";
                
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
                nativeShader += "    return _Output {output_color[0], output_color[1], output_color[2], output_color[3]};\n";
                nativeShader += "    (void)fragment_coord;\n";
                nativeShader += "    (void)_nearestSampler;\n";
                nativeShader += "    (void)_linearSampler;\n";
                nativeShader += "}\n";
                fssrcBlockDone = true;
                continue;
            }
            
            _platform->logError("[MetalRendering::createShader] shader '%s' has unexpected '%s' block\n", name, blockName.data());
        }
        
        nativeShader = shaderUtils::makeLines(nativeShader);
        printf("---------- begin ----------\n%s\n----------- end -----------\n", nativeShader.data());
        
        if (completed && vssrcBlockDone && fssrcBlockDone) {
            @autoreleasepool {
                NSError *nsError = nil;
                MTLCompileOptions* compileOptions = [MTLCompileOptions new];
                compileOptions.languageVersion = MTLLanguageVersion2_0;
                compileOptions.fastMathEnabled = true;
                
                id<MTLLibrary> library = [_device newLibraryWithSource:[NSString stringWithUTF8String:nativeShader.data()] options:compileOptions error:&nsError];

                if (library) {
                    layout.vertexRepeat = std::max(layout.vertexRepeat, std::uint32_t(1));
                    result = std::make_shared<MetalShader>(name, std::move(layout), library, constBlockLength);
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
            MTLPixelFormatInvalid,
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
    
    RenderTargetPtr MetalRendering::createRenderTarget(RenderTextureFormat format, unsigned count, std::uint32_t w, std::uint32_t h, bool withZBuffer) {
        @autoreleasepool {
            MTLTextureDescriptor *desc = [MTLTextureDescriptor new];
            id<MTLTexture> targets[RenderTarget::MAX_TEXTURE_COUNT] = {nullptr, nullptr, nullptr, nullptr};
            
            desc.pixelFormat = nativeTextureFormat[int(format)];
            desc.width = w;
            desc.height = h;
            desc.mipmapLevelCount = 1;
            desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite | MTLTextureUsageRenderTarget;

            for (unsigned i = 0; i < count; i++) {
                targets[i] = [_device newTextureWithDescriptor:desc];
            }

            id<MTLTexture> depth = nil;
            
            if (withZBuffer) {
                desc.pixelFormat = MTLPixelFormatDepth32Float;
                desc.usage = MTLTextureUsageRenderTarget;
                desc.storageMode = MTLStorageModeMemoryless;
                depth = [_device newTextureWithDescriptor:desc];
            }
            
            return std::make_shared<MetalTarget>(targets, count, depth, format, w, h);
        }
    }
    
    RenderDataPtr MetalRendering::createData(const void *data, const std::vector<InputLayout::Attribute> &layout, std::uint32_t capacity) {
        std::uint32_t stride = 0;
        for (const InputLayout::Attribute &item : layout) {
            stride += g_formatConversionTable[int(item.format)].size;
        }
        @autoreleasepool {
            id<MTLBuffer> buffer = [_device newBufferWithBytes:data length:(capacity * stride) options:MTLResourceStorageModeShared];
            return std::make_shared<MetalData>(buffer, stride);
        }
    }
    
    float MetalRendering::getBackBufferWidth() const {
        return _platform->getScreenWidth();
    }
    
    float MetalRendering::getBackBufferHeight() const {
        return _platform->getScreenHeight();
    }
    
    void MetalRendering::applyState(const RenderShaderPtr &shader, const RenderPassConfig &cfg) {
        if (_currentRenderCommandEncoder) {
            [_currentRenderCommandEncoder endEncoding];
            _currentRenderCommandEncoder = nil;
            _currentShader = nullptr;
        }
        
        if (_view == nil) {
            _view = (__bridge MTKView *)_platform->attachNativeRenderingContext((__bridge void *)_device);
        }
        if (_view) {
            if (_currentCommandBuffer == nil) {
                _currentCommandBuffer = [_commandQueue commandBuffer];

                dispatch_semaphore_wait(_constBufferSemaphore, DISPATCH_TIME_FOREVER);
                _constantsBuffersIndex = (_constantsBuffersIndex + 1) % CONSTANT_BUFFER_FRAMES_MAX;
                _constantsBufferOffset = 0;
            }

            MTLClearColor clearColor = MTLClearColorMake(cfg.initialColor[0], cfg.initialColor[1], cfg.initialColor[2], cfg.initialColor[3]);
            MTLLoadAction loadAction = cfg.clearColorEnabled ? MTLLoadActionClear : MTLLoadActionLoad;
            
            _currentPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor]; //_view.currentRenderPassDescriptor;//
            _currentPassDescriptor.colorAttachments[0].texture = _view.currentDrawable.texture;
            _currentPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
            _currentPassDescriptor.colorAttachments[0].loadAction = loadAction;
            _currentPassDescriptor.colorAttachments[0].clearColor = clearColor;
            _currentPassDescriptor.depthAttachment.clearDepth = cfg.initialDepth;
            _currentPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
            _currentPassDescriptor.depthAttachment.loadAction = cfg.clearDepthEnabled ? MTLLoadActionClear : MTLLoadActionLoad;
            _currentPassDescriptor.depthAttachment.texture = _view.depthStencilTexture;
            
            if (cfg.target) {
                for (std::uint32_t i = 0; i < cfg.target->getTextureCount(); i++) {
                    _currentPassDescriptor.colorAttachments[i].texture = static_cast<const MetalTexBase *>(cfg.target->getTexture(i).get())->getNativeTexture();
                    _currentPassDescriptor.colorAttachments[i].storeAction = MTLStoreActionStore;
                    _currentPassDescriptor.colorAttachments[i].loadAction = loadAction;
                    _currentPassDescriptor.colorAttachments[i].clearColor = clearColor;
                }
                
                if (cfg.target->hasDepthBuffer() == false) {
                    _currentPassDescriptor.depthAttachment.texture = nil;
                    _currentPassDescriptor.depthAttachment.loadAction = cfg.clearDepthEnabled ? MTLLoadActionClear : MTLLoadActionLoad;
                }
            }
            
            const MetalShader *platformShader = static_cast<const MetalShader *>(shader.get());
            const MetalTarget *platformTarget = static_cast<const MetalTarget *>(cfg.target.get());
            
            id<MTLRenderPipelineState> state = nil;
            
            if (platformShader) {
                _currentShader = shader;
                std::string stateName = generateRenderPipelineStateName(*platformShader, cfg);
                
                auto index = _renderPipelineStates.find(stateName);
                if (index == _renderPipelineStates.end()) {
                    MTLRenderPipelineDescriptor *desc = [MTLRenderPipelineDescriptor new];
                    
                    desc.vertexFunction = platformShader->getVertexShader();
                    desc.fragmentFunction = platformShader->getFragmentShader();
                    desc.colorAttachments[0].pixelFormat = _view.colorPixelFormat;
                    desc.depthAttachmentPixelFormat = _view.depthStencilPixelFormat;
                    initializeBlendOptions(desc.colorAttachments[0], cfg);
                    
                    if (cfg.target) {
                        desc.depthAttachmentPixelFormat = platformTarget->getDepthFormat();

                        for (std::uint32_t i = 0; i < cfg.target->getTextureCount(); i++) {
                            desc.colorAttachments[i].pixelFormat = nativeTextureFormat[int(platformTarget->getFormat())];
                            initializeBlendOptions(desc.colorAttachments[i], cfg);
                        }
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
            }

            _currentRenderCommandEncoder = [_currentCommandBuffer renderCommandEncoderWithDescriptor:_currentPassDescriptor];

            if (state) {
                double rtWidth = _platform->getScreenWidth();
                double rtHeight = _platform->getScreenHeight();
                
                if (cfg.target) {
                    rtWidth = double(cfg.target->getWidth());
                    rtHeight = double(cfg.target->getHeight());
                }
                
                _frameConstants.rtBounds.x = rtWidth;
                _frameConstants.rtBounds.y = rtHeight;
                _appendConstantBuffer(&_frameConstants, sizeof(FrameConstants), FRAME_CONST_BINDING_INDEX);
                
                if (cfg.zBehaviorType != foundation::ZBehaviorType::DISABLED) {
                    [_currentRenderCommandEncoder setDepthStencilState:_zBehaviorStates[int(cfg.zBehaviorType)]];
                }

                MTLViewport viewPort {0.0f, 0.0f, rtWidth, rtHeight, 0.0f, 1.0f};

                [_currentRenderCommandEncoder setRenderPipelineState:state];
                [_currentRenderCommandEncoder setViewport:viewPort];
                
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
    
    void MetalRendering::applyTextures(const RenderTexturePtr *textures, std::uint32_t count) {
        if (_currentRenderCommandEncoder && _currentShader) {
            if (count <= MAX_TEXTURES) {
                NSRange range {0, count};
                id<MTLTexture> texarray[MAX_TEXTURES] = {nil};
                
                for (std::uint32_t i = 0; i < count; i++) {
                    texarray[i] = textures[i] ? static_cast<const MetalTexBase *>(textures[i].get())->getNativeTexture() : nil;
                }
                
                [_currentRenderCommandEncoder setFragmentTextures:texarray withRange:range];
                [_currentRenderCommandEncoder setVertexTextures:texarray withRange:range];
            }
        }
    }
    
    void MetalRendering::draw(std::uint32_t vertexCount, RenderTopology topology) {
        if (_currentRenderCommandEncoder && _currentShader) {
            id<MTLBuffer> buffers[2] = {nullptr, nullptr};
            NSUInteger offsets[2] = {0, 0};
            NSRange vrange {VERTEX_IN_BINDING_START, 2};
            
            //[_currentRenderCommandEncoder setTriangleFillMode:MTLTriangleFillModeLines];
            [_currentRenderCommandEncoder setVertexBuffers:buffers offsets:offsets withRange:vrange];
            [_currentRenderCommandEncoder drawPrimitives:g_topologies[int(topology)] vertexStart:0 vertexCount:vertexCount];
        }
    }
    
    void MetalRendering::draw(const RenderDataPtr &vertexData, RenderTopology topology) {
        if (_currentRenderCommandEncoder && _currentShader && vertexData) {
            const MetalData *implData = static_cast<const MetalData *>(vertexData.get());
            const std::uint32_t totalVertexCount = implData->getCount() * _currentShader->getInputLayout().vertexRepeat;
            id<MTLBuffer> buffers[2] = {implData->get(), nullptr};

            NSUInteger offsets[2] = {0, 0};
            NSRange vrange {VERTEX_IN_BINDING_START, 2};
            
            //[_currentRenderCommandEncoder setTriangleFillMode:MTLTriangleFillModeLines];
            [_currentRenderCommandEncoder setVertexBuffers:buffers offsets:offsets withRange:vrange];
            [_currentRenderCommandEncoder drawPrimitives:g_topologies[int(topology)] vertexStart:0 vertexCount:totalVertexCount];
        }
    }
    
    void MetalRendering::drawIndexed(const RenderDataPtr &vertexData, const RenderDataPtr &indexData, RenderTopology topology) {
        if (_currentRenderCommandEncoder && _currentShader) {
            if (vertexData && indexData) {
                const MetalData *implVData = static_cast<const MetalData *>(vertexData.get());
                const MetalData *implIData = static_cast<const MetalData *>(indexData.get());

                id<MTLBuffer> buffers[2] = {implVData->get(), nullptr};
                NSUInteger offsets[2] = {0, 0};
                NSRange vrange {VERTEX_IN_BINDING_START, 2};
                MTLPrimitiveType topology = g_topologies[int(topology)];
                
                //[_currentRenderCommandEncoder setTriangleFillMode:MTLTriangleFillModeLines];
                [_currentRenderCommandEncoder setVertexBuffers:buffers offsets:offsets withRange:vrange];
                [_currentRenderCommandEncoder drawIndexedPrimitives:topology indexCount:implIData->getCount() indexType:MTLIndexTypeUInt32 indexBuffer:implIData->get() indexBufferOffset:0];
            }
        }
    }

    void MetalRendering::drawInstanced(const RenderDataPtr &vertexData, const RenderDataPtr &instanceData, RenderTopology topology) {
        if (_currentRenderCommandEncoder && _currentShader) {
            if (vertexData && instanceData) {
                const MetalData *implVData = static_cast<const MetalData *>(vertexData.get());
                const MetalData *implIData = static_cast<const MetalData *>(instanceData.get());

                id<MTLBuffer> buffers[2] = {implVData->get(), implIData->get()};
                NSUInteger offsets[2] = {0, 0};
                NSRange vrange {VERTEX_IN_BINDING_START, 2};
                
                //[_currentRenderCommandEncoder setTriangleFillMode:MTLTriangleFillModeLines];
                [_currentRenderCommandEncoder setVertexBuffers:buffers offsets:offsets withRange:vrange];
                [_currentRenderCommandEncoder drawPrimitives:g_topologies[int(topology)] vertexStart:0 vertexCount:implVData->getCount() instanceCount:implIData->getCount()];
            }
        }
    }
    
    void MetalRendering::presentFrame() {
        if (_currentRenderCommandEncoder) {
            [_currentRenderCommandEncoder endEncoding];
            _currentRenderCommandEncoder = nil;
            _currentShader = nullptr;
        }
        if (_view && _currentCommandBuffer) {
            MetalRendering *m = this;
            
            [_currentCommandBuffer presentDrawable:_view.currentDrawable];
            [_currentCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
                dispatch_semaphore_signal(m->_constBufferSemaphore);
            }];
            [_currentCommandBuffer commit];
            _currentCommandBuffer = nil;
        }
    }
    
    void MetalRendering::_appendConstantBuffer(const void *buffer, std::uint32_t size, std::uint32_t index) {
        std::uint32_t roundedSize = roundTo256(size);
        
        if (_constantsBufferOffset + roundedSize < CONSTANT_BUFFER_OFFSET_MAX) {
            std::uint8_t *constantsMemory = static_cast<std::uint8_t *>([_constantsBuffers[_constantsBuffersIndex] contents]);
            std::memcpy(constantsMemory + _constantsBufferOffset, buffer, size);
            
            [_currentRenderCommandEncoder setVertexBuffer:_constantsBuffers[_constantsBuffersIndex] offset:_constantsBufferOffset atIndex:index];
            [_currentRenderCommandEncoder setFragmentBuffer:_constantsBuffers[_constantsBuffersIndex] offset:_constantsBufferOffset atIndex:index];
            
            _constantsBufferOffset+= roundedSize;
            return true;
        }
        else {
            _platform->logError("[MetalRendering::_appendConstantBuffer] Out of constants buffer length\n");
        }
        return false;
    }
}

namespace foundation {
    std::shared_ptr<RenderingInterface> RenderingInterface::instance(const std::shared_ptr<PlatformInterface> &platform) {
        return std::make_shared<MetalRendering>(platform);
    }
}

#endif // PLATFORM_IOS
