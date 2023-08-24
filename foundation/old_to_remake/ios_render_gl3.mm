#include "interfaces.h"
#include "ios_render.h"

#include <numeric>
#include <sstream>
#include <iomanip>
#include <string>
#include <regex>

#import  <OpenGLES/ES3/gl.h>
#import  <OpenGLES/ES3/glext.h>

#define GLCHECK(...) __VA_ARGS__; if (auto error = glGetError()) { _platform->logError("[Render] GL Error : 0x%X at %s(%d)", error, __FUNCTION__, __LINE__); }

namespace {
    static constexpr std::size_t SHADER_LINES_MAX = 1024;
    static constexpr std::size_t SHADER_CONST_STREAM_BUFFER_SIZE = 64 * 1024;
    static constexpr std::size_t SHADER_BIND_FRAME_DATA = 0;
    static constexpr std::size_t SHADER_BIND_PERMANENT_CONST = 1;
    static constexpr std::size_t SHADER_BIND_CONSTANTS = 2;
    
    std::shared_ptr<platform::IOSRender> _render;
    
    GLenum _topologyMap[std::size_t(platform::Topology::_count)] = {
        GL_LINES,
        GL_LINE_STRIP,
        GL_TRIANGLES,
        GL_TRIANGLE_STRIP
    };
    
    struct NativeTexturFormat {
        GLint  internalFormat;
        GLenum format;
    }
    _nativeTextureFormatMap[unsigned(platform::Texture2D::Format::_count)] = {
        {GL_RGBA8, GL_RGBA},
        {GL_RGB8, GL_RGB},
        {GL_R8, GL_RED}
    };
    
    struct NativeVertexAttribFormat {
        GLint       componentCount;
        GLenum      componentType;
        GLboolean   normalized;
        std::size_t size;
    }
    _nativeVertexAttribFormat[unsigned(platform::ShaderInput::Format::_count)] = {
        {},
        {2, GL_HALF_FLOAT, GL_FALSE, 4},
        {4, GL_HALF_FLOAT, GL_FALSE, 8},
        {1, GL_FLOAT, GL_FALSE, 4},
        {2, GL_FLOAT, GL_FALSE, 8},
        {3, GL_FLOAT, GL_FALSE, 12},
        {4, GL_FLOAT, GL_FALSE, 16},
        {2, GL_SHORT, GL_FALSE, 4},
        {4, GL_SHORT, GL_FALSE, 8},
        {2, GL_SHORT, GL_TRUE, 4},
        {4, GL_SHORT, GL_TRUE, 8},
        {4, GL_UNSIGNED_BYTE, GL_FALSE, 4},
        {4, GL_UNSIGNED_BYTE, GL_TRUE, 4},
        {1, GL_INT, GL_FALSE, 4},
        {2, GL_INT, GL_FALSE, 8},
        {3, GL_INT, GL_FALSE, 12},
        {4, GL_INT, GL_FALSE, 16},
    };
}

