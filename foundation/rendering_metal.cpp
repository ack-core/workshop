

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
        explicit braced(std::string &out) : _out(out) {}

        inline friend std::istream &operator >>(std::istream &stream, const braced &target) {
            char dbg = (stream >> std::ws).peek();
            (stream >> std::ws).peek() == '[' ? (void)stream.ignore() : stream.setstate(std::ios_base::failbit);

            while (stream.fail() == false && stream.peek() != ']') {
                target._out.push_back(stream.get());
            }
            if (stream.fail() == false) {
                (stream >> std::ws).peek() == ']' ? (void)stream.ignore() : stream.setstate(std::ios_base::failbit);
            }

            return stream;
        }

    private:
        std::string &_out;
    };

    static const MTLPrimitiveType g_topologies[] = {
        MTLPrimitiveTypeLine,
        MTLPrimitiveTypeLineStrip,
        MTLPrimitiveTypeTriangle,
        MTLPrimitiveTypeTriangleStrip,
    };
}

namespace foundation {
    MetalShader::MetalShader(id<MTLLibrary> library, MTLVertexDescriptor *vdesc, id<MTLBuffer> constBuffer) : _library(library), _vdesc(vdesc), _constBuffer(constBuffer) {}
    MetalShader::~MetalShader() {
        _library = nil;
        _constBuffer = nil;
        _vdesc = nil;
    }
    
    id<MTLFunction> MetalShader::getVertexShader() const {
        return [_library newFunctionWithName:@"main_vertex"];
    }
    
    id<MTLFunction> MetalShader::getFragmentShader() const {
        return [_library newFunctionWithName:@"main_fragment"];
    }
    
    id<MTLBuffer> MetalShader::getConstantBuffer() const {
        return _constBuffer;
    }
    
    MTLVertexDescriptor *MetalShader::getVertexDesc() const {
        return _vdesc;
    }
}

namespace foundation {
    MetalTexture2D::MetalTexture2D(RenderingTextureFormat fmt, std::uint32_t w, std::uint32_t h, std::uint32_t mipCount)
        : _format(fmt)
        , _width(w)
        , _height(h)
        , _mipCount(mipCount)
    {}

    MetalTexture2D::~MetalTexture2D() {
    
    }

    std::uint32_t MetalTexture2D::getWidth() const {
        return _width;
    }

    std::uint32_t MetalTexture2D::getHeight() const {
        return _height;
    }

    std::uint32_t MetalTexture2D::getMipCount() const {
        return _mipCount;
    }

    RenderingTextureFormat MetalTexture2D::getFormat() const {
        return _format;
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
        _platform->logMsg("[RENDER] Initialization : Metal");

        _device = MTLCreateSystemDefaultDevice();
        _commandQueue = [_device newCommandQueue];

        _frameConstantsBuffer = [_device newBufferWithLength:sizeof(FrameConstants) options:MTLResourceStorageModeShared];
        _platform->logMsg("[RENDER] Initialization : complete");
    }

    MetalRendering::~MetalRendering() {}

    void MetalRendering::updateFrameConstants(const float(&camPos)[3], const float(&camDir)[3], const float(&camVP)[16]) {
        FrameConstants *frameConstants = (FrameConstants *)[_frameConstantsBuffer contents];
        ::memcpy(frameConstants->vewProjMatrix, camVP, 16 * sizeof(float));
        ::memcpy(frameConstants->cameraPosition, camPos, 3 * sizeof(float));
        ::memcpy(frameConstants->cameraDirection, camDir, 3 * sizeof(float));
        frameConstants->screenBounds[0] = _platform->getScreenWidth();
        frameConstants->screenBounds[1] = _platform->getScreenHeight();
    }

    static const char *sh = R"(
        using namespace metal;
        
