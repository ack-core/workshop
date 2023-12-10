
#ifdef PLATFORM_WASM
#include "util.h"
#include "rendering_wasm.h"

#include <GLES3/gl3.h>

// From js
extern "C" {
    void *webgl_makeProgram(const std::uint16_t *vsrc, std::size_t vlen, const std::uint16_t *fsrc, std::size_t flen);
    void webgl_applyState(const void *shader);
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
    
    std::uint32_t roundTo256(std::uint32_t value) {
        std::uint32_t result = ((value - std::uint32_t(1)) & ~std::uint32_t(255)) + 256;
        return result;
    }
    
//    std::string generateRenderPipelineStateName(const foundation::WASMShader &shader, const foundation::RenderPassConfig &passConfig) {
//        std::string result = "r0_b0_" + shader.getName();
//        result[1] = passConfig.target ? '0' + int(passConfig.target->getFormat()) : 'X';
//        result[4] += int(passConfig.blendType);
//        return result;
//    }
}

namespace foundation {
    WASMShader::WASMShader(const void *webglShader, std::uint32_t constBufferLength)
        : _shader(webglShader)
        , _constBufferLength(constBufferLength)
    {}
    WASMShader::~WASMShader() {

    }
    
    std::uint32_t WASMShader::getConstBufferLength() const {
        return _constBufferLength;
    }
    const void *WASMShader::getWebGLShader() const {
        return _shader;
    }
}

namespace foundation {
    WASMRendering::WASMRendering(const std::shared_ptr<PlatformInterface> &platform) : _platform(platform) {
        _platform->logMsg("[RENDER] Initialization : WebGL");
    }
    
    WASMRendering::~WASMRendering() {}
    
    void WASMRendering::updateFrameConstants(const math::transform3f &view, const math::transform3f &proj, const math::vector3f &camPos, const math::vector3f &camDir) {
        _frameConstants.viewMatrix = view;
        _frameConstants.projMatrix = proj;
        _frameConstants.cameraPosition.xyz = camPos;
        _frameConstants.cameraDirection.xyz = camDir;
    }
    
