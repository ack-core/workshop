
#ifdef PLATFORM_WASM
#include "util.h"
#include "rendering_wasm.h"

#include <GLES3/gl3.h>

// From js
extern "C" {
    auto webgl_createProgram(const std::uint16_t *vsrc, std::size_t vlen, const std::uint16_t *fsrc, std::size_t flen) -> WebGLId;
    auto webgl_createData(const void *layout, std::uint32_t layoutLen, const void *data, std::uint32_t dataLen, std::uint32_t stride, const std::uint32_t *idx, std::uint32_t icount) -> WebGLId;
    auto webgl_createTexture(GLenum format, GLenum internal, GLenum type, std::uint32_t sz, std::uint32_t w, std::uint32_t h, const std::uint32_t *mipAdds, std::uint32_t mipCount) -> WebGLId;
    auto webgl_createTarget(GLenum format, GLenum internal, GLenum type, std::uint32_t w, std::uint32_t h, std::uint32_t count, bool enableDepth, WebGLId *textures) -> WebGLId;
    void webgl_viewPort(std::uint32_t w, std::uint32_t h);
    void webgl_beginTarget(WebGLId target, WebGLId depth, GLenum cmask, float r, float g, float b, float a, float d);
    void webgl_endTarget(WebGLId target, WebGLId depth);
    void webgl_applyShader(WebGLId shader, std::uint32_t ztype, std::uint32_t btype);
    void webgl_applyConstants(std::uint32_t index, const void *drawConstants, std::uint32_t byteLength);
    void webgl_applyTexture(std::uint32_t index, WebGLId texture, int samplingType);
    void webgl_drawDefault(WebGLId data, std::uint32_t vertexCount, std::uint32_t instanceCount, GLenum topology);
    void webgl_drawIndexed(WebGLId data, std::uint32_t indexCount, std::uint32_t instanceCount, GLenum topology);
    void webgl_drawWithRepeat(WebGLId data, std::uint32_t attrCount, std::uint32_t instanceCount, std::uint32_t vertexCount, std::uint32_t totalInstCount, GLenum topology);
    void webgl_deleteProgram(WebGLId id);
    void webgl_deleteData(WebGLId id);
    void webgl_deleteTexture(WebGLId id);
    void webgl_deleteTarget(WebGLId id);
}

namespace {
    static const GLenum g_topologies[] = {
        GL_POINTS,
        GL_LINES,
        GL_LINE_STRIP,
        GL_TRIANGLES,
        GL_TRIANGLE_STRIP
    };
    
    static const std::uint32_t MAX_TEXTURES = 4;
    static const std::uint32_t FRAME_CONST_BINDING_INDEX = 0;
    static const std::uint32_t DRAW_CONST_BINDING_INDEX = 1;
    static const std::uint32_t VERTEX_IN_BINDING_START = 2;
    
    std::uint32_t roundTo4096(std::uint32_t value) {
        std::uint32_t result = ((value - std::uint32_t(1)) & ~std::uint32_t(4095)) + 4096;
        return result;
    }
    
    struct NativeVertexFormat {
        const char *nativeTypeName;
        std::uint32_t size;
        std::uint32_t components;
    }
    g_formatConversionTable[] = { // index is RenderShaderInputFormat value
        {"vec2",  4,  2},
        {"vec4",  8,  4},
        {"float", 4,  1},
        {"vec2",  8,  2},
        {"vec3",  12, 3},
        {"vec4",  16, 4},
        {"ivec2", 4,  2},
        {"ivec4", 8,  4},
        {"uvec2", 4,  2},
        {"uvec4", 8,  4},
        {"vec2",  4,  2},
        {"vec4",  8,  4},
        {"vec2",  4,  2},
        {"vec4",  8,  4},
        {"uvec4", 4,  4},
        {"vec4",  4,  4},
        {"int",   4,  1},
        {"ivec2", 8,  2},
        {"ivec3", 12, 3},
        {"ivec4", 16, 4},
        {"uint",  4,  1},
        {"uvec2", 8,  2},
        {"uvec3", 12, 3},
        {"uvec4", 16, 4},
    };