namespace platform {
    class ShaderImp : public Shader {
    public:
        ShaderImp(
            const std::shared_ptr<Platform> &platform,
            const char **vsrc, GLint *vlen, std::size_t vcnt,
            const char **fsrc, GLint *flen, std::size_t fcnt,
            std::vector<ShaderInput> &&vertexLayout,
            std::vector<ShaderInput> &&instanceLayout,
            const void *permanentConstBlockData,
            std::size_t permanentConstBlockSize,
            std::size_t constantsBlockSize
        )
        : _platform(platform)
        , _vertexLayout(std::move(vertexLayout))
        , _instanceLayout(std::move(instanceLayout))
        , _permanentConstBlockSize(permanentConstBlockSize)
        , _constantsBlockSize(constantsBlockSize)
        {
            struct fn {
                static void printLinedShader(const std::shared_ptr<Platform> &platform, const char **src, GLint *len, std::size_t cnt) {
                    platform->logError("[Render] --------------------------------");
                    for (std::size_t i = 0; i < cnt; i++) {
                        char buffer[256] = {0};
                        std::memcpy(buffer, src[i], len[i] - 1);
                        platform->logError("%03zu |%s", i + 1, buffer);
                    }
                    platform->logError("-----------------------------------------");
                }
            };
            
            static constexpr unsigned BUFFER_MAX = 256;
            static char errorBuffer[BUFFER_MAX];
            
            GLuint program = GLCHECK(glCreateProgram());
            GLuint vshader = GLCHECK(glCreateShader(GL_VERTEX_SHADER));
            GLuint fshader = GLCHECK(glCreateShader(GL_FRAGMENT_SHADER));
            
            GLint length = 0;
            GLint status = 0;
            
            GLCHECK(glShaderSource(vshader, GLsizei(vcnt), vsrc, vlen));
            GLCHECK(glCompileShader(vshader));
            GLCHECK(glGetShaderiv(vshader, GL_COMPILE_STATUS, &status));
            
            if (status == GL_TRUE) {
                GLCHECK(glShaderSource(fshader, GLsizei(fcnt), fsrc, flen));
                GLCHECK(glCompileShader(fshader));
                GLCHECK(glGetShaderiv(fshader, GL_COMPILE_STATUS, &status));
                
                if (status == GL_TRUE) {
                    GLCHECK(glAttachShader(program, vshader));
                    GLCHECK(glAttachShader(program, fshader));
                    
                    for (std::size_t i = 0; i < _vertexLayout.size(); i++) {
                        GLCHECK(glBindAttribLocation(program, GLuint(i), (std::string("vertex_") + _vertexLayout[i].name).c_str()));
                    }
                    
                    GLCHECK(glLinkProgram(program));
                    GLCHECK(glGetProgramiv(program, GL_LINK_STATUS, &status));
                    
                    if (status == GL_TRUE) {
                        GLuint index;
                        
                        if ((index = glGetUniformBlockIndex(program, "_FrameData")) != GL_INVALID_INDEX) {
                            GLCHECK(glUniformBlockBinding(program, index, SHADER_BIND_FRAME_DATA));
                        }
                        if ((index = glGetUniformBlockIndex(program, "_Permanent")) != GL_INVALID_INDEX) {
                            GLCHECK(glUniformBlockBinding(program, index, SHADER_BIND_PERMANENT_CONST));
                        }
                        if ((index = glGetUniformBlockIndex(program, "_Constants")) != GL_INVALID_INDEX) {
                            GLCHECK(glUniformBlockBinding(program, index, SHADER_BIND_CONSTANTS));
                        }
                        //
                        //glcall(glUseProgram, program);
                        //
                        //for (unsigned i = 0; i < __TEXTURE_SLOTS_MAX; i++) {
                        //    int location = glcall(glGetUniformLocation, program, __textureSamplerNames[i]);
                        //
                        //    if (location != -1) {
                        //        glcall(glUniform1i, location, i);
                        //    }
                        //}
                        
                        _program = program;
                        _vshader = vshader;
                        _fshader = fshader;
                        _permanentConstBlockBuffer = 0;
                        
                        GLCHECK(glGenBuffers(1, &_permanentConstBlockBuffer));
                        GLCHECK(glBindBuffer(GL_UNIFORM_BUFFER, _permanentConstBlockBuffer));
                        GLCHECK(glBufferData(GL_UNIFORM_BUFFER, _permanentConstBlockSize, permanentConstBlockData, GL_STATIC_DRAW));
                        GLCHECK(glBindBuffer(GL_UNIFORM_BUFFER, 0));
                        
                        return;
                    }
                    else {
                        GLCHECK(glGetProgramInfoLog(program, BUFFER_MAX - 1, &length, errorBuffer));
                        fn::printLinedShader(platform, vsrc, vlen, vcnt);
                        fn::printLinedShader(platform, fsrc, flen, fcnt);
                        _platform->logError("[Render] shader linking failed: %s", errorBuffer);
                    }
                }
                else {
                    GLCHECK(glGetShaderInfoLog(fshader, BUFFER_MAX - 1, &length, errorBuffer));
                    fn::printLinedShader(platform, fsrc, flen, fcnt);
                    _platform->logError("[Render] fragment shader compilation failed: %s", errorBuffer);
                }
            }
            else {
                GLCHECK(glGetShaderInfoLog(vshader, BUFFER_MAX - 1, &length, errorBuffer));
                fn::printLinedShader(platform, vsrc, vlen, vcnt);
                _platform->logError("[Render] vertex shader compilation failed: %s", errorBuffer);
            }
            
            GLCHECK(glDeleteShader(program));
            GLCHECK(glDeleteShader(vshader));
            GLCHECK(glDeleteShader(fshader));
        }
        
        ~ShaderImp() {
            GLCHECK(glDeleteBuffers(1, &_permanentConstBlockBuffer));
            GLCHECK(glDeleteShader(_program));
            GLCHECK(glDeleteShader(_vshader));
            GLCHECK(glDeleteShader(_fshader));
        }
        