    math::vector2f WASMRendering::getScreenCoordinates(const math::vector3f &worldPosition) {
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
    
    math::vector3f WASMRendering::getWorldDirection(const math::vector2f &screenPosition) {
        const math::transform3f inv = (_frameConstants.viewMatrix * _frameConstants.projMatrix).inverted();
        
        float relScreenPosX = 2.0f * screenPosition.x / _platform->getScreenWidth() - 1.0f;
        float relScreenPosY = 1.0f - 2.0f * screenPosition.y / _platform->getScreenHeight();
        
        const math::vector4f worldPos = math::vector4f(relScreenPosX, relScreenPosY, 0.0f, 1.0f).transformed(inv);
        return math::vector3f(worldPos.x / worldPos.w, worldPos.y / worldPos.w, worldPos.z / worldPos.w).normalized();
    }
    
    RenderShaderPtr WASMRendering::createShader(const char *name, const char *shadersrc, const RenderShaderInputDesc &vertex, const RenderShaderInputDesc &instance) {
        std::shared_ptr<RenderShader> result;
        util::strstream input(shadersrc, strlen(shadersrc));
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
            std::uint16_t *buffer = reinterpret_cast<std::uint16_t *>(malloc(sizeof(std::uint16_t) * src.length()));
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
                        
                        output += ");\n";
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
                shaderVS +=
                    "#define vertex_ID gl_VertexID\n"
                    "#define instance_ID gl_InstanceID\n"
                    "#define output_position gl_Position\n"
                    "\n";
                
                shaderVS = shaderVS + shaderFixed + shaderFunctions + shaderVSOutput;
                std::string codeBlock;
                
                if (shaderUtils::formCodeBlock(indent, input, codeBlock) == false) {
                    _platform->logError("[WASMRendering::createShader] shader '%s' has uncompleted 'vssrc' block\n", name);
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
                shaderUtils::replace(codeBlock, "vertex_", "input.", SEPARATORS, {"vertex_ID"});
                shaderUtils::replace(codeBlock, "instance_", "input.", SEPARATORS, {"instance_ID"});
                shaderUtils::replace(codeBlock, "output_", "passing.", SEPARATORS, {"output_position"});

                shaderVS += "void main() {\n";
                shaderVS += codeBlock;
                shaderVS += "}\n\n";
  
//                for (std::size_t i = 0, cnt = vertex.size(); i < cnt; i++) {
//                    const RenderShader::Input &current = *(vertex.begin() + i);
//
//                    if (current.format != ShaderInput::Format::VERTEX_ID) {
//                        nativeShaderVS += "in mediump " + shaderGetTypeName( current.format ) + " vertex_" + std::string(current.name) + ";\n";
//                    }
//                }
                
//                for (std::size_t i = 0, cnt = instance.size(); i < cnt; i++) {
//                    const ShaderInput &current = *(instance.begin() + i);
//
//                    if (current.format != ShaderInput::Format::VERTEX_ID) {
//                        vsInout += "in mediump " + shaderGetTypeName( current.format ) + " instance_" + std::string(current.name) + ";\n";
//                    }
//                }
                
                
                shaderVS = shaderUtils::makeLines(shaderVS);
                nativeShaderVS = createNativeSrc(shaderVS);
                _platform->logMsg("---------- begin ----------\n%s\n----------- end -----------\n", shaderVS.data());
                
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
                
                shaderFS += "out vec4 output_color[4];\n\n";
                shaderFS += "void main() {\n";
                shaderFS += codeBlock;
                shaderFS += "}\n\n";

                shaderFS = shaderUtils::makeLines(shaderFS);
                nativeShaderFS = createNativeSrc(shaderFS);

                _platform->logMsg("---------- begin ----------\n%s\n----------- end -----------\n", shaderFS.data());
                fssrcBlockDone = true;
                continue;
            }
            
            _platform->logError("[MetalRendering::createShader] shader '%s' has unexpected '%s' block\n", name, blockName.data());
        }
                
        if (completed && vssrcBlockDone && fssrcBlockDone) {
            void *webglShader = webgl_makeProgram(nativeShaderVS.src, nativeShaderVS.length, nativeShaderFS.src, nativeShaderFS.length);
            result = std::make_shared<WASMShader>(webglShader, constBlockLength);
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
        return nullptr;
    }
    
    RenderTargetPtr WASMRendering::createRenderTarget(RenderTextureFormat format, unsigned count, std::uint32_t w, std::uint32_t h, bool withZBuffer) {
        return nullptr;
    }
    
    RenderDataPtr WASMRendering::createData(const void *data, std::uint32_t count, std::uint32_t stride) {
        return nullptr;
    }
    
    float WASMRendering::getBackBufferWidth() const {
        return _platform->getScreenWidth();
    }
    
    float WASMRendering::getBackBufferHeight() const {
        return _platform->getScreenHeight();
    }
    
    void WASMRendering::applyState(const RenderShaderPtr &shader, const RenderPassConfig &cfg) {
        const WASMShader *platformShader = static_cast<const WASMShader *>(shader.get());
        webgl_applyState(platformShader->getWebGLShader());
    }
    
    void WASMRendering::applyShaderConstants(const void *constants) {
        
    }
    
    void WASMRendering::applyTextures(const RenderTexturePtr *textures, std::uint32_t count) {
        
    }
    
    void WASMRendering::drawGeometry(const RenderDataPtr &vertexData, std::uint32_t vcount, RenderTopology topology) {
        
    }
    
    void WASMRendering::drawGeometry(const RenderDataPtr &vertexData, const RenderDataPtr &indexData, std::uint32_t indexCount, RenderTopology topology) {
        
    }

    void WASMRendering::drawGeometry(const RenderDataPtr &vertexData, const RenderDataPtr &instanceData, std::uint32_t vcount, std::uint32_t icount, RenderTopology topology) {
        
    }
    
    void WASMRendering::presentFrame() {
        
    }
}

namespace foundation {
    std::shared_ptr<RenderingInterface> RenderingInterface::instance(const std::shared_ptr<PlatformInterface> &platform) {
        return std::make_shared<WASMRendering>(platform);
    }
}

#endif // PLATFORM_WASM
