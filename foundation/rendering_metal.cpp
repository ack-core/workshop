

#include <cctype>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <regex>

#ifdef PLATFORM_IOS

#if ! __has_feature(objc_arc)
#error "ARC is off"
#endif

#include "rendering_metal.h"

namespace {
    template <typename = void> std::istream &expect(std::istream &stream) {
        return stream;
    }
    template <char Ch, char... Chs> std::istream &expect(std::istream &stream) {
        if ((stream >> std::ws).peek() == Ch) {
            stream.ignore();
        }
        else {
            stream.setstate(std::ios_base::failbit);
        }
        return expect<Chs...>(stream);
    }
    
    struct braced {
        explicit braced(std::string &out, char opening, char closing) : _opening(opening), _closing(closing), _out(out) {}
        
        inline friend std::istream &operator >>(std::istream &stream, const braced &target) {
            (stream >> std::ws).peek();
            (stream >> std::ws).peek() == target._opening ? (void)stream.ignore() : stream.setstate(std::ios_base::failbit);
            
            while (stream.fail() == false && stream.peek() != target._closing) {
                target._out.push_back(stream.get());
            }
            if (stream.fail() == false) {
                (stream >> std::ws).peek() == target._closing ? (void)stream.ignore() : stream.setstate(std::ios_base::failbit);
            }
            
            return stream;
        }
        
    private:
        char _opening;
        char _closing;
        std::string &_out;
    };
    
    static const MTLPrimitiveType g_topologies[] = {
        MTLPrimitiveTypePoint,
        MTLPrimitiveTypeLine,
        MTLPrimitiveTypeLineStrip,
        MTLPrimitiveTypeTriangle,
        MTLPrimitiveTypeTriangleStrip,
    };
    
    static const std::uint32_t MAX_TEXTURES = 4;
    
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
}

namespace foundation {
    MetalShader::MetalShader(const std::string &name, id<MTLLibrary> library, MTLVertexDescriptor *vdesc, std::uint32_t constBufferLength)
        : _name(name)
        , _library(library)
        , _vdesc(vdesc)
        , _constBufferLength(constBufferLength)
    {}
    