        const std::vector<ShaderInput> &getVertexLayout() const {
            return _vertexLayout;
        }
        
        const std::vector<ShaderInput> &getInstanceLayout() const {
            return _instanceLayout;
        }
        
        GLuint getProgram() const {
            return _program;
        }
        
        GLuint getPermanentConstBlockBuffer() const {
            return _permanentConstBlockBuffer;
        }
        
        std::size_t getPermanentConstBlockSize() const {
            return _permanentConstBlockSize;
        }
        
        std::size_t getConstBlockSize() const {
            return 256;//_constantsBlockSize;
        }
        
    private:
        std::shared_ptr<Platform> _platform;
        std::vector<ShaderInput> _vertexLayout;
        std::vector<ShaderInput> _instanceLayout;
        std::size_t _permanentConstBlockSize;
        std::size_t _constantsBlockSize;
        
        GLuint _vshader;
        GLuint _fshader;
        GLuint _program;
        GLuint _permanentConstBlockBuffer;
    };
}

namespace platform {
    class Texture2DImp : public Texture2D {
    public:
        Texture2DImp(
            const std::shared_ptr<Platform> &platform,
            Texture2D::Format format,
            std::uint32_t w,
            std::uint32_t h,
            const NativeTexturFormat &nativeFormat,
            const std::initializer_list<const std::uint8_t *> &imgMipsData
            
        )
        : _platform(platform)
        , _format(format)
        , _width(w)
        , _height(h)
        , _mipCount(std::uint32_t(imgMipsData.size()))
        {
            GLCHECK(glGenTextures(1, &_texture));
            GLCHECK(glBindTexture(GL_TEXTURE_2D, _texture));
            GLCHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
            GLCHECK(glTexImage2D(GL_TEXTURE_2D, 0, nativeFormat.internalFormat, w, h, 0, nativeFormat.format, GL_UNSIGNED_BYTE, nullptr));
            
            GLCHECK(glTexStorage2D(GL_TEXTURE_2D, std::uint32_t(imgMipsData.size()), nativeFormat.internalFormat, w, h));
            GLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            GLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
            
            GLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
            GLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
            
            for(std::uint32_t i = 0; i < imgMipsData.size(); i++) {
                std::uint32_t curWidth  = w >> i;
                std::uint32_t curHeight = h >> i;
                
                GLCHECK(glTexSubImage2D(GL_TEXTURE_2D, i, 0, 0, curWidth, curHeight, nativeFormat.format, GL_UNSIGNED_BYTE, imgMipsData.begin()[i] ));
            }
            
            GLCHECK(glBindTexture(GL_TEXTURE_2D, 0));
        }
        
        ~Texture2DImp() {
        
        }
        
        std::uint32_t getWidth() const {
            return _width;
        }
        
        std::uint32_t getHeight() const {
            return _height;
        }
        
        std::uint32_t getMipCount() const {
            return _mipCount;
        }
        
        Texture2D::Format getFormat() const {
            return _format;
        }
        
        GLuint getTexture() const {
            return _texture;
        }
        
        std::shared_ptr<Platform> _platform;
        std::uint32_t _mipCount;
        std::uint32_t _width;
        std::uint32_t _height;
        Texture2D::Format _format;
        GLuint _texture;
    };
    
    std::uint32_t Texture2D::getWidth() const {
        return static_cast<const Texture2DImp *>(this)->getWidth();
    }
    
    std::uint32_t Texture2D::getHeight() const {
        return static_cast<const Texture2DImp *>(this)->getHeight();
    }
    
    std::uint32_t Texture2D::getMipCount() const {
        return static_cast<const Texture2DImp *>(this)->getMipCount();
    }
    
    Texture2D::Format Texture2D::getFormat() const {
        return static_cast<const Texture2DImp *>(this)->getFormat();
    }
}