        struct _FrameData {
            float4x4 viewProjMatrix;
            float4 cameraPosition;
            float4 cameraDirection;
            float4 renderTargetBounds;
        };
        constant float4 const_multiplier[3] = {
            float4(0.3f, 0.3f, 0.3f, 1.0f),
            float4(0.3f, 0.3f, 0.3f, 1.0f),
            float4(0.3f, 0.3f, 0.3f, 1.0f),
        };
        constant float4 const_mul00000 = {
            float4(0.3f, 0.3f, 0.3f, 1.0f),
        };
        struct _Constants {
            float4 tmp;
        };
        struct _VSIn {
            float3 position [[attribute(0)]];
            float4 color [[attribute(1)]];
        };
        struct _InOut {
            float4 position [[position]];
            float4 color;
        };
        vertex _InOut main_vertex(unsigned int vid [[vertex_id]], _VSIn input [[stage_in]], constant _FrameData &framedata [[buffer(2)]], constant _Constants &constants [[buffer(3)]]) {
            _InOut result;
            result.position = framedata.viewProjMatrix * float4(input.position, 1.0);
            result.color = input.color + const_multiplier[vid >> 1];
            return result;
        }
        fragment float4 main_fragment(_InOut input [[stage_in]]) {
            return input.color;
        }
    )";

    RenderingShaderPtr MetalRendering::createShader(const char *name, const char *shadersrc, const RenderingShaderInputDesc &vertex, const RenderingShaderInputDesc &instance) {
        std::shared_ptr<RenderingShader> result;
        std::istringstream input(shadersrc);
        const std::string indent = "    ";
        
        auto shaderGetTypeSize = [](const std::string &varname, const std::string &typeName, std::string &nativeTypeName, std::size_t &elementSize, std::size_t &elementCount) {
            struct {
                const char *typeName;
                const char *nativeTypeName;
                std::size_t size;
            }
            typeSizeTable[] = {
                {"float1",  "float",           4},
                {"float2",  "packed_float2",   8},
                {"float3",  "packed_float3",   12},
                {"float4",  "float4",          16},
                {"int1",    "int",             4},
                {"int2",    "packed_int2",     8},
                {"int3",    "packed_int3",     12},
                {"int4",    "int4",            16},
                {"uint1",   "uint",            4},
                {"uint2",   "packed_uint2",    8},
                {"uint3",   "packed_uint3",    12},
                {"uint4",   "uint4",           16},
                {"matrix3", "packed_float3x3", 36},
                {"matrix4", "float4x4",        64},
            };
            
            int  multiply = 1;
            auto braceStart = varname.find('[');
            auto braceEnd = varname.rfind(']');

            if (braceStart != std::string::npos && braceEnd != std::string::npos) {
                multiply = std::max(std::stoi(varname.substr(braceStart + 1, braceEnd - braceStart - 1)), multiply);
            }

            for (auto index = std::begin(typeSizeTable); index != std::end(typeSizeTable); ++index) {
                if (index->typeName == typeName) {
                    nativeTypeName = index->nativeTypeName;
                    elementSize = index->size;
                    elementCount = multiply;
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
                    
                    if (shaderGetTypeSize(varname, arg, nativeTypeName, elementSize, elementCount)) {
                        output += "constant " + nativeTypeName + " fixed_" + varname + " = {\n";
                        for (std::size_t i = 0; i < elementCount; i++) {
                            output += indent + nativeTypeName + "(";

                            if (stream >> braced(output)) {
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

        auto formVarsBlock = [&shaderGetTypeSize, &indent](std::istringstream &stream, std::string &output) {
            std::string varname, arg;
            std::uint32_t totalLength = 0;
            
            while (stream >> varname && varname[0] != '}') {
                if (stream >> expect<':'> >> arg) {
                    std::size_t elementSize, elementCount;
                    std::string nativeTypeName;
                    
                    if (shaderGetTypeSize(varname, arg, nativeTypeName, elementSize, elementCount)) {
                        output += indent + nativeTypeName + " " + varname + ";\n";
                        totalLength += elementSize * elementCount;
                        continue;
                    }
                }
                
                return std::uint32_t(0);
            }
            
            return totalLength;
        };
        
        auto formCodeBlock = [&shaderGetTypeSize, &indent](std::istringstream &stream, std::string &output) {
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

        auto makeLines = [](const std::string &src) {
            std::string result;
            std::string::const_iterator left = std::begin(src);
            std::string::const_iterator right;
            int counter = 0;

            while ((right = std::find(left, std::end(src), '\n')) != std::end(src)) {
                right++;
                char line[64];
                std::sprintf(line, "/* %3d */  ", ++counter);
                result += std::string(line) + std::string(left, right);
                left = right;
            }

            return result;
        };

        MTLVertexDescriptor *vdesc = [MTLVertexDescriptor vertexDescriptor];
        std::string nativeShader =
            "using namespace metal;\n"
            "\n"
            "#define _sin(a) sin(a)\n"
            "#define _cos(a) cos(a)\n"
            "#define _transform(a, b) (b * a)\n"
            "\n"
            "struct _FrameData {\n"
            "    float4x4 viewProjMatrix;\n"
            "    float4 cameraPosition;\n"
            "    float4 cameraDirection;\n"
            "    float4 screenBounds;\n"
            "};\n";
        
        std::string blockName;
        std::uint32_t constBlockLength = 0;
        
        bool completed = true;
        bool fixedBlockDone = false;
        bool constBlockDone = false;
        bool inoutBlockDone = false;
        bool vssrcBlockDone = false;
        bool fssrcBlockDone = false;
        
        while (input >> blockName >> expect<'{'>) {
            if (fixedBlockDone == false && blockName == "fixed") {
                if (formFixedBlock(input, nativeShader) == false) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' has ill-formed 'fixed' block\n", name);
                    completed = false;
                    break;
                }

                fixedBlockDone = true;
                continue;
            }
            if (constBlockDone == false && blockName == "const") {
                nativeShader += "struct _Constants {\n";
                
                if ((constBlockLength = formVarsBlock(input, nativeShader)) == 0) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' has ill-formed 'const' block\n", name);
                    completed = false;
                    break;
                }
                nativeShader += "};\n";
                constBlockDone = true;
                continue;
            }
            if (inoutBlockDone == false && blockName == "inout") {
                nativeShader += "struct _InOut {\n    float4 position [[position]];\n";
                
                if (formVarsBlock(input, nativeShader) == 0) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' has ill-formed 'inout' block\n", name);
                    completed = false;
                    break;
                }
                nativeShader += "};\n";
                inoutBlockDone = true;
                continue;
            }
            if (vssrcBlockDone == false && blockName == "vssrc") {
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
                formatTable[] = { // index is RenderingShaderInputFormat value
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
                    {"int1",   4,  MTLVertexFormatUInt},
                    {"int2",   8,  MTLVertexFormatUInt2},
                    {"int3",   12, MTLVertexFormatUInt3},
                    {"int4",   16, MTLVertexFormatUInt4}
                };

                std::string vertexIDName;
                std::string instanceIDName;
                std::size_t index = 0;
                std::uint32_t offset = 0;
                
                for (const auto &item : vertex) {
                    if (item.format == RenderingShaderInputFormat::ID) {
                        vertexIDName = std::string("vertex_") + item.name;
                    }
                    else {
                        NativeFormat &fmt = formatTable[int(item.format)];
                        vdesc.attributes[index].bufferIndex = 0;
                        vdesc.attributes[index].offset = offset;
                        vdesc.attributes[index].format = fmt.nativeFormat;
                        nativeShader += indent + fmt.nativeTypeName + " " + item.name + " [[attribute(" + std::to_string(index) + ")]];\n";
                        index++;
                        offset += fmt.size;
                    }
                }
                vdesc.layouts[0].stride = offset;
                vdesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
                vdesc.layouts[0].stepRate = 1;;
                offset = 0;
                
                for (const auto &item : instance) {
                    if (item.format == RenderingShaderInputFormat::ID) {
                        instanceIDName = std::string("instance_") + item.name;
                    }
                    else {
                        NativeFormat &fmt = formatTable[int(item.format)];
                        vdesc.attributes[index].bufferIndex = 1;
                        vdesc.attributes[index].offset = offset;
                        vdesc.attributes[index].format = fmt.nativeFormat;
                        nativeShader += indent + fmt.nativeTypeName + " " + item.name + " [[attribute(" + std::to_string(index) + ")]];\n";
                        index++;
                        offset += fmt.size;
                    }
                }
                vdesc.layouts[1].stride = offset;
                vdesc.layouts[1].stepFunction = MTLVertexStepFunctionPerInstance;
                vdesc.layouts[1].stepRate = 1;

                nativeShader += "};\nvertex _InOut main_vertex(\n";

                if (vertexIDName.length()) {
                    nativeShader += "    unsigned int " + vertexIDName + " [[vertex_id]],\n";
                }
                if (instanceIDName.length()) {
                    nativeShader += "    unsigned int " + instanceIDName + " [[instance_id]],\n";
                }
                if (vdesc.layouts[0].stride) { // if there is something but vertex_id
                    nativeShader += "    _VSIn input [[stage_in]],\n";
                }
                if (constBlockLength) {
                    nativeShader += "    constant _Constants &constants [[buffer(2)]],\n";
                }
                    
                nativeShader += "    constant _FrameData &framedata [[buffer(3)]])\n{\n    _InOut output;\n";
                std::string codeBlock;
                
                if (formCodeBlock(input, codeBlock) == false) {
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
            if (fssrcBlockDone == false && blockName == "fssrc") {
                if (vssrcBlockDone == false) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' : 'vssrc' block must be defined before 'fssrc'\n", name);
                    completed = false;
                    break;
                }
                nativeShader += "fragment float4 main_fragment(\n    _InOut input [[stage_in]],\n";
                
                if (constBlockLength) {
                    nativeShader += "    constant _Constants &constants [[buffer(0)]],\n";
                }
                
                nativeShader += "    constant _FrameData &framedata [[buffer(1)]])\n{\n    float4 output_color;\n";
                std::string codeBlock;

                if (formCodeBlock(input, codeBlock) == false) {
                    _platform->logError("[MetalRendering::createShader] shader '%s' has uncompleted 'fssrc' block\n", name);
                    completed = false;
                    break;
                }
                
                replacePattern(codeBlock, "frame_", "framedata.", "._");
                replacePattern(codeBlock, "const_", "constants.", "._");
                replacePattern(codeBlock, "input_", "input.", "._");
                nativeShader += codeBlock;
                nativeShader += "    return output_color;\n}\n";
                fssrcBlockDone = true;
                continue;
            }
            
            _platform->logError("[MetalRendering::createShader] shader '%s' has unexpected '%s' block\n", name, blockName.data());
        }
        
        nativeShader = makeLines(nativeShader);
        printf("----------\n%s\n------------\n", nativeShader.data());
        
        if (completed && vssrcBlockDone && fssrcBlockDone) {
            @autoreleasepool {
                NSError *nsError = nil;
                MTLCompileOptions* compileOptions = [MTLCompileOptions new];
                compileOptions.languageVersion = MTLLanguageVersion2_0;
                compileOptions.fastMathEnabled = true;
                
                id<MTLBuffer> constBuffer = nil;
                id<MTLLibrary> library = [_device newLibraryWithSource:[NSString stringWithUTF8String:nativeShader.data()] options:compileOptions error:&nsError];
                
                if (constBlockLength) {
                    constBuffer = [_device newBufferWithLength:constBlockLength options:MTLResourceStorageModeShared];
                }
                if (library) {
                    result = std::make_shared<MetalShader>(library, vdesc, constBuffer);
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

    RenderingTexture2DPtr MetalRendering::createTexture(RenderingTextureFormat format, std::uint32_t w, std::uint32_t h, const std::uint8_t * const *mips, std::uint32_t mcount) {
        std::shared_ptr<RenderingTexture2D> result;

        return result;
    }

    RenderingDataPtr MetalRendering::createData(const void *data, std::uint32_t count, std::uint32_t stride) {
        id<MTLBuffer> buffer = [_device newBufferWithBytes:data length:(count * stride) options:MTLResourceStorageModeShared];
        return std::make_shared<MetalData>(buffer, count, stride);
    }
    
    void MetalRendering::beginPass(const char *name, const RenderingShaderPtr &shader, const void *constants) {
        if (_view == nil) {
            _view = (__bridge MTKView *)_platform->attachNativeRenderingContext((__bridge void *)_device);
        }
        if (_view) {
            if (_currentCommandBuffer == nil) {
                _currentCommandBuffer = [_commandQueue commandBuffer];
            }
            
            _currentPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
            _currentPassDescriptor.colorAttachments[0].texture = _view.currentDrawable.texture;
            //_currentPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
            _currentPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
            _currentPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.7, 0.7, 0.7, 1.0);
            _currentCommandEncoder = [_currentCommandBuffer renderCommandEncoderWithDescriptor:_currentPassDescriptor];
            
            const MetalShader *platformShader = static_cast<const MetalShader *>(shader.get());
            id<MTLRenderPipelineState> state = nil;
            
            if (platformShader) {
                auto index = _pipelineStates.find(name);
                if (index == _pipelineStates.end()) {
                    NSError *error;
                    MTLRenderPipelineDescriptor *desc = [MTLRenderPipelineDescriptor new];

                    desc.vertexFunction = platformShader->getVertexShader();
                    desc.vertexDescriptor = platformShader->getVertexDesc();
                    desc.fragmentFunction = platformShader->getFragmentShader();
                    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
                    
                    state = [_device newRenderPipelineStateWithDescriptor:desc error:&error];
                    
                    if (state) {
                        _pipelineStates.emplace(name, state);
                    }
                    else {
                        _platform->logError("[MetalRendering::beginPass] %s\n", [[error localizedDescription] UTF8String]);
                    }
                }
                else {
                    state = index->second;
                }
                
                if (state) {
                    id<MTLBuffer> constantBuffer = platformShader->getConstantBuffer();
                    
                    if (constantBuffer && constants) {
                        std::memcpy([constantBuffer contents], constants, constantBuffer.length);
                    }
                    
                    [_currentCommandEncoder setRenderPipelineState:state];
                    _currentShader = shader;
                }
            }
        }
    }
    
    void MetalRendering::applyTextures(const RenderingTexture2D * const * textures, std::uint32_t tcount) {
    
    }
    
    void MetalRendering::drawGeometry(const RenderingDataPtr &vertexData, std::uint32_t vcount, RenderingTopology topology) {
        id<MTLBuffer> nativeVertexData = vertexData ? static_cast<const MetalData *>(vertexData.get())->get() : nullptr;
        const MetalShader *platformShader = static_cast<const MetalShader *>(_currentShader.get());
        
        id<MTLBuffer> buffers[4] = {nativeVertexData, nullptr, platformShader->getConstantBuffer(), _frameConstantsBuffer};
        NSUInteger offsets[4] = {0, 0, 0, 0};
        NSRange vrange {0, 4};
        NSRange frange {0, 2};
        
        [_currentCommandEncoder setVertexBuffers:buffers offsets:offsets withRange:vrange];
        [_currentCommandEncoder setFragmentBuffers:buffers + 2 offsets:offsets withRange:frange];
        [_currentCommandEncoder drawPrimitives:g_topologies[int(topology)] vertexStart:0 vertexCount:vcount];
    }

    void MetalRendering::drawGeometry(const RenderingDataPtr &vertexData, const RenderingDataPtr &instanceData, std::uint32_t vcount, std::uint32_t icount, RenderingTopology topology) {
        id<MTLBuffer> nativeVertexData = vertexData ? static_cast<const MetalData *>(vertexData.get())->get() : nullptr;
        id<MTLBuffer> nativeInstanceData = instanceData ? static_cast<const MetalData *>(instanceData.get())->get() : nullptr;
        const MetalShader *platformShader = static_cast<const MetalShader *>(_currentShader.get());
        
        id<MTLBuffer> buffers[4] = {nativeVertexData, nativeInstanceData, platformShader->getConstantBuffer(), _frameConstantsBuffer};
        NSUInteger offsets[4] = {0, 0, 0, 0};
        NSRange vrange {0, 4};
        NSRange frange {0, 2};
        
        [_currentCommandEncoder setVertexBuffers:buffers offsets:offsets withRange:vrange];
        [_currentCommandEncoder setFragmentBuffers:buffers + 2 offsets:offsets withRange:frange];
        [_currentCommandEncoder drawPrimitives:g_topologies[int(topology)] vertexStart:0 vertexCount:vcount instanceCount:icount];
    }

    void MetalRendering::endPass() {
        [_currentCommandEncoder endEncoding];
        _currentShader = nullptr;
    }
    
    void MetalRendering::presentFrame() {
        if (_view && _currentCommandBuffer) {
            [_currentCommandBuffer presentDrawable:_view.currentDrawable];
            [_currentCommandBuffer commit];
            _currentCommandBuffer = nil;
        }
    }

    void MetalRendering::getFrameBufferData(std::uint8_t *imgFrame) {}
}

namespace foundation {
    std::shared_ptr<RenderingInterface> RenderingInterface::instance(const std::shared_ptr<PlatformInterface> &platform) {
        return std::make_shared<MetalRendering>(platform);
    }
}

#endif // PLATFORM_IOS