    struct NativeTextureFormat {
        GLenum format;
        GLenum internal;
        GLenum type;
        std::uint32_t size;
    }
    g_textureFormatTable[] = { // index is RenderTextureFormat value
        {GL_RED,   GL_R8,      GL_UNSIGNED_BYTE, 1},
        {GL_RED,   GL_R16F,    GL_HALF_FLOAT, 2},
        {GL_RED,   GL_R32F,    GL_FLOAT, 4},
        {GL_RG,    GL_RG8,     GL_UNSIGNED_BYTE, 2},
        {GL_RG,    GL_RG16F,   GL_HALF_FLOAT, 4},
        {GL_RG,    GL_RG32F,   GL_FLOAT, 8},
        {GL_RGBA,  GL_RGBA8,   GL_UNSIGNED_BYTE, 4},
        {GL_RGBA,  GL_RGBA16F, GL_HALF_FLOAT, 8},
        {GL_RGBA,  GL_RGBA32F, GL_FLOAT, 16},
    };
}

namespace foundation {
    WASMShader::WASMShader(WebGLId shader, const InputLayout &layout, std::uint32_t constBufferLength)
        : _shader(shader)
        , _constBufferLength(constBufferLength)
        , _inputLayout(layout)
    {
    }
    WASMShader::~WASMShader() {
        webgl_deleteProgram(_shader);
    }
    const InputLayout &WASMShader::getInputLayout() const {
        return _inputLayout;
    }
    std::uint32_t WASMShader::getConstBufferLength() const {
        return _constBufferLength;
    }
    WebGLId WASMShader::getWebGLShader() const {
        return _shader;
    }
}

namespace foundation {
    WASMData::WASMData(WebGLId data, std::uint32_t vcount, std::uint32_t icount, std::uint32_t stride)
        : _data(data)
        , _vcount(vcount)
        , _icount(icount)
        , _stride(stride)
    {
    }
    WASMData::~WASMData() {
        webgl_deleteData(_data);
    }
    std::uint32_t WASMData::getVertexCount() const {
        return _vcount;
    }
    std::uint32_t WASMData::getIndexCount() const {
        return _icount;
    }
    std::uint32_t WASMData::getStride() const {
        return _stride;
    }
    WebGLId WASMData::getWebGLData() const {
        return _data;
    }
}

namespace foundation {
    WASMTexture::WASMTexture(WebGLId texture, RenderTextureFormat fmt, std::uint32_t w, std::uint32_t h, std::uint32_t mipCount)
        : _texture(texture)
        , _format(fmt)
        , _width(w)
        , _height(h)
        , _mipCount(mipCount)
    {
    }
    WASMTexture::~WASMTexture() {
        webgl_deleteTexture(_texture);
    }
    std::uint32_t WASMTexture::getWidth() const {
        return _width;
    }
    std::uint32_t WASMTexture::getHeight() const {
        return _height;
    }
    std::uint32_t WASMTexture::getMipCount() const {
        return _mipCount;
    }
    RenderTextureFormat WASMTexture::getFormat() const {
        return _format;
    }
    WebGLId WASMTexture::getWebGLTexture() const {
        return _texture;
    }
}

namespace foundation {
    WASMTarget::WASMTarget(WebGLId target, const WebGLId *textures, std::uint32_t count, WebGLId depth, RenderTextureFormat fmt, std::uint32_t w, std::uint32_t h)
        : _target(target)
        , _count(count)
        , _width(w)
        , _height(h)
    {
        for (unsigned i = 0; i < count; i++) {
            _textures[i] = std::make_shared<RTTexture>(*this, fmt, textures[i]);
        }
        _depth = std::make_shared<RTTexture>(*this, RenderTextureFormat::R32F, depth);
    }
    WASMTarget::RTTexture::~RTTexture() {
        webgl_deleteTexture(_texture);
    }
    WASMTarget::~WASMTarget() {
        webgl_deleteTarget(_target);
    }
    std::uint32_t WASMTarget::getWidth() const {
        return _width;
    }
    std::uint32_t WASMTarget::getHeight() const {
        return _height;
    }
    RenderTextureFormat WASMTarget::getFormat() const {
        return _textures[0]->getFormat();
    }
    std::uint32_t WASMTarget::getTextureCount() const {
        return _count;
    }
    const std::shared_ptr<RenderTexture> &WASMTarget::getTexture(unsigned index) const {
        return _textures[index];
    }
    const std::shared_ptr<RenderTexture> &WASMTarget::getDepth() const {
        return _depth;
    }
    WebGLId WASMTarget::getWebGLTarget() const {
        return _target;
    }
}