namespace platform {
    class StructuredDataImp : public StructuredData {
    public:
        StructuredDataImp(
            const std::shared_ptr<Platform> &platform,
            const void *data,
            std::uint32_t count,
            std::uint32_t stride
        )
        : _platform(platform)
        , _count(count)
        , _stride(stride)
        {
            GLCHECK(glGenBuffers(1, &_vbo));
            GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, _vbo));
            GLCHECK(glBufferData(GL_ARRAY_BUFFER, count * stride, data, GL_STATIC_DRAW));
            GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
        }
        
        ~StructuredDataImp() {
            GLCHECK(glDeleteBuffers(1, &_vbo));
        }
        
        std::uint32_t getCount() const {
            return _count;
        }
        
        std::uint32_t getStride() const {
            return _stride;
        }
        
        GLuint getBuffer() const {
            return _vbo;
        }
        
        std::shared_ptr<Platform> _platform;
        std::uint32_t _count;
        std::uint32_t _stride;
        GLuint _vbo;
    };
    
    std::uint32_t StructuredData::getCount() const {
        return static_cast<const StructuredDataImp *>(this)->getCount();
    }

    std::uint32_t StructuredData::getStride() const {
        return static_cast<const StructuredDataImp *>(this)->getStride();
    }
}

namespace platform {
    IOSRender::IOSRender(const std::shared_ptr<Platform> &platform) : _platform(platform), _frameData(), _shaderConstStreamOffset(0) {
        GLCHECK(glEnable(GL_DEPTH_TEST));
        GLCHECK(glDepthFunc(GL_GREATER));
        GLCHECK(glClearDepthf(0.0f));
        
        GLCHECK(glGenBuffers(1, &_shaderFrameDataBuffer));
        GLCHECK(glBindBuffer(GL_UNIFORM_BUFFER, _shaderFrameDataBuffer));
        GLCHECK(glBufferData(GL_UNIFORM_BUFFER, sizeof(FrameData), nullptr, GL_DYNAMIC_DRAW));
        GLCHECK(glBindBuffer(GL_UNIFORM_BUFFER, 0));

        GLCHECK(glGenBuffers(1, &_shaderConstStreamBuffer));
        GLCHECK(glBindBuffer(GL_UNIFORM_BUFFER, _shaderConstStreamBuffer));
        GLCHECK(glBufferData(GL_UNIFORM_BUFFER, SHADER_CONST_STREAM_BUFFER_SIZE, nullptr, GL_DYNAMIC_DRAW));
        GLCHECK(glBindBuffer(GL_UNIFORM_BUFFER, 0));
    }
    
    IOSRender::~IOSRender() {
        GLCHECK(glDeleteBuffers(1, &_shaderFrameDataBuffer));
        GLCHECK(glDeleteBuffers(1, &_shaderConstStreamBuffer));
    }
    
    void IOSRender::updateCameraTransform(const float (&camPos)[3], const float(&camDir)[3], const float(&camVP)[16]) {
        ::memcpy(_frameData.cameraPosition, camPos, 3 * sizeof(float));
        ::memcpy(_frameData.cameraDirection, camDir, 3 * sizeof(float));
        ::memcpy(_frameData.viewProjMatrix, camVP, 16 * sizeof(float));
    }

    template <typename = void> std::istream &expect(std::istream &stream) {
        return stream;
    }
    template <char Ch, char... Chs> std::istream &expect(std::istream &stream) {
        (stream >> std::ws).peek() == Ch ? (void)stream.ignore() : stream.setstate(std::ios_base::failbit);
        return expect<Chs...>(stream);
    }
    
    std::size_t shaderGetTypeSize(const std::string &varname, std::string &format, bool extended) {
        struct {
            const char *inputFormat;
            const char *outputFormat;
            std::size_t size;
        }
        typeSizeTable[] = {
            {"float4", "vec4",  16},
            {"int4",   "ivec4", 16},
            {"uint4",  "uvec4", 16},
            {"matrix4", "mat4", 64},
        },
        typeSizeTableEx[] = {
            {"float", "float",  4},
            {"float2", "vec2",  8},
            {"float3", "vec3",  12},
            {"float4", "vec4",  16},
            {"int",   "int", 4},
            {"int2",   "ivec2", 8},
            {"int3",   "ivec3", 12},
            {"int4",   "ivec4", 16},
            {"uint",  "uint", 4},
            {"uint2",  "uvec2", 8},
            {"uint3",  "uvec3", 12},
            {"uint4",  "uvec4", 16},
            {"matrix3", "mat3", 36},
            {"matrix4", "mat4", 64},
        };
        
        int  multiply = 1;
        auto braceStart = varname.find('[');
        auto braceEnd = varname.rfind(']');
        
        if (braceStart != std::string::npos && braceEnd != std::string::npos) {
            multiply = std::max(std::stoi(varname.substr(braceStart + 1, braceEnd - braceStart - 1)), multiply);
        }

        auto begin = extended ? std::begin(typeSizeTableEx) : std::begin(typeSizeTable);
        auto end = extended ? std::end(typeSizeTableEx) : std::end(typeSizeTable);

        for (auto index = begin; index != end; ++index) {
            if (index->inputFormat == format) {
                format = index->outputFormat;
                return index->size * multiply;
            }
        }
        
        return 0;
    }
    
