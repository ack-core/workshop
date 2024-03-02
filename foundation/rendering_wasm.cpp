
#ifdef PLATFORM_WASM
#include "util.h"
#include "rendering_wasm.h"

#include <GLES3/gl3.h>

// TODO: +unique_ptr
// TODO: resource dtors (webglData)

// From js
extern "C" {
    auto webgl_createProgram(const std::uint16_t *vsrc, std::size_t vlen, const std::uint16_t *fsrc, std::size_t flen) -> WebGLId;
    auto webgl_createData(const void *layout, std::uint32_t layoutLen, const void *data, std::uint32_t dataLen, std::uint32_t stride) -> WebGLId;
    auto webgl_createTexture(GLenum format, GLenum internal, GLenum type, std::uint32_t sz, std::uint32_t w, std::uint32_t h, const std::uint32_t *mipAdds, std::uint32_t mipCount) -> WebGLId;
    auto webgl_createTarget(GLenum format, GLenum internal, GLenum type, std::uint32_t w, std::uint32_t h, std::uint32_t count, bool enableDepth, WebGLId *textures) -> WebGLId;
    void webgl_viewPort(std::uint32_t w, std::uint32_t h);
    void webgl_applyState(WebGLId target, WebGLId shader, GLenum cmask, float r, float g, float b, float a, float d, std::uint32_t ztype, std::uint32_t btype);
    void webgl_applyConstants(std::uint32_t index, const void *drawConstants, std::uint32_t byteLength);
    void webgl_applyTexture(std::uint32_t index, WebGLId texture, int samplingType);
    //void webgl_draw(std::uint32_t vertexCount, std::uint32_t instanceCount, GLenum topology);
    void webgl_drawDefault(WebGLId data, std::uint32_t vertexCount, std::uint32_t instanceCount, GLenum topology);
    void webgl_drawWithRepeat(WebGLId data, std::uint32_t attrCount, std::uint32_t instanceCount, std::uint32_t vertexCount, std::uint32_t totalInstCount, GLenum topology);
    
//                webgl_drawDefault: function(buffer, vertexCount, instanceCount, topology) {
//                    glcontext.bindVertexArray(glbuffers[buffer].layout);
//                    glcontext.drawArraysInstanced(topology, 0, vertexCount, instanceCount);
//                },
//                webgl_drawWithRepeat: function(buffer, attrCount, vertexCount, instanceCount, topology) {
//
//    void webgl_bindBuffer(WebGLId data);
//    void webgl_setInstanceCount(std::uint32_t instanceCount);
//    void webgl_draw(std::uint32_t vertexCount, std::uint32_t instanceCount, GLenum topology);
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
    WASMShader::WASMShader(WebGLId shader, InputLayout &&layout, std::uint32_t constBufferLength)
        : _shader(shader)
        , _constBufferLength(constBufferLength)
        , _inputLayout(std::move(layout))
    {
    }
    WASMShader::~WASMShader() {
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
    WASMData::WASMData(WebGLId data, std::uint32_t count, std::uint32_t stride)
        : _data(data)
        , _count(count)
        , _stride(stride)
    {
    }
    WASMData::~WASMData() {

    }
    std::uint32_t WASMData::getCount() const {
        return _count;
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
    WASMTarget::WASMTarget(WebGLId target, const WebGLId *textures, unsigned count, RenderTextureFormat fmt, std::uint32_t w, std::uint32_t h, bool hasDepth)
        : _target(target)
        , _format(fmt)
        , _count(count)
        , _width(w)
        , _height(h)
        , _hasDepth(hasDepth)
    {
        for (unsigned i = 0; i < count; i++) {
            _textures[i] = std::make_shared<RTTexture>(*this, textures[i]);
        }
    }
    WASMTarget::~WASMTarget() {
        
    }
    std::uint32_t WASMTarget::getWidth() const {
        return _width;
    }
    std::uint32_t WASMTarget::getHeight() const {
        return _height;
    }
    RenderTextureFormat WASMTarget::getFormat() const {
        return _format;
    }
    std::uint32_t WASMTarget::getTextureCount() const {
        return _count;
    }
    const std::shared_ptr<RenderTexture> &WASMTarget::getTexture(unsigned index) const {
        return _textures[index];
    }
    WebGLId WASMTarget::getWebGLTarget() const {
        return _target;
    }
    bool WASMTarget::hasDepthBuffer() const {
        return _hasDepth;
    }
}

namespace foundation {
    WASMRendering::WASMRendering(const std::shared_ptr<PlatformInterface> &platform) : _platform(platform) {
        _platform->logMsg("[RENDER] Initialization : WebGL");
        _frameConstants = new FrameConstants;
        _uploadBufferLength = 1024;
        _uploadBufferData = new std::uint8_t [_uploadBufferLength];
        _platform->logMsg("[RENDER] Initialization : complete");
    }
    
    WASMRendering::~WASMRendering() {}
    
    void WASMRendering::updateFrameConstants(const math::transform3f &view, const math::transform3f &proj, const math::vector3f &camPos, const math::vector3f &camDir) {
        _frameConstants->viewMatrix = view;
        _frameConstants->projMatrix = proj;
        _frameConstants->cameraPosition.xyz = camPos;
        _frameConstants->cameraDirection.xyz = camDir;
    }
    
    math::vector2f WASMRendering::getScreenCoordinates(const math::vector3f &worldPosition) {
        const math::transform3f vp = _frameConstants->viewMatrix * _frameConstants->projMatrix;
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
    
    math::vector3f WASMRendering::getWorldDirection(const math::vector2f &screenPosition) {
        const math::transform3f inv = (_frameConstants->viewMatrix * _frameConstants->projMatrix).inverted();
        
        float relScreenPosX = 2.0f * screenPosition.x / _platform->getScreenWidth() - 1.0f;
        float relScreenPosY = 1.0f - 2.0f * screenPosition.y / _platform->getScreenHeight();
        
        const math::vector4f worldPos = math::vector4f(relScreenPosX, relScreenPosY, 0.0f, 1.0f).transformed(inv);
        return math::vector3f(worldPos.x / worldPos.w, worldPos.y / worldPos.w, worldPos.z / worldPos.w).normalized();
    }
    
    RenderShaderPtr WASMRendering::createShader(const char *name, const char *src, InputLayout &&layout) {
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
            const std::uint16_t *src;
            std::size_t length;
        }
        nativeShaderVS, nativeShaderFS;
        
        auto createNativeSrc = [](const std::string &src) {
            std::uint16_t *buffer = new std::uint16_t [src.length()];
            for (std::size_t i = 0; i < src.length(); i++) {
                buffer[i] = src[i];
            }
            return ShaderSrc {buffer, src.length()};
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
            "#define _fract(a) fract(a)\n"
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
            "    mediump mat4 viewMatrix;\n"
            "    mediump mat4 projMatrix;\n"
            "    mediump vec4 cameraPosition;\n"
            "    mediump vec4 cameraDirection;\n"
            "    mediump vec4 rtBounds;\n"
            "}\n"
            "framedata;\n\n";
            
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
                        shaderFunctions += "\n" + funcReturnType + " " + funcName + "(" + funcSignature + ") {\n";
                        shaderFunctions += codeBlock;
                        shaderFunctions += "}\n\n";
                        
                        shaderUtils::replace(shaderFunctions, "float", "vec", SEPARATORS, "234");
                        shaderUtils::replace(shaderFunctions, "int", "ivec", SEPARATORS, "234");
                        shaderUtils::replace(shaderFunctions, "uint", "uvec", SEPARATORS, "234");
                        shaderUtils::replace(shaderFunctions, "float1", "float", SEPARATORS, SEPARATORS);
                        shaderUtils::replace(shaderFunctions, "int1", "int", SEPARATORS, SEPARATORS);
                        shaderUtils::replace(shaderFunctions, "uint1", "uint", SEPARATORS, SEPARATORS);
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
                
                shaderVS += "\n";
                shaderVS += "uniform float _vertical_flip;\n";
                                
                shaderUtils::replace(codeBlock, "float", "vec", SEPARATORS, "234");
                shaderUtils::replace(codeBlock, "int", "ivec", SEPARATORS, "234");
                shaderUtils::replace(codeBlock, "uint", "uvec", SEPARATORS, "234");
                shaderUtils::replace(codeBlock, "float1", "float", SEPARATORS, SEPARATORS);
                shaderUtils::replace(codeBlock, "int1", "int", SEPARATORS, SEPARATORS);
                shaderUtils::replace(codeBlock, "uint1", "uint", SEPARATORS, SEPARATORS);
                
                shaderUtils::replace(codeBlock, "const_", "constants.", SEPARATORS);
                shaderUtils::replace(codeBlock, "frame_", "framedata.", SEPARATORS);
                shaderUtils::replace(codeBlock, "output_", "passing.", SEPARATORS, {"output_position"});
                
                if (layout.repeat > 1) {
                    shaderVS += "uniform int _instance_count;\n";
                    shaderVS += "\n#define repeat_ID gl_VertexID\n";
                    shaderVS += "\nvoid main() {\n    int vertex_ID = gl_InstanceID / _instance_count;\n    int instance_ID = gl_InstanceID % _instance_count;\n";
                }
                else {
                    shaderVS += "\n#define repeat_ID 0\n#define vertex_ID gl_VertexID\n#define instance_ID gl_InstanceID\n";
                    shaderVS += "\nvoid main() {\n";
                }
                                
                shaderVS += codeBlock;
                shaderVS += "    gl_Position.y *= _vertical_flip;\n";
                shaderVS += "}\n\n";
                
                shaderVS = shaderUtils::makeLines(shaderVS);
                nativeShaderVS = createNativeSrc(shaderVS);
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
                
                shaderUtils::replace(codeBlock, "float", "vec", SEPARATORS, "234");
                shaderUtils::replace(codeBlock, "int", "ivec", SEPARATORS, "234");
                shaderUtils::replace(codeBlock, "uint", "uvec", SEPARATORS, "234");
                shaderUtils::replace(codeBlock, "float1", "float", SEPARATORS, SEPARATORS);
                shaderUtils::replace(codeBlock, "int1", "int", SEPARATORS, SEPARATORS);
                shaderUtils::replace(codeBlock, "uint1", "uint", SEPARATORS, SEPARATORS);

                shaderUtils::replace(codeBlock, "const_", "constants.", SEPARATORS);
                shaderUtils::replace(codeBlock, "frame_", "framedata.", SEPARATORS);
                shaderUtils::replace(codeBlock, "input_", "passing.", SEPARATORS, {"input_position"});
                
                shaderFS += "uniform sampler2D _samplers[4];\n";
                shaderFS += "out vec4 output_color[4];\n\n";
                shaderFS += "void main() {\n";
                shaderFS += codeBlock;
                shaderFS += "}\n\n";

                shaderFS = shaderUtils::makeLines(shaderFS);
                nativeShaderFS = createNativeSrc(shaderFS);

                //_platform->logMsg("---------- begin ----------\n%s\n----------- end -----------\n", shaderFS.data());
                fssrcBlockDone = true;
                continue;
            }
            
            _platform->logError("[MetalRendering::createShader] shader '%s' has unexpected '%s' block\n", name, blockName.data());
        }
                
        if (completed && vssrcBlockDone && fssrcBlockDone) {
            layout.repeat = std::max(layout.repeat, std::uint32_t(1));
            WebGLId webglShader = webgl_createProgram(nativeShaderVS.src, nativeShaderVS.length, nativeShaderFS.src, nativeShaderFS.length);
            result = std::make_shared<WASMShader>(webglShader, std::move(layout), constBlockLength);
            delete [] nativeShaderVS.src;
            delete [] nativeShaderFS.src;
        }
        else if(vssrcBlockDone == false) {
            _platform->logError("[MetalRendering::createShader] shader '%s' missing 'vssrc' block\n", name);
        }
        else if(fssrcBlockDone == false) {
            _platform->logError("[MetalRendering::createShader] shader '%s' missing 'fssrc' block\n", name);
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
    
    RenderTargetPtr WASMRendering::createRenderTarget(RenderTextureFormat format, unsigned count, std::uint32_t w, std::uint32_t h, bool withZBuffer) {
        RenderTargetPtr result = nullptr;
        
        if (count && w && h) {
            const NativeTextureFormat &fmt = g_textureFormatTable[int(format)];
            WebGLId *textureIds = reinterpret_cast<WebGLId *>(_getUploadBuffer(sizeof(WebGLId) * count));
            WebGLId webglTarget = webgl_createTarget(fmt.format, fmt.internal, fmt.type, w, h, count, withZBuffer, textureIds);
            result = std::make_shared<WASMTarget>(webglTarget, textureIds, count, format, w, h, withZBuffer);
        }
        
        return result;
    }
    
    RenderDataPtr WASMRendering::createData(const void *data, const InputLayout &layout, std::uint32_t count) {
        RenderDataPtr result = nullptr;
        
        if (count && layout.attributes.empty() == false) {
            std::uint32_t stride = 0;
            for (std::size_t i = 0; i < layout.attributes.size(); i++) {
                stride += g_formatConversionTable[int(layout.attributes[i].format)].size;
            }

            std::uint8_t *memory = _getUploadBuffer(layout.attributes.size() + stride * count);
            for (std::size_t i = 0; i < layout.attributes.size(); i++) {
                memory[i] = std::uint8_t(layout.attributes[i].format);
            }
            
            memcpy(memory + layout.attributes.size(), data, stride * count);
            WebGLId webglData = webgl_createData(memory, layout.attributes.size(), memory + layout.attributes.size(), count * stride, stride);
            result = std::make_shared<WASMData>(webglData, count, stride);
        }
        
        return result;
    }
    
    float WASMRendering::getBackBufferWidth() const {
        return _platform->getScreenWidth();
    }
    
    float WASMRendering::getBackBufferHeight() const {
        return _platform->getScreenHeight();
    }
    
    void WASMRendering::applyState(const RenderShaderPtr &shader, RenderTopology topology, const RenderPassConfig &cfg) {
        if (shader) {
            float rtWidth = _platform->getScreenWidth();
            float rtHeight = _platform->getScreenHeight();
            
            if (cfg.target) {
                rtWidth = float(cfg.target->getWidth());
                rtHeight = float(cfg.target->getHeight());
            }
            
            _frameConstants->rtBounds.x = rtWidth;
            _frameConstants->rtBounds.y = rtHeight;
            
            webgl_applyConstants(FRAME_CONST_BINDING_INDEX, _frameConstants, sizeof(FrameConstants));
            
            const GLenum cmask = (cfg.doClearColor ? GL_COLOR_BUFFER_BIT : 0) | (cfg.doClearDepth ? GL_DEPTH_BUFFER_BIT : 0);
            const WASMTarget *platformTarget = static_cast<const WASMTarget *>(cfg.target.get());
            WebGLId glTarget = 0;
            
            if (platformTarget) {
                webgl_viewPort(platformTarget->getWidth(), platformTarget->getHeight());
                glTarget = platformTarget->getWebGLTarget();
            }
            else {
                webgl_viewPort(_platform->getScreenWidth(), _platform->getScreenHeight());
            }
            
            _topology = topology;
            _currentShader = std::static_pointer_cast<WASMShader>(shader);
            
            const std::uint32_t ztype = std::uint32_t(cfg.zBehaviorType);
            const std::uint32_t btype = std::uint32_t(cfg.blendType);
            
            webgl_applyState(glTarget, _currentShader->getWebGLShader(), cmask, cfg.color[0], cfg.color[1], cfg.color[2], cfg.color[3], cfg.depth, ztype, btype);
        }
        else {
            _platform->logError("[MetalRendering::applyState] shader is null\n");
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
        for (std::size_t i = 0; i < textures.size(); i++) {
            const WASMTexture *platformTexture = static_cast<const WASMTexture *>(textures.begin()[i].first.get());
            webgl_applyTexture(i, platformTexture ? platformTexture->getWebGLTexture() : 0, int(textures.begin()[i].second));
        }
    }
    
    void WASMRendering::draw(const RenderDataPtr &inputData, std::uint32_t instanceCount) {
        const std::shared_ptr<WASMData> platformData = std::static_pointer_cast<WASMData>(inputData);
        if (_currentShader) {
            const InputLayout &layout = _currentShader->getInputLayout();
            
            WebGLId id = 0;
            std::uint32_t vcount = 1;
            
            if (platformData) {
                id = platformData->getWebGLData();
                vcount = platformData->getCount();
            }
            
            if (layout.repeat > 1) {
                webgl_drawWithRepeat(id, std::uint32_t(layout.attributes.size()), instanceCount, layout.repeat, vcount * instanceCount, g_topologies[int(_topology)]);
            }
            else {
                webgl_drawDefault(id, vcount, instanceCount, g_topologies[int(_topology)]);
            }
        }
    }
        
    void WASMRendering::presentFrame() {
        
    }
    
    std::uint8_t *WASMRendering::_getUploadBuffer(std::size_t requiredLength) {
        if (requiredLength > _uploadBufferLength) {
            delete [] _uploadBufferData;
            _uploadBufferLength = roundTo4096(requiredLength);
            _uploadBufferData = new std::uint8_t [_uploadBufferLength];
        }
        
        return _uploadBufferData;
    }
    
}

namespace foundation {
    std::shared_ptr<RenderingInterface> RenderingInterface::instance(const std::shared_ptr<PlatformInterface> &platform) {
        return std::make_shared<WASMRendering>(platform);
    }
}

#endif // PLATFORM_WASM