namespace foundation {
    WASMRendering::WASMRendering(const std::shared_ptr<PlatformInterface> &platform) : _platform(platform) {
        _platform->logMsg("[RENDER] Initialization : WebGL");
        _frameConstants = std::make_unique<FrameConstants>();
        _uploadBufferLength = 1024;
        _uploadBufferData = std::make_unique<std::uint8_t[]>(_uploadBufferLength);
        _platform->logMsg("[RENDER] Initialization : complete");
    }
    
    WASMRendering::~WASMRendering() {}
    
    void WASMRendering::updateFrameConstants(const math::transform3f &vp, const math::transform3f &svp, const math::transform3f &ivp, const math::vector3f &camPos, const math::vector3f &camDir) {
        _frameConstants->plmVPMatrix = vp;
        _frameConstants->stdVPMatrix = svp;
        _frameConstants->invVPMatrix = ivp;
        _frameConstants->cameraPosition.xyz = camPos;
        _frameConstants->cameraDirection.xyz = camDir;
    }
        
    RenderShaderPtr WASMRendering::createShader(const char *name, const char *src, const InputLayout &layout) {
        std::shared_ptr<RenderShader> result;
        util::strstream input(src, strlen(src));
        const std::string indent = "    ";

        if (_shaderNames.find(name) == _shaderNames.end()) {
            _shaderNames.emplace(name);
        }
        else {
            _platform->logError("[WASMRendering::createShader] shader name '%s' already used\n", name);
        }
        
        static const char *SEPARATORS = " ;,=-+*/{(\n\t\r";
        static const std::size_t TYPES_COUNT = 14;
        static const std::size_t TYPES_PASS_COUNT = 4;
        static const shaderUtils::ShaderTypeInfo TYPE_SIZE_TABLE[TYPES_COUNT] = {
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
        
        struct ShaderSrc {
            std::unique_ptr<std::uint16_t[]> src;
            std::size_t length;
        }
        nativeShaderVS, nativeShaderFS;
        
        auto createNativeSrc = [](ShaderSrc &output, const std::string &src) {
            output.length = src.length();
            output.src = std::make_unique<std::uint16_t[]>(src.length());
            for (std::size_t i = 0; i < src.length(); i++) {
                output.src[i] = src[i];
            }
        };
        auto formFixedBlock = [&indent](util::strstream &stream, std::string &output) {
            std::string varname, arg;
            
            while (stream >> varname && varname[0] != '}') {
                if (stream >> util::sequence(":") >> arg >> util::sequence("=")) {
                    std::size_t elementSize = 0, elementCount = shaderUtils::getArrayMultiply(varname);
                    std::string nativeTypeName;
                    
                    if (shaderUtils::shaderGetTypeSize(arg, TYPE_SIZE_TABLE, TYPES_COUNT, nativeTypeName, elementSize)) {
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
        };
        auto formVarsBlock = [&indent](util::strstream &stream, std::string &output, std::size_t allowedTypeCount) {
            std::string varname, arg;
            std::uint32_t totalLength = 0;

            while (stream >> varname && varname[0] != '}') {
                if (stream >> util::sequence(":") >> arg) {
                    std::size_t elementSize = 0, elementCount = shaderUtils::getArrayMultiply(varname);
                    std::string nativeTypeName;

                    if (shaderGetTypeSize(arg, TYPE_SIZE_TABLE, allowedTypeCount, nativeTypeName, elementSize)) {
                        output += indent + "mediump " + nativeTypeName + " " + varname + ";\n";
                        totalLength += elementSize * elementCount;
                        continue;
                    }
                }

                return std::uint32_t(0);
            }

            return totalLength;
        };
        auto transformCode = [](std::string &target) {
            shaderUtils::replace(target, "float", "vec", SEPARATORS, "234");
            shaderUtils::replace(target, "int", "ivec", SEPARATORS, "234");
            shaderUtils::replace(target, "uint", "uvec", SEPARATORS, "234");
            shaderUtils::replace(target, "float1", "float", SEPARATORS, SEPARATORS);
            shaderUtils::replace(target, "int1", "int", SEPARATORS, SEPARATORS);
            shaderUtils::replace(target, "uint1", "uint", SEPARATORS, SEPARATORS);
            shaderUtils::replace(target, "const_", "constants.", SEPARATORS);
            shaderUtils::replace(target, "frame_", "framedata.", SEPARATORS);
        };
        
        std::string shaderDefines =
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
            
        std::string shaderFixed;
        std::string shaderFunctions;
        std::string shaderVSOutput;
        std::string shaderFSInput;
        
        std::string blockName;
        std::uint32_t constBlockLength = 0;
        
        bool completed = true;
        bool fixedBlockDone = false;
        bool constBlockDone = false;
        bool inoutBlockDone = false;
        bool vssrcBlockDone = false;
        bool fssrcBlockDone = false;
        
        while (input >> blockName) {
            if (constBlockDone == false && blockName == "const" && (input >> util::sequence("{"))) {
                shaderDefines += "layout(std140) uniform _Constants {\n";
                
                if ((constBlockLength = formVarsBlock(input, shaderDefines, TYPES_PASS_COUNT)) == 0) {
                    _platform->logError("[WASMRendering::createShader] shader '%s' has ill-formed 'const' block\n", name);
                    completed = false;
                    break;
                }
                shaderDefines += "}\nconstants;\n\n";
                constBlockDone = true;
                continue;
            }
            if (fixedBlockDone == false && blockName == "fixed" && (input >> util::sequence("{"))) {
                if (formFixedBlock(input, shaderFixed) == false) {
                    _platform->logError("[WASMRendering::createShader] shader '%s' has ill-formed 'fixed' block\n", name);
                    completed = false;
                    break;
                }
                
                fixedBlockDone = true;
                continue;
            }
            if (inoutBlockDone == false && blockName == "inout" && (input >> util::sequence("{"))) {
                shaderVSOutput += "out struct _InOut {\n";
                shaderFSInput  += "in struct _InOut {\n";
                
                std::string varsBlock;
                if (formVarsBlock(input, varsBlock, TYPES_COUNT) == 0) {
                    _platform->logError("[WASMRendering::createShader] shader '%s' has ill-formed 'inout' block\n", name);
                    completed = false;
                    break;
                }
                
                shaderVSOutput += varsBlock;
                shaderVSOutput += "}\npassing;\n\n";
                shaderFSInput += varsBlock;
                shaderFSInput += "}\npassing;\n\n";
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
                        shaderFunctions += funcReturnType + " " + funcName + "(" + funcSignature + ") {\n";
                        shaderFunctions += codeBlock;
                        shaderFunctions += "}\n\n";
                    }
                    else {
                        _platform->logError("[WASMRendering::createShader] shader '%s' has uncompleted 'fndef' block\n", name);
                        completed = false;
                        break;
                    }
                }
                else {
                    _platform->logError("[WASMRendering::createShader] shader '%s' has invalid 'fndef' block\n", name);
                    completed = false;
                    break;
                }
                continue;
            }
            if (vssrcBlockDone == false && blockName == "vssrc" && (input >> util::sequence("{"))) {
                std::string shaderVS = shaderDefines;
                
                transformCode(shaderFunctions);
                shaderVS += "#define output_position gl_Position\n\n";
                shaderVS = shaderVS + shaderFixed + shaderFunctions + shaderVSOutput;
                std::string codeBlock;
                
                if (shaderUtils::formCodeBlock(indent, input, codeBlock) == false) {
                    _platform->logError("[WASMRendering::createShader] shader '%s' has uncompleted 'vssrc' block\n", name);
                    completed = false;
                    break;
                }
                
                for (std::size_t i = 0; i < layout.attributes.size(); i++) {
                    const InputLayout::Attribute &attribute = layout.attributes[i];
                    const std::string type = std::string(g_formatConversionTable[int(attribute.format)].nativeTypeName);
                    shaderVS += "layout(location = " + std::to_string(i) + ") in " + type + " vertex_" + std::string(attribute.name) + ";\n";
                }
                if (layout.attributes.size()) {
                    shaderVS += "\n";
                }
                
                transformCode(codeBlock);
                shaderUtils::replace(codeBlock, "output_", "passing.", SEPARATORS, {"output_position"});
                
                if (layout.repeat > 1) {
                    shaderVS += "uniform int _instance_count;\n";
                    shaderVS += "\n#define repeat_ID gl_VertexID\n";
                    shaderVS += "\nvoid main() {\n    int vertex_ID = gl_InstanceID % _instance_count;\n    int instance_ID = gl_InstanceID / _instance_count;\n";
                    //shaderVS += "\nvoid main() {\n    int vertex_ID = gl_InstanceID / _instance_count;\n    int instance_ID = gl_InstanceID % _instance_count;\n";
                }
                else {
                    shaderVS += "\n#define repeat_ID 0\n#define vertex_ID gl_VertexID\n#define instance_ID gl_InstanceID\n";
                    shaderVS += "\nvoid main() {\n";
                }
                                
                shaderVS += codeBlock;
                shaderVS += "    gl_Position.y *= -1.0;\n";
                shaderVS += "}\n\n";
                
                shaderVS = shaderUtils::makeLines(shaderVS);
                createNativeSrc(nativeShaderVS, shaderVS);
                //_platform->logMsg("---------- begin ----------\n%s\n----------- end -----------\n", shaderVS.data());
                
                vssrcBlockDone = true;
                continue;
            }
            if (fssrcBlockDone == false && blockName == "fssrc" && (input >> util::sequence("{"))) {
                if (vssrcBlockDone == false) {
                    _platform->logError("[WASMRendering::createShader] shader '%s' : 'vssrc' block must be defined before 'fssrc'\n", name);
                    completed = false;
                    break;
                }
                
                std::string shaderFS = shaderDefines + shaderFixed + shaderFunctions + shaderFSInput;
                std::string codeBlock;
                
                if (shaderUtils::formCodeBlock(indent, input, codeBlock) == false) {
                    _platform->logError("[WASMRendering::createShader] shader '%s' has uncompleted 'fssrc' block\n", name);
                    completed = false;
                    break;
                }
                
                transformCode(codeBlock);
                shaderUtils::replace(codeBlock, "input_", "passing.", SEPARATORS, {"input_position"});
                
                shaderFS += "out vec4 output_color[4];\n\n";
                shaderFS += "void main() {\n";
                shaderFS += codeBlock;
                shaderFS += "}\n\n";
                
                shaderFS = shaderUtils::makeLines(shaderFS);
                createNativeSrc(nativeShaderFS, shaderFS);

                //_platform->logMsg("---------- begin ----------\n%s\n----------- end -----------\n", shaderFS.data());
                fssrcBlockDone = true;
                continue;
            }
            
            _platform->logError("[WASMRendering::createShader] shader '%s' has unexpected '%s' block\n", name, blockName.data());
        }
                
        if (completed && vssrcBlockDone && fssrcBlockDone) {
            WebGLId webglShader = webgl_createProgram(nativeShaderVS.src.get(), nativeShaderVS.length, nativeShaderFS.src.get(), nativeShaderFS.length);
            result = std::make_shared<WASMShader>(webglShader, layout, constBlockLength);
        }
        else if(vssrcBlockDone == false) {
            _platform->logError("[WASMRendering::createShader] shader '%s' missing 'vssrc' block\n", name);
        }
        else if(fssrcBlockDone == false) {
            _platform->logError("[WASMRendering::createShader] shader '%s' missing 'fssrc' block\n", name);
        }

        return result;
    }
    
    RenderTexturePtr WASMRendering::createTexture(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, const std::initializer_list<const void *> &mipsData) {
        RenderTexturePtr result = nullptr;
        
        if (w && h && mipsData.size()) {
            const NativeTextureFormat &fmt = g_textureFormatTable[int(format)];
            
            static_assert(sizeof(std::size_t) == sizeof(std::uint32_t), "that code assumes 32-bit webassembly environment");
            std::uint8_t *buffer = _getUploadBuffer(2 * w * h * fmt.size + 4 * mipsData.size());
            std::uint8_t *memory = buffer + 4 * mipsData.size();
            std::uint32_t *mptrs = reinterpret_cast<std::uint32_t *>(buffer);
            std::uint32_t offset = 0;
            
            for (std::size_t i = 0; i < mipsData.size(); i++) {
                std::uint32_t miplen = (w >> i) * (h >> i) * fmt.size;
                memcpy(memory + offset, mipsData.begin()[i], miplen);
                mptrs[i] = std::uint32_t(memory + offset);
                offset += miplen;
            }
            
            WebGLId webglTexture = webgl_createTexture(fmt.format, fmt.internal, fmt.type, fmt.size, w, h, mptrs, mipsData.size());
            result = std::make_shared<WASMTexture>(webglTexture, format, w, h, mipsData.size());
        }
        
        return result;
    }
    
    RenderTargetPtr WASMRendering::createRenderTarget(RenderTextureFormat format, std::uint32_t count, std::uint32_t w, std::uint32_t h, bool withZBuffer) {
        RenderTargetPtr result = nullptr;
        
        if (count && w && h) {
            const NativeTextureFormat &fmt = g_textureFormatTable[int(format)];
            WebGLId *textureIds = reinterpret_cast<WebGLId *>(_getUploadBuffer(sizeof(WebGLId) * (count + 1)));
            WebGLId webglTarget = webgl_createTarget(fmt.format, fmt.internal, fmt.type, w, h, count, withZBuffer, textureIds);
            result = std::make_shared<WASMTarget>(webglTarget, &textureIds[1], count, textureIds[0], format, w, h);
        }
        
        return result;
    }
    
    RenderDataPtr WASMRendering::createData(const void *data, const foundation::InputLayout &layout, std::uint32_t vcnt, const std::uint32_t *indexes, std::uint32_t icnt) {
        RenderDataPtr result = nullptr;
        
        if (indexes && layout.repeat > 1) {
            _platform->logError("[WASMRendering::createData] Vertex repeat is incompatible with indexed data");
            return nullptr;
        }
        if (vcnt && layout.attributes.empty() == false) {
            std::uint32_t stride = 0;
            for (std::size_t i = 0; i < layout.attributes.size(); i++) {
                stride += g_formatConversionTable[int(layout.attributes[i].format)].size;
            }
            
            std::uint8_t *memory = _getUploadBuffer(layout.attributes.size() + stride * vcnt);
            for (std::size_t i = 0; i < layout.attributes.size(); i++) {
                memory[i] = std::uint8_t(layout.attributes[i].format);
            }
            
            memcpy(memory + layout.attributes.size(), data, stride * vcnt);
            WebGLId webglData = webgl_createData(memory, layout.attributes.size(), memory + layout.attributes.size(), vcnt * stride, stride, indexes, icnt);
            result = std::make_shared<WASMData>(webglData, vcnt, icnt, stride);
        }
        
        return result;
    }
    
    float WASMRendering::getBackBufferWidth() const {
        return _platform->getScreenWidth();
    }
    
    float WASMRendering::getBackBufferHeight() const {
        return _platform->getScreenHeight();
    }
    
    math::transform3f WASMRendering::getStdVPMatrix() const {
        return _frameConstants->stdVPMatrix;
    }
    
    void WASMRendering::forTarget(const RenderTargetPtr &target, const RenderTexturePtr &depth, const std::optional<math::color> &rgba, util::callback<void(RenderingInterface &)> &&pass) {
        const WASMTarget *platformTarget = static_cast<const WASMTarget *>(target.get());
        WebGLId platformDepth = 0;
        WebGLId glTarget = 0;
        GLenum cmask = 0;
        
        if (platformTarget) {
            _frameConstants->rtBounds.x = platformTarget->getWidth();
            _frameConstants->rtBounds.y = platformTarget->getHeight();
            glTarget = platformTarget->getWebGLTarget();
        }
        else {
            _frameConstants->rtBounds.x = _platform->getScreenWidth();
            _frameConstants->rtBounds.y = _platform->getScreenHeight();
        }
        
        if (depth) {
            platformDepth = static_cast<const WASMTexture *>(depth.get())->getWebGLTexture();
        }
        else {
            cmask = cmask | GL_DEPTH_BUFFER_BIT;
        }
        if (rgba.has_value()) {
            cmask = cmask | GL_COLOR_BUFFER_BIT;
        }
        
        webgl_beginTarget(glTarget, platformDepth, cmask, rgba->r, rgba->g, rgba->b, rgba->a, 0.0f);
        webgl_viewPort(_frameConstants->rtBounds.x, _frameConstants->rtBounds.y);
        webgl_applyConstants(FRAME_CONST_BINDING_INDEX, _frameConstants.get(), sizeof(FrameConstants));
        _isForTarget = true;
        
        pass(*this);
        
        _isForTarget = false;
        webgl_endTarget(glTarget, platformDepth);
    }
    
    void WASMRendering::applyShader(const RenderShaderPtr &shader, foundation::RenderTopology topology, BlendType blendType, DepthBehavior depthBehavior) {
        if (_isForTarget && shader) {
            _topology = topology;
            _currentShader = std::static_pointer_cast<WASMShader>(shader);
            webgl_applyShader(_currentShader->getWebGLShader(), std::uint32_t(depthBehavior), std::uint32_t(blendType));
        }
    }
    
    void WASMRendering::applyShaderConstants(const void *constants) {
        if (_currentShader) {
            const std::uint32_t requiredLength = _currentShader->getConstBufferLength();
            
            if (requiredLength) {
                std::uint8_t *buffer = _getUploadBuffer(requiredLength);
                memcpy(buffer, constants, _currentShader->getConstBufferLength());
                webgl_applyConstants(DRAW_CONST_BINDING_INDEX, buffer, _currentShader->getConstBufferLength());
            }
        }
    }
    
    void WASMRendering::applyTextures(const std::initializer_list<std::pair<const RenderTexturePtr, SamplerType>> &textures) {
        if (_currentShader) {
            for (std::size_t i = 0; i < textures.size(); i++) {
                const WASMTexture *platformTexture = static_cast<const WASMTexture *>(textures.begin()[i].first.get());
                webgl_applyTexture(i, platformTexture ? platformTexture->getWebGLTexture() : 0, int(textures.begin()[i].second));
            }
        }
    }
    
    void WASMRendering::draw(std::uint32_t vertexCount) {
        if (_currentShader) {
            const InputLayout &layout = _currentShader->getInputLayout();
            
            if (layout.repeat > 1) {
                webgl_drawDefault(0, layout.repeat, vertexCount, g_topologies[int(_topology)]);
            }
            else {
                webgl_drawDefault(0, vertexCount, 1, g_topologies[int(_topology)]);
            }
        }
    }
    
    void WASMRendering::draw(const RenderDataPtr &inputData, std::uint32_t instanceCount) {
        const WASMData *platformData = static_cast<const WASMData *>(inputData.get());
        
        if (_currentShader) {
            const InputLayout &layout = _currentShader->getInputLayout();
            WebGLId id = 0;
            std::uint32_t vcount = 1;
            
            if (platformData) {
                id = platformData->getWebGLData();
                vcount = platformData->getVertexCount();
            }
            if (layout.repeat > 1) {
                webgl_drawWithRepeat(id, std::uint32_t(layout.attributes.size()), instanceCount, layout.repeat, vcount * instanceCount, g_topologies[int(_topology)]);
            }
            else {
                if (platformData->getIndexCount()) {
                    webgl_drawIndexed(id, platformData->getIndexCount(), instanceCount, g_topologies[int(_topology)]);
                }
                else {
                    webgl_drawDefault(id, vcount, instanceCount, g_topologies[int(_topology)]);
                }
            }
        }
    }
    
    void WASMRendering::presentFrame() {
        for (std::size_t i = 0; i < MAX_TEXTURES; i++) {
            webgl_applyTexture(i, 0, 0);
        }
    }
    
    std::uint8_t *WASMRendering::_getUploadBuffer(std::size_t requiredLength) {
        if (requiredLength > _uploadBufferLength) {
            _uploadBufferLength = roundTo4096(requiredLength);
            _uploadBufferData = std::make_unique<std::uint8_t[]>(_uploadBufferLength);
        }
        
        return _uploadBufferData.get();
    }
    
}

namespace foundation {
    std::shared_ptr<RenderingInterface> RenderingInterface::instance(const std::shared_ptr<PlatformInterface> &platform) {
        return std::make_shared<WASMRendering>(platform);
    }
}

#endif // PLATFORM_WASM