    std::string shaderGetTypeName(ShaderInput::Format format) {
        struct {
            ShaderInput::Format format;
            const char *name;
        }
        formatTable[] = {
            {ShaderInput::Format::VERTEX_ID, "uint"},
            {ShaderInput::Format::HALF2, "vec2"},
            {ShaderInput::Format::HALF4, "vec4"},
            {ShaderInput::Format::FLOAT1, "float"},
            {ShaderInput::Format::FLOAT2, "vec2"},
            {ShaderInput::Format::FLOAT3, "vec3"},
            {ShaderInput::Format::FLOAT4, "vec4"},
            {ShaderInput::Format::SHORT2, "ivec2"},
            {ShaderInput::Format::SHORT4, "ivec4"},
            {ShaderInput::Format::SHORT2_NRM, "vec2"},
            {ShaderInput::Format::SHORT4_NRM, "vec4"},
            {ShaderInput::Format::BYTE4, "uvec4"},
            {ShaderInput::Format::BYTE4_NRM, "vec4"},
        };
        
        for (const auto &fmt : formatTable) {
            if (fmt.format == format) {
                return fmt.name;
            }
        }
        
        return {};
    }
    
    std::shared_ptr<Shader> IOSRender::createShader(
        const char *shadersrc,
        const std::initializer_list<ShaderInput> &vertex,
        const std::initializer_list<ShaderInput> &instance,
        const void *prmnt
    ) {
        std::shared_ptr<Shader> result;
        
        std::string varname, arg;
        std::string shaderConsts;
        std::string vsInout;
        std::string fsInout;
        std::string vsBlock;
        std::string fsBlock;
        
        std::size_t shaderPrmntCount = 0;
        std::size_t shaderConstCount = 0;
        std::size_t shaderInterCount = 0;
        std::size_t shaderVssrcCount = 0;
        std::size_t shaderFssrcCount = 0;
        
        std::size_t shaderPrmntSize = 0;
        std::size_t shaderConstSize = 0;
        
        std::istringstream stream (shadersrc);
        
        std::string vsDefines = "#define out_position gl_Position\n";
        std::string vsShader =
            "#version 300 es\n\n"
            "#define _sign(a) (2.0 * step(0.0, a) - 1.0)\n"
            "#define _transform(a, b) (b * a)\n"
            "#define _dot(a, b) dot(a, b)\n"
            "#define _cos(a) cos(a)\n"
            "#define _sin(a) sin(a)\n"
            "#define _norm(a) normalize(a)\n"
            "#define _tex2d(a, b) texture(_textures[a], b)\n"
            "\n"
            "layout(std140) uniform _FrameData\n{\n"
            "mediump mat4 _viewProjMatrix;\n"
            "mediump vec4 _cameraPosition;\n"
            "mediump vec4 _cameraDirection;\n"
            "mediump vec4 _renderTargetBounds;\n"
            "};\n"
            "\n";
        
        std::string fsShader = vsShader;
        bool error = false;
        
        for (std::size_t i = 0, cnt = vertex.size(); i < cnt; i++) {
            const ShaderInput &current = *(vertex.begin() + i);
            
            if (current.format == ShaderInput::Format::VERTEX_ID) {
                vsDefines += std::string("#define vertex_") + current.name + " gl_VertexID\n\n";
            }
            else {
                vsInout += "in mediump " + shaderGetTypeName( current.format ) + " vertex_" + std::string(current.name) + ";\n";
            }
        }
        
        for (std::size_t i = 0, cnt = instance.size(); i < cnt; i++) {
            const ShaderInput &current = *(instance.begin() + i);
            
            if (current.format != ShaderInput::Format::VERTEX_ID) {
                vsInout += "in mediump " + shaderGetTypeName( current.format ) + " instance_" + std::string(current.name) + ";\n";
            }
        }
        
        vsInout += "\n";
        
        auto readVarsBlock = [this, &stream, &varname, &arg, &error](const char *blockName, std::size_t &counter, std::string &out1, std::string *out2) {
            std::size_t bufferSize = 0;
            bool extendedRangeOfTypes = strcmp(blockName, "inter") == 0;
            
            if (counter++ == 0) {
                while (stream >> varname && varname[0] != '}') {
                    if (stream >> expect<':'> >> arg) {
                        if (std::size_t typeSize = shaderGetTypeSize(varname, arg, extendedRangeOfTypes)) {
                            bufferSize += typeSize;
                            out1 += "mediump " + arg + " " + varname + ";\n";
                            if (out2) out2->append("mediump " + arg + " " + varname + ";\n");
                        }
                        else {
                            _platform->logError("[Render] shader : unknown type of constant '%s'", varname.c_str());
                            error = true;
                            break;
                        }
                    }
                    else {
                        _platform->logError("[Render] shader : constant block syntax error");
                        error = true;
                        break;
                    }
                }
            }
            else {
                _platform->logError("[Render] shader : only one '%s' block is allowed", blockName);
                error = true;
            }
            
            return bufferSize;
        };
        
        auto readCodeBlock = [this, &stream, &error](const char *blockName, std::size_t &counter, std::string &dest) {
            if (counter++ == 0) {
                char ch[2] = {};
                int braceCounter = 0; // '{' already skipped
                
                while (true) {
                    ch[0] = stream.get();
                    
                    if (ch[0] == '{') braceCounter++;
                    if (ch[0] == '}') {
                        if (braceCounter == 0) break;
                        else if (braceCounter > 0) {
                            braceCounter--;
                        }
                        else {
                            _platform->logError("[Render] shader '%s' : unexpected '}'", blockName);
                            error = true;
                            return false;
                        }
                    }
                    
                    if (stream.good()) {
                        if (ch[0] == '\n') {
                            stream >> std::ws;
                        }
                        
                        dest += ch;
                    }
                    else {
                        _platform->logError("[Render] shader '%s' : stream error", blockName);
                        error = true;
                        return false;
                    }
                }
                
                std::regex ctors1(R"(float([234]{1})[\s]*\()");
                dest = std::regex_replace(dest, ctors1, "vec$1(");
                
                std::regex ctors2(R"(matrix([34]{1})[\s]*\()");
                dest = std::regex_replace(dest, ctors2, "mat$1(");
                
                std::regex vars1(R"(float([234]{1})[\s]+([\w]+))");
                dest = std::regex_replace(dest, vars1, "mediump vec$1 $2");
                
                std::regex vars2(R"(matrix([34]{1})[\s]+([\w]+))");
                dest = std::regex_replace(dest, vars2, "mediump mat$1 $2");
                
                return true;
            }
            else {
                _platform->logError("[Render] shader : only one '%s' block is allowed", blockName);
                error = true;
                return false;
            }
        };
        
        while (error == false && bool(stream >> arg >> expect<'{'>)) {
            if (arg == "prmnt") {
                shaderConsts += "layout(std140) uniform _Permanent\n{\n";
                shaderPrmntSize = readVarsBlock("prmnt", shaderPrmntCount, shaderConsts, nullptr);
                shaderConsts += "};\n\n";
            }
            else if (arg == "const") {
                shaderConsts += "layout(std140) uniform _Constants\n{\n";
                shaderConstSize = readVarsBlock("const", shaderConstCount, shaderConsts, nullptr);
                shaderConsts += "};\n\n";
            }
            else if (arg == "inter") {
                vsInout += "out struct _Inter\n{\n";
                fsInout += "in struct _Inter\n{\n";
                readVarsBlock("inter", shaderInterCount, vsInout, &fsInout);
                vsInout += "}\ninter;\n\n";
                fsInout += "}\ninter;\n\n";
            }
            else if (arg == "vssrc") {
                if (readCodeBlock("vssrc", shaderVssrcCount, vsBlock)) {
                    vsBlock = "void main()\n{" + vsBlock + "}\n";
                    vsShader += shaderConsts + vsDefines + vsInout + vsBlock;
                }
            }
            else if (arg == "fssrc") {
                if (readCodeBlock("fssrc", shaderFssrcCount, fsBlock)) {
                    fsInout += "out mediump vec4 out_color;\n\n";
                    fsBlock = "uniform sampler2D _textures[8];\nvoid main()\n{" + fsBlock + "}\n";
                    fsShader += shaderConsts + fsInout + fsBlock;
                }
            }
            else {
                _platform->logError("[Render] shader : undefined block");
                break;
            }
        }
        
        if (error == false) {
            std::size_t vsLineCounter = 0;
            std::size_t fsLineCounter = 0;
            const char *vsrc[SHADER_LINES_MAX] = {vsShader.data()};
            const char *fsrc[SHADER_LINES_MAX] = {fsShader.data()};
            GLint vslen[SHADER_LINES_MAX] = {0};
            GLint fslen[SHADER_LINES_MAX] = {0};
            
            {
                const char *current = vsShader.data();
                while(const char *chpos = (std::strchr(current, '\n'))) {
                    vsrc[vsLineCounter] = current;
                    vslen[vsLineCounter++] = GLint(chpos - current + 1);
                    current = chpos + 1;
                }
            }
            {
                const char *current = fsShader.data();
                while(const char *chpos = (std::strchr(current, '\n'))) {
                    fsrc[fsLineCounter] = current;
                    fslen[fsLineCounter++] = GLint(chpos - current + 1);
                    current = chpos + 1;
                }
            }
            
            result = std::make_shared<ShaderImp>(
                _platform,
                vsrc, vslen, vsLineCounter,
                fsrc, fslen, fsLineCounter,
                std::vector<ShaderInput>(vertex.begin(), vertex.end()),
                std::vector<ShaderInput>(instance.begin(), instance.end()),
                prmnt,
                shaderPrmntSize,
                shaderConstSize
            );
        }
        
        return result;
    }
    