    MetalShader::~MetalShader() {
        _library = nil;
        _vdesc = nil;
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
    
    MTLVertexDescriptor *MetalShader::getVertexDesc() const {
        return _vdesc;
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
    MetalData::MetalData(id<MTLBuffer> buffer, std::uint32_t count, std::uint32_t stride) : _buffer(buffer), _count(count), _stride(stride) {
    
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
    
    void MetalRendering::updateFrameConstants(const float(&view)[16], const float(&proj)[16], const float(&camPos)[3], const float(&camDir)[3]) {
        ::memcpy(_frameConstants.viewMatrix, view, 16 * sizeof(float));
        ::memcpy(_frameConstants.projMatrix, proj, 16 * sizeof(float));
        ::memcpy(_frameConstants.cameraPosition, camPos, 3 * sizeof(float));
        ::memcpy(_frameConstants.cameraDirection, camDir, 3 * sizeof(float));
    }
    
    void MetalRendering::getFrameConstants(float(&view)[16], float(&proj)[16], float(&camPos)[3], float(&camDir)[3]) {
        ::memcpy(view, _frameConstants.viewMatrix, 16 * sizeof(float));
        ::memcpy(proj, _frameConstants.projMatrix, 16 * sizeof(float));
        ::memcpy(camPos, _frameConstants.cameraPosition, 3 * sizeof(float));
        ::memcpy(camDir, _frameConstants.cameraDirection, 3 * sizeof(float));
    }
    
    namespace shaderUtils {
        bool formCodeBlock(const std::string &indent, std::istringstream &stream, std::string &output) {
            stream >> std::ws;
            int  braceCounter = 1;
            char prev = '\n', next;

            while (stream && ((next = stream.get()) != '}' || --braceCounter > 0)) {
                if (prev == '\n') output += indent;
                if (next == '{') braceCounter++;
                output += next;
                if (next == '\n')  stream >> std::ws;
                prev = next;
            }
            
            return braceCounter == 0;
        };
        
        std::string makeLines(const std::string &src) {
            std::string result;
            std::string::const_iterator left = std::begin(src);
            std::string::const_iterator right;
            int counter = 0;

            while ((right = std::find(left, std::end(src), '\n')) != std::end(src)) {
                right++;
                char line[64];
                std::snprintf(line, 64, "/* %3d */  ", ++counter);
                result += std::string(line) + std::string(left, right);
                left = right;
            }

            return result;
        };
    }

    RenderShaderPtr MetalRendering::createShader(const char *name, const char *shadersrc, const RenderShaderInputDesc &vertex, const RenderShaderInputDesc &instance) {
        std::shared_ptr<RenderShader> result;
        std::istringstream input(shadersrc);
        const std::string indent = "    ";
        
        if (_shaderNames.find(name) == _shaderNames.end()) {
            _shaderNames.emplace(name);
        }
        else {
            _platform->logError("[MetalRendering::createShader] shader name '%s' already used\n", name);
        }
        
        auto getArrayMultiply = [](const std::string &varname) {
            int  multiply = 1;
            auto braceStart = varname.find('[');
            auto braceEnd = varname.rfind(']');

            if (braceStart != std::string::npos && braceEnd != std::string::npos) {
                multiply = std::max(std::stoi(varname.substr(braceStart + 1, braceEnd - braceStart - 1)), multiply);
            }
            
            return multiply;
        };
        
        auto shaderGetTypeSize = [&getArrayMultiply](const std::string &varname, const std::string &typeName, bool packed, std::string &nativeTypeName, std::size_t &size, std::size_t &count) {
            struct {
                const char *typeName;
                const char *nativeTypeName;
                const char *nativeTypeNamePacked;
                std::size_t packedSize;
            }
            typeSizeTable[] = {
                {"float1",  "float",    "float",           4},
                {"float2",  "float2",   "packed_float2",   8}, // TODO: decline in 'const' block
                {"float3",  "float3",   "packed_float3",   12},
                {"float4",  "float4",   "float4",          16},
                {"int1",    "int",      "int",             4},
                {"int2",    "int2",     "packed_int2",     8},
                {"int3",    "int3",     "packed_int3",     12},
                {"int4",    "int4",     "int4",            16},
                {"uint1",   "uint",     "uint",            4},
                {"uint2",   "uint2",    "packed_uint2",    8},
                {"uint3",   "uint3",    "packed_uint3",    12},
                {"uint4",   "uint4",    "uint4",           16},
                {"matrix3", "float3x3", "packed_float3x3", 36},
                {"matrix4", "float4x4", "float4x4",        64},
            };
            
            int  multiply = getArrayMultiply(varname);
            for (auto index = std::begin(typeSizeTable); index != std::end(typeSizeTable); ++index) {
                if (index->typeName == typeName) {
                    nativeTypeName = packed ? index->nativeTypeNamePacked : index->nativeTypeName;
                    size = index->packedSize;
                    count = multiply;
                    return true;
                }
            }

            return false;
        };
        
        auto formFixedBlock = [&shaderGetTypeSize, &indent](std::istringstream &stream, std::string &output) {
            std::string varname, arg;
            
            while (stream >> varname && varname[0] != '}') {
                if (stream >> expect<':'> >> arg >> expect<'='>) {
                    std::size_t elementSize, elementCount;
                    std::string nativeTypeName;
                    
                    if (shaderGetTypeSize(varname, arg, false, nativeTypeName, elementSize, elementCount)) {
                        output += "constant " + nativeTypeName + " fixed_" + varname + " = {\n";
                        for (std::size_t i = 0; i < elementCount; i++) {
                            output += indent + nativeTypeName + "(";

                            if (stream >> braced(output, '[', ']')) {
                                output += "),\n";
                            }
                            else return false;
                        }
                        
                        output += "};\n";
                        continue;
                    }
                }
                
                return false;
            }
            
            return true;
        };

        auto formVarsBlock = [&shaderGetTypeSize, &indent](std::istringstream &stream, std::string &output, bool packed) {
            std::string varname, arg;
            std::uint32_t totalLength = 0;
            
            while (stream >> varname && varname[0] != '}') {
                if (stream >> expect<':'> >> arg) {
                    std::size_t elementSize, elementCount;
                    std::string nativeTypeName;
                    
                    if (shaderGetTypeSize(varname, arg, packed, nativeTypeName, elementSize, elementCount)) {
                        output += indent + nativeTypeName + " " + varname + ";\n";
                        totalLength += elementSize * elementCount;
                        continue;
                    }
                }
                
                return std::uint32_t(0);
            }
            
            return totalLength;
        };
        
        auto replacePattern = [](std::string &target, const std::string &pattern, const std::string &replacement, const std::string &prevChExc, const std::string &exception = {}) {
            std::size_t start_pos = 0;
            while((start_pos = target.find(pattern, start_pos)) != std::string::npos) {
                if (exception.length() == 0 || ::memcmp(target.data() + start_pos, exception.data(), exception.length()) != 0) {
                    if (prevChExc.find(target.data()[start_pos - 1]) == std::string::npos) {
                        target.replace(start_pos, pattern.length(), replacement);
                        start_pos += replacement.length();
                        continue;
                    }
                }
                
                start_pos++;
            }
        };

        MTLVertexDescriptor *vdesc = [MTLVertexDescriptor vertexDescriptor];
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
            "#define _asFloat(a) as_type<float>(a)\n"
            "#define _asUint(a) as_type<uint>(a)\n"
            "#define _transform(a, b) (b * a)\n"
            "#define _dot(a, b) dot(a, b)\n"
            "#define _cross(a, b) cross(a, b)\n"
            "#define _len(a) length(a)\n"
            "#define _pow(a, b) pow(a, b)\n"
            "#define _fract(a) fract(a)\n"
            "#define _floor(a) floor(a)\n"
            "#define _clamp(a) clamp(a, 0.0, 1.0)\n"
            "#define _norm(a) normalize(a)\n"
            "#define _lerp(a, b, k) mix(a, b, k)\n"
            "#define _step(k, a) step(k, a)\n"
            "#define _smooth(a, b, k) smoothstep(a, b, k)\n"
            "#define _min(a, b) min(a, b)\n"
            "#define _max(a, b) max(a, b)\n"
            "#define _tex2nearest(i, a) _texture##i.sample(_nearestSampler, a)\n"
            "#define _tex2linear(i, a) _texture##i.sample(_linearSampler, a)\n"
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
            "};\n";
        
        std::string blockName;
        std::uint32_t constBlockLength = 0;
        
        bool completed = true;
        bool fixedBlockDone = false;
        bool constBlockDone = false;
        bool inoutBlockDone = false;
        bool vssrcBlockDone = false;
        bool fssrcBlockDone = false;
        
        while (input >> blockName) {
            if (fixedBlockDone == false && blockName == "fixed" && (input >> expect<'{'>)) {
                if (formFixedBlock(input, nativeShader) == false) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' has ill-formed 'fixed' block\n", name);
                    completed = false;
                    break;
                }

                fixedBlockDone = true;
                continue;
            }
            if (constBlockDone == false && blockName == "const" && (input >> expect<'{'>)) {
                nativeShader += "struct _Constants {\n";
                
                if ((constBlockLength = formVarsBlock(input, nativeShader, true)) == 0) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' has ill-formed 'const' block\n", name);
                    completed = false;
                    break;
                }
                nativeShader += "};\n";
                constBlockDone = true;
                continue;
            }
            if (inoutBlockDone == false && blockName == "inout" && (input >> expect<'{'>)) {
                nativeShader += "struct _InOut {\n    float4 position [[position]];\n";
                
                if (formVarsBlock(input, nativeShader, false) == 0) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' has ill-formed 'inout' block\n", name);
                    completed = false;
                    break;
                }
                nativeShader += "};\n";
                inoutBlockDone = true;
                continue;
            }
            if (blockName == "fndef") {
                std::string funcName;
                std::string funcSignature;
                std::string funcReturnType;

                if (input >> funcName >> braced(funcSignature, '(', ')') >> expect<'-', '>'> >> funcReturnType >> expect<'{'>) {
                    std::string codeBlock;
                    
                    if (shaderUtils::formCodeBlock(indent, input, codeBlock)) {
                        nativeShader += funcReturnType + " " + funcName + "(" + funcSignature + ") {\n";
                        nativeShader += codeBlock;
                        nativeShader += "}\n";
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
            if (vssrcBlockDone == false && blockName == "vssrc" && (input >> expect<'{'>)) {
                if (inoutBlockDone == false) {
                    inoutBlockDone = true;
                    nativeShader += "struct _InOut {\n    float4 position [[position]];\n};\n";
                }
                nativeShader += "struct _VSIn {\n";

                struct NativeFormat {
                    const char *nativeTypeName;
                    std::uint32_t size;
                    MTLVertexFormat nativeFormat;
                }
                formatTable[] = { // index is RenderShaderInputFormat value
                    {"uint",   0,  MTLVertexFormatInvalid},
                    {"float2", 4,  MTLVertexFormatHalf2},
                    {"float4", 8,  MTLVertexFormatHalf4},
                    {"float1", 4,  MTLVertexFormatFloat},
                    {"float2", 8,  MTLVertexFormatFloat2},
                    {"float3", 12, MTLVertexFormatFloat3},
                    {"float4", 16, MTLVertexFormatFloat4},
                    {"int2",   4,  MTLVertexFormatShort2},
                    {"int4",   8,  MTLVertexFormatShort4},
                    {"float2", 4,  MTLVertexFormatShort2Normalized},
                    {"float4", 8,  MTLVertexFormatShort4Normalized},
                    {"uint4",  4,  MTLVertexFormatUChar4},
                    {"float4", 4,  MTLVertexFormatUChar4Normalized},
                    {"int1",   4,  MTLVertexFormatInt},
                    {"int2",   8,  MTLVertexFormatInt2},
                    {"int3",   12, MTLVertexFormatInt3},
                    {"int4",   16, MTLVertexFormatInt4},
                    {"uint1",  4,  MTLVertexFormatUInt},
                    {"uint2",  8,  MTLVertexFormatUInt2},
                    {"uint3",  12, MTLVertexFormatUInt3},
                    {"uint4",  16, MTLVertexFormatUInt4}
                };

                std::string vertexIDName;
                std::string instanceIDName;
                std::size_t index = 0;
                std::uint32_t offset = 0;
                
                auto formInput = [&](const RenderShaderInputDesc &desc, const std::string &prefix, std::uint32_t bufferIndex, std::string &idout, std::string &output) {
                    offset = 0;
                                
                    for (const auto &item : desc) {
                        if (item.format == RenderShaderInputFormat::ID) {
                            idout = prefix + item.name;
                        }
                        else {
                            NativeFormat &fmt = formatTable[int(item.format)];
                            vdesc.attributes[index].bufferIndex = bufferIndex;
                            vdesc.attributes[index].offset = offset;
                            vdesc.attributes[index].format = fmt.nativeFormat;
                            
                            output += indent + fmt.nativeTypeName + " " + item.name + " [[attribute(" + std::to_string(index) + ")]];\n";
                            offset += fmt.size;
                            index++;
                        }
                    }
                    vdesc.layouts[bufferIndex].stride = offset;
                    vdesc.layouts[bufferIndex].stepRate = 1;
                };
                
                formInput(vertex, "vertex_", 0, vertexIDName, nativeShader);
                formInput(instance, "instance_", 1, instanceIDName, nativeShader);

                vdesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
                vdesc.layouts[1].stepFunction = MTLVertexStepFunctionPerInstance;

                nativeShader += "};\nvertex _InOut main_vertex(\n";

                if (vertexIDName.length()) {
                    nativeShader += "    unsigned int " + vertexIDName + " [[vertex_id]],\n";
                }
                if (instanceIDName.length()) {
                    nativeShader += "    unsigned int " + instanceIDName + " [[instance_id]],\n";
                }
                if (vdesc.layouts[0].stride || vdesc.layouts[1].stride) { // if there is something but vertex_id/instance_id
                    nativeShader += "    _VSIn input [[stage_in]],\n";
                }
                if (constBlockLength) {
                    nativeShader += "    constant _Constants &constants [[buffer(2)]],\n";
                }
                    
                nativeShader += "    constant _FrameData &framedata [[buffer(3)]],\n    ";
                nativeShader +=
                    "texture2d<float> _texture0 [[texture(0)]],\n    "
                    "texture2d<float> _texture1 [[texture(1)]],\n    "
                    "texture2d<float> _texture2 [[texture(2)]],\n    "
                    "texture2d<float> _texture3 [[texture(3)]])\n{\n    _InOut output;\n";

                std::string codeBlock;
                
                if (shaderUtils::formCodeBlock(indent, input, codeBlock) == false) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' has uncompleted 'vssrc' block\n", name);
                    completed = false;
                    break;
                }
  
                replacePattern(codeBlock, "vertex_", "input.", "._", vertexIDName);
                replacePattern(codeBlock, "instance_", "input.", "._", instanceIDName);
                replacePattern(codeBlock, "output_", "output.", "._");
                replacePattern(codeBlock, "const_", "constants.", "._");
                replacePattern(codeBlock, "frame_", "framedata.", "._");
                nativeShader += codeBlock;
                nativeShader += "    return output;\n}\n";
                vssrcBlockDone = true;
                continue;
            }
            if (fssrcBlockDone == false && blockName == "fssrc" && (input >> expect<'{'>)) {
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
                
                if (constBlockLength) {
                    nativeShader += "    constant _Constants &constants [[buffer(0)]],\n";
                }
                
                nativeShader += "    constant _FrameData &framedata [[buffer(1)]])\n{\n    ";
                nativeShader += "float2 fragment_coord = input.position.xy / framedata.rtBounds.xy;\n    ";
                nativeShader += "float4 output_color[4] = {};\n";
                
                std::string codeBlock;

                if (shaderUtils::formCodeBlock(indent, input, codeBlock) == false) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' has uncompleted 'fssrc' block\n", name);
                    completed = false;
                    break;
                }
                
                replacePattern(codeBlock, "frame_", "framedata.", "._");
                replacePattern(codeBlock, "const_", "constants.", "._");
                replacePattern(codeBlock, "input_", "input.", "._");
                nativeShader += codeBlock;
                nativeShader += "    return _Output {output_color[0], output_color[1], output_color[2], output_color[3]};\n}\n";
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
                    result = std::make_shared<MetalShader>(name, library, vdesc, constBlockLength);
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
            id<MTLTexture> targets[RenderTarget::MAXCOUNT] = {nullptr, nullptr, nullptr, nullptr};
            
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
    
    RenderDataPtr MetalRendering::createData(const void *data, std::uint32_t count, std::uint32_t stride) {
        @autoreleasepool {
            id<MTLBuffer> buffer = [_device newBufferWithBytes:data length:(count * stride) options:MTLResourceStorageModeShared];
            return std::make_shared<MetalData>(buffer, count, stride);
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
            }

            MTLClearColor clearColor = MTLClearColorMake(cfg.initialColor[0], cfg.initialColor[1], cfg.initialColor[2], cfg.initialColor[3]);
            MTLLoadAction loadAction = cfg.clearColorEnabled ? MTLLoadActionClear : MTLLoadActionDontCare;
            
            _currentPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
            _currentPassDescriptor.colorAttachments[0].texture = _view.currentDrawable.texture;
            _currentPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionDontCare;
            _currentPassDescriptor.colorAttachments[0].loadAction = loadAction;
            _currentPassDescriptor.colorAttachments[0].clearColor = clearColor;
            _currentPassDescriptor.depthAttachment.clearDepth = cfg.initialDepth;
            _currentPassDescriptor.depthAttachment.loadAction = cfg.clearDepthEnabled ? MTLLoadActionClear : MTLLoadActionDontCare;
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
                    desc.vertexDescriptor = platformShader->getVertexDesc();
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
                double rtWidth = _view.currentDrawable.texture.width;
                double rtHeight = _view.currentDrawable.texture.height;
                
                if (cfg.target) {
                    rtWidth = double(cfg.target->getWidth());
                    rtHeight = double(cfg.target->getHeight());
                }
                
                _frameConstants.rtBounds[0] = rtWidth;
                _frameConstants.rtBounds[1] = rtHeight;                
                
                _appendConstantBuffer(&_frameConstants, sizeof(FrameConstants), 3, 1);

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
                _appendConstantBuffer(constants, constBufferLength, 2, 0);
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
    
    void MetalRendering::drawGeometry(const RenderDataPtr &vertexData, std::uint32_t vcount, RenderTopology topology) {
        if (_currentRenderCommandEncoder && _currentShader) {
            id<MTLBuffer> nativeVertexData = vertexData ? static_cast<const MetalData *>(vertexData.get())->get() : nullptr;
            id<MTLBuffer> buffers[2] = {nativeVertexData, nullptr};

            NSUInteger offsets[2] = {0, 0};
            NSRange vrange {0, 2};
            
            //[_currentRenderCommandEncoder setTriangleFillMode:MTLTriangleFillModeLines];
            [_currentRenderCommandEncoder setVertexBuffers:buffers offsets:offsets withRange:vrange];
            [_currentRenderCommandEncoder drawPrimitives:g_topologies[int(topology)] vertexStart:0 vertexCount:vcount];
        }
    }
    
    void MetalRendering::drawGeometry(const RenderDataPtr &vertexData, const RenderDataPtr &indexData, std::uint32_t indexCount, RenderTopology topology) {
        if (_currentRenderCommandEncoder && _currentShader) {
            if (vertexData && indexData) {
                id<MTLBuffer> idata = static_cast<const MetalData *>(indexData.get())->get();
                id<MTLBuffer> nativeVertexData = static_cast<const MetalData *>(vertexData.get())->get();
                id<MTLBuffer> buffers[2] = {nativeVertexData, nullptr};

                NSUInteger offsets[2] = {0, 0};
                NSRange vrange {0, 2};
                MTLPrimitiveType t = g_topologies[int(topology)];
                
                //[_currentRenderCommandEncoder setTriangleFillMode:MTLTriangleFillModeLines];
                [_currentRenderCommandEncoder setVertexBuffers:buffers offsets:offsets withRange:vrange];
                [_currentRenderCommandEncoder drawIndexedPrimitives:t indexCount:indexCount indexType:MTLIndexTypeUInt32 indexBuffer:idata indexBufferOffset:0];
            }
        }
    }

    void MetalRendering::drawGeometry(const RenderDataPtr &vertexData, const RenderDataPtr &instanceData, std::uint32_t vcount, std::uint32_t icount, RenderTopology topology) {
        if (_currentRenderCommandEncoder && _currentShader) {
            id<MTLBuffer> nativeVertexData = vertexData ? static_cast<const MetalData *>(vertexData.get())->get() : nullptr;
            id<MTLBuffer> nativeInstanceData = instanceData ? static_cast<const MetalData *>(instanceData.get())->get() : nullptr;
            id<MTLBuffer> buffers[2] = {nativeVertexData, nativeInstanceData};
            
            NSUInteger offsets[2] = {0, 0};
            NSRange vrange {0, 2};
            
            [_currentRenderCommandEncoder setVertexBuffers:buffers offsets:offsets withRange:vrange];
            [_currentRenderCommandEncoder drawPrimitives:g_topologies[int(topology)] vertexStart:0 vertexCount:vcount instanceCount:icount];
        }
    }
    
    void MetalRendering::presentFrame() {
        if (_currentRenderCommandEncoder) {
            [_currentRenderCommandEncoder endEncoding];
            _currentRenderCommandEncoder = nil;
            _currentShader = nullptr;
        }
        if (_view && _currentCommandBuffer) {
            const std::uint32_t bufferIndex = _constantsBuffersIndex;
            MetalRendering *m = this;
            [_currentCommandBuffer presentDrawable:_view.currentDrawable];
            [_currentCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
                m->_constantsBuffersInUse[bufferIndex] = false;
            }];
            [_currentCommandBuffer commit];
            _currentCommandBuffer = nil;

            _constantsBuffersInUse[_constantsBuffersIndex] = true;
            _constantsBufferOffset = 0;
            
            for (std::uint32_t i = 0; i < CONSTANT_BUFFER_FRAMES_MAX; i++) {
                if (_constantsBuffersInUse[_constantsBuffersIndex] == false) {
                    _constantsBuffersIndex = i;
                    break;
                }
            }

            if (_constantsBuffersIndex >= CONSTANT_BUFFER_FRAMES_MAX) {
                _platform->logError("[MetalRendering::presentFrame] Global constant buffer in flight\n");
            }
        }
    }
    
    void MetalRendering::_appendConstantBuffer(const void *buffer, std::uint32_t size, std::uint32_t vIndex, std::uint32_t fIndex) {
        if (_constantsBufferOffset + size < CONSTANT_BUFFER_OFFSET_MAX) {
            std::uint8_t *constantsMemory = static_cast<std::uint8_t *>([_constantsBuffers[_constantsBuffersIndex] contents]);
            std::memcpy(constantsMemory + _constantsBufferOffset, buffer, size);
            
            [_currentRenderCommandEncoder setVertexBuffer:_constantsBuffers[_constantsBuffersIndex] offset:_constantsBufferOffset atIndex:vIndex];
            [_currentRenderCommandEncoder setFragmentBuffer:_constantsBuffers[_constantsBuffersIndex] offset:_constantsBufferOffset atIndex:fIndex];
            
            _constantsBufferOffset+= roundTo256(sizeof(FrameConstants));
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