    std::shared_ptr<Texture2D> IOSRender::createTexture(Texture2D::Format format, std::uint32_t w, std::uint32_t h, const std::initializer_list<const std::uint8_t *> &mipsData) {
        return std::make_unique<Texture2DImp>(_platform, format, w, h, _nativeTextureFormatMap[std::size_t(format)], mipsData);
    }
    
    std::shared_ptr<StructuredData> IOSRender::createData(const void *data, std::uint32_t count, std::uint32_t stride) {
        return std::make_unique<StructuredDataImp>(_platform, data, count, stride);
    }
    
    void IOSRender::applyShader(const std::shared_ptr<Shader> &shader, const void *constants) {
        const ShaderImp *platformShader = static_cast<const ShaderImp *>(shader.get());
        
        if (platformShader) {
            GLCHECK(glUseProgram(platformShader->getProgram()));
            
            if (constants) {
                if (_shaderConstStreamOffset + platformShader->getConstBlockSize() > SHADER_CONST_STREAM_BUFFER_SIZE) {
                    _shaderConstStreamOffset = 0;
                }
                
                GLCHECK(glBindBuffer(GL_UNIFORM_BUFFER, _shaderConstStreamBuffer));
                if (char *mapPtr = (char *)glMapBufferRange(GL_UNIFORM_BUFFER, _shaderConstStreamOffset, platformShader->getConstBlockSize(), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)) {
                    memcpy(mapPtr, constants, platformShader->getConstBlockSize()); //
                    GLCHECK(glUnmapBuffer(GL_UNIFORM_BUFFER));
                }
                else {
                    _platform->logError("[Render] Unable to map uniform buffer");
                }
                GLCHECK(glBindBuffer(GL_UNIFORM_BUFFER, 0));
                //GLCHECK(glBindBufferRange(GL_UNIFORM_BUFFER, SHADER_BIND_CONSTANTS, _shaderConstStreamBuffer, 0, platformShader->getConstBlockSize()));
                //_shaderConstStreamOffset += platformShader->getConstBlockSize();
            }
            
            GLCHECK(glBindBufferBase(GL_UNIFORM_BUFFER, SHADER_BIND_CONSTANTS, _shaderConstStreamBuffer));
            GLCHECK(glBindBufferBase(GL_UNIFORM_BUFFER, SHADER_BIND_FRAME_DATA, _shaderFrameDataBuffer));
            GLCHECK(glBindBufferBase(GL_UNIFORM_BUFFER, SHADER_BIND_PERMANENT_CONST, platformShader->getPermanentConstBlockBuffer()));
            
            _currentShader = shader;
        }
    }
    
    void IOSRender::applyTextures(const std::initializer_list<const Texture2D *> &textures) {
        for (std::size_t i = 0; i < textures.size(); i++) {
            const Texture2DImp *currentTexture = static_cast<const Texture2DImp *>(textures.begin()[i]);
            
            if (currentTexture) {
                GLCHECK(glActiveTexture(GL_TEXTURE0 + GLenum(i)));
                GLCHECK(glBindTexture(GL_TEXTURE_2D, currentTexture->getTexture()));
            }
        }
    }
    
    void IOSRender::drawGeometry(std::uint32_t vertexCount, Topology topology) {
        GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
        GLCHECK(glDrawArrays(_topologyMap[unsigned(topology)], 0, vertexCount));
    }
    
    void IOSRender::drawGeometry(
        const std::shared_ptr<StructuredData> &vertexData,
        const std::shared_ptr<StructuredData> &instanceData,
        std::uint32_t vertexCount,
        std::uint32_t instanceCount,
        Topology topology
    ) {
        if (_currentShader) {
            const ShaderImp *shaderImp = static_cast<const ShaderImp *>(_currentShader.get());
            const std::vector<ShaderInput> &vertexDesc = shaderImp->getVertexLayout();
            const std::vector<ShaderInput> &instanceDesc = shaderImp->getInstanceLayout();
            
            GLuint index = 0;
            
            if (const StructuredDataImp *vertexDataImp = static_cast<const StructuredDataImp *>(vertexData.get())) {
                GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, vertexDataImp->getBuffer()));
                
                const char *offset = 0;
                for (GLuint i = 0; i < vertexDesc.size(); i++) {
                    if (vertexDesc[i].format != ShaderInput::Format::VERTEX_ID) {
                        auto &format = _nativeVertexAttribFormat[unsigned(vertexDesc[i].format)];
                        GLCHECK(glVertexAttribPointer(index, format.componentCount, format.componentType, format.normalized, vertexDataImp->getStride(), offset));
                        GLCHECK(glVertexAttribDivisor(index, 0));
                        GLCHECK(glEnableVertexAttribArray(index));
                        offset += format.size;
                        index++;
                    }
                }
            }
            
            if (const StructuredDataImp *instanceDataImp = static_cast<const StructuredDataImp *>(instanceData.get())) {
                GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, instanceDataImp->getBuffer()));
                
                const char *offset = 0;
                for (GLuint i = 0; i < instanceDesc.size(); i++) {
                    if (instanceDesc[i].format != ShaderInput::Format::VERTEX_ID) {
                        auto &format = _nativeVertexAttribFormat[unsigned(instanceDesc[i].format)];
                        GLCHECK(glVertexAttribPointer(index, format.componentCount, format.componentType, format.normalized, instanceDataImp->getStride(), offset));
                        GLCHECK(glVertexAttribDivisor(index, 1));
                        GLCHECK(glEnableVertexAttribArray(index));
                        offset += format.size;
                        index++;
                    }
                }
            }
            
            GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
            GLCHECK(glDrawArraysInstanced(_topologyMap[unsigned(topology)], 0, vertexCount, instanceCount));
        }
        else {
            _platform->logWarning("[Render] drawGeometry requires shader set");
        }
    }
    
    void IOSRender::prepareFrame() {
        GLCHECK(glClearColor(0.7f, 0.7f, 0.7f, 1.0f));
        GLCHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        
        _frameData.renderTargetBounds[0] = _platform->getNativeScreenWidth();
        _frameData.renderTargetBounds[1] = _platform->getNativeScreenHeight();
        
        GLCHECK(glBindBuffer(GL_UNIFORM_BUFFER, _shaderFrameDataBuffer));
        GLCHECK(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(FrameData), &_frameData));
        GLCHECK(glBindBuffer(GL_UNIFORM_BUFFER, 0));
    }
    
    void IOSRender::presentFrame(float dtSec) {
        
    }
    
    void IOSRender::getFrameBufferData(std::uint8_t *imgFrame) {
        GLCHECK(glFinish());
        GLCHECK(glReadPixels(0, 0, _platform->getNativeScreenWidth(), _platform->getNativeScreenHeight(), GL_RGBA, GL_UNSIGNED_BYTE, imgFrame));
    }
    
    std::shared_ptr<RenderingDevice> getRenderingDeviceInstance(const std::shared_ptr<Platform> &platform) {
        if (_render == nullptr) {
            EAGLContext *glContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
        
            if (glContext) {
                platform->setNativeRenderingContext((__bridge_retained void *)(glContext));
                platform->logInfo("[Render] ES context: OK");
                
                [EAGLContext setCurrentContext:glContext];
            }
            else {
                platform->logError("[Render] Failed to create ES context");
            }
            
            _render = std::make_shared<platform::IOSRender>(platform);
        }
        
        return _render;
    }
}

