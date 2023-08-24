
namespace fg {
    namespace opengl {
        const unsigned __BUFFER_MAX = 4096;
        const unsigned __INSTANCE_ATTRIBUTE_LOCATION_START = 8;

        const char *__uboNames[unsigned(platform::ShaderConstBufferUsing::_ShaderConstBufferUsingCount)] = {
            "FrameData",
            "CameraData",
            "MaterialData",
            "LightingData",
            "SkinData",
            "AdditionalData",
        };

        const char *__textureSamplerNames[unsigned(platform::TextureSlot::_TextureSlotCount)] = {
            "textures[0]",
            "textures[1]",
            "texEnv[0]",
            "texEnv[1]",
            "texShadow[0]",
            "texShadow[1]",
            "texShadow[2]",
            "texShadow[3]",
        };

        struct __LayoutDesc {
            const char *name;
            unsigned   stride;
            unsigned   offset;
            unsigned   componentCount;
            GLenum     componentType;
            bool       normalized;
        };
        
        struct __AttributeBinding {
            const char *name;
            unsigned   index;
        };

        __AttributeBinding __vertexAttributes[] = {
            {"position", 0},
            {"texcoord", 1},
            {"nrmXYZhand", 2},
            {"tanXYZnone", 3},
            {"bIndexes", 4},
            {"bWeights", 5},
        };

        __AttributeBinding __instanceAttributes[] = {
            {"instance_color", 0},
            {"instance_mdlTransform_0", 1},
            {"instance_mdlTransform_1", 2},
            {"instance_mdlTransform_2", 3},
            {"instance_mdlTransform_3", 4},
            {"instance_position_grey", 1},
        };

        __LayoutDesc __inputVertexLayoutSimple[] =
        {
            {"position", sizeof(platform::VertexSimple), 0, 3, GL_FLOAT, false},
            {"texcoord", sizeof(platform::VertexSimple), 12, 2, GL_HALF_FLOAT, false},
        };

        __LayoutDesc __inputVertexLayoutFull[] =
        {
            {"position", sizeof(platform::VertexSkinnedNormal), 0, 3, GL_FLOAT, false},
            {"texcoord", sizeof(platform::VertexSkinnedNormal), 12, 2, GL_HALF_FLOAT, false},
            {"nrmXYZhand", sizeof(platform::VertexSkinnedNormal), 16, 4, GL_UNSIGNED_BYTE, true},
            {"tanXYZnone", sizeof(platform::VertexSkinnedNormal), 20, 4, GL_UNSIGNED_BYTE, true},
            {"bIndexes", sizeof(platform::VertexSkinnedNormal), 24, 4, GL_UNSIGNED_BYTE, false},
            {"bWeights", sizeof(platform::VertexSkinnedNormal), 28, 4, GL_UNSIGNED_BYTE, true},
        };

        __LayoutDesc __inputInstanceLayoutRenderObject[] =
        {
            {"instance_color", sizeof(platform::InstanceDataRenderObjectDefault), 0, 4, GL_FLOAT, false},
            {"instance_mdlTransform_0", sizeof(platform::InstanceDataRenderObjectDefault), 16, 4, GL_FLOAT, false},
            {"instance_mdlTransform_1", sizeof(platform::InstanceDataRenderObjectDefault), 32, 4, GL_FLOAT, false},
            {"instance_mdlTransform_2", sizeof(platform::InstanceDataRenderObjectDefault), 48, 4, GL_FLOAT, false},
            {"instance_mdlTransform_3", sizeof(platform::InstanceDataRenderObjectDefault), 64, 4, GL_FLOAT, false},
        };

        __LayoutDesc __inputInstanceLayoutDisplayObject[] =
        {
            {"instance_color", sizeof(platform::InstanceDataDisplayObjectDefault), 0, 4, GL_FLOAT, false},
            {"instance_position_grey", sizeof(platform::InstanceDataDisplayObjectDefault), 16, 4, GL_FLOAT, false},
        };


        __LayoutDesc *__inputVertexLayouts[unsigned(platform::VertexType::_VertexTypeCount)] = { __inputVertexLayoutSimple, __inputVertexLayoutFull };
        __LayoutDesc *__inputInstanceLayouts[unsigned(platform::InstanceDataType::_InstanceDataTypeCount)] = {__inputInstanceLayoutRenderObject, __inputInstanceLayoutDisplayObject};

        unsigned __vertexTypeSizes[unsigned(platform::VertexType::_VertexTypeCount)] = {sizeof(platform::VertexSimple), sizeof(platform::VertexSkinnedNormal)};
        unsigned __instanceTypeSizes[unsigned(platform::InstanceDataType::_InstanceDataTypeCount)] = {sizeof(platform::InstanceDataRenderObjectDefault), sizeof(platform::InstanceDataDisplayObjectDefault)};

        unsigned __inputVertexLayoutSizes[unsigned(platform::VertexType::_VertexTypeCount)] = {_countof(__inputVertexLayoutSimple), _countof(__inputVertexLayoutFull)};
        unsigned __inputInstanceLayoutSizes[unsigned(platform::InstanceDataType::_InstanceDataTypeCount)] = {_countof(__inputInstanceLayoutRenderObject), _countof(__inputInstanceLayoutDisplayObject)};

        char   __buffer[__BUFFER_MAX];
        GLenum __nativeTextureFormat[unsigned(platform::TextureFormat::_TextureFormatCount)] = {GL_RGBA16F, GL_RGBA, GL_RGBA, GL_RED, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE};
        GLenum __nativeTextureInternalFormat[unsigned(platform::TextureFormat::_TextureFormatCount)] = {GL_RGBA16F, GL_RGBA8, GL_RGBA8, GL_R8, GL_NONE, GL_NONE, GL_NONE};
        GLenum __nativeTopology[unsigned(platform::PrimitiveTopology::_PrimitiveTopologyCount)] = {GL_LINES, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP};
        GLenum __nativeCmpFunc[unsigned(platform::DepthFunc::_DepthFuncCount)] = {GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS};

        //---

        PlatformVertexBuffer::PlatformVertexBuffer(OpenGLES3Win32Platform *owner,  platform::VertexType type, unsigned vcount, GLenum usage) : PlatformObject(owner) {
            _type = type;
            _usage = usage;
            _length = __vertexTypeSizes[unsigned(type)] * vcount;
            
            glGenBuffers(1, &_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, _vbo);
            glBufferData(GL_ARRAY_BUFFER, _length, nullptr, _usage);
        
            _owner->resetLastVBO();
        }

        PlatformVertexBuffer::~PlatformVertexBuffer() {
            glDeleteBuffers(1, &_vbo);
        }

        GLuint PlatformVertexBuffer::getVBO() const {
            return _vbo;
        }

        platform::VertexType PlatformVertexBuffer::getType() const {
            return _type;
        }

        void *PlatformVertexBuffer::lock(bool doNotWait) {
            _owner->resetLastVBO();
            glBindBuffer(GL_ARRAY_BUFFER, _vbo);
            return glMapBufferRange(GL_ARRAY_BUFFER, 0, _length, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | (doNotWait ? GL_MAP_UNSYNCHRONIZED_BIT : 0));
        }

        void PlatformVertexBuffer::unlock() {
            _owner->resetLastVBO();
            glBindBuffer(GL_ARRAY_BUFFER, _vbo);
            glUnmapBuffer(GL_ARRAY_BUFFER);
        }
       
        bool PlatformVertexBuffer::valid() const {
            return _vbo != 0;
        }

        unsigned PlatformVertexBuffer::getLength() const {
            return _length;
        }

        //---

        PlatformIndexedVertexBuffer::PlatformIndexedVertexBuffer(OpenGLES3Win32Platform *owner, platform::VertexType type, unsigned vcount, unsigned icount, GLenum usage) : PlatformObject(owner) {
            _type = type;
            _usage = usage;
            _vlength = __vertexTypeSizes[unsigned(type)] * vcount;
            _ilength = icount * sizeof(unsigned short);

            glGenBuffers(1, &_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, _vbo);
            glGenBuffers(1, &_ibo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);

            glBufferData(GL_ARRAY_BUFFER, _vlength, nullptr, _usage);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, _ilength, nullptr, _usage);
        
            _owner->resetLastVBO();
        }

        PlatformIndexedVertexBuffer::~PlatformIndexedVertexBuffer() {
            glDeleteBuffers(1, &_vbo);
            glDeleteBuffers(1, &_ibo);
        }

        GLuint PlatformIndexedVertexBuffer::getVBO() const {
            return _vbo;
        }

        GLuint PlatformIndexedVertexBuffer::getIBO() const {
            return _ibo;
        }
        
        platform::VertexType PlatformIndexedVertexBuffer::getType() const {
            return _type;
        }

        void *PlatformIndexedVertexBuffer::lockVertices(bool doNotWait) {
            _owner->resetLastVBO();            
            glBindBuffer(GL_ARRAY_BUFFER, _vbo);
            return glMapBufferRange(GL_ARRAY_BUFFER, 0, _vlength, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | (doNotWait ? GL_MAP_UNSYNCHRONIZED_BIT : 0));
        }

        void *PlatformIndexedVertexBuffer::lockIndices(bool doNotWait) {
            _owner->resetLastIBO();
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
            return glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, _ilength, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | (doNotWait ? GL_MAP_UNSYNCHRONIZED_BIT : 0));
        }

        void PlatformIndexedVertexBuffer::unlockVertices() {
            _owner->resetLastVBO();
            glBindBuffer(GL_ARRAY_BUFFER, _vbo);
            glUnmapBuffer(GL_ARRAY_BUFFER);
        }

        void PlatformIndexedVertexBuffer::unlockIndices() {
            _owner->resetLastIBO();
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
            glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        }

        bool PlatformIndexedVertexBuffer::valid() const {
            return _vbo != 0 && _ibo != 0;
        }

        unsigned PlatformIndexedVertexBuffer::getVertexDataLength() const {
            return _vlength;
        }

        unsigned PlatformIndexedVertexBuffer::getIndexDataLength() const {
            return _ilength;
        }

        //---

        PlatformInstanceData::PlatformInstanceData(OpenGLES3Win32Platform *owner, platform::InstanceDataType type, unsigned instanceCount) : PlatformObject(owner) {
            _length = instanceCount * __instanceTypeSizes[unsigned(type)];
            _type = type;
            
            glGenBuffers(1, &_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, _vbo);
            glBufferData(GL_ARRAY_BUFFER, _length, nullptr, GL_DYNAMIC_DRAW);
        
            _owner->resetLastVBO();
        }

        PlatformInstanceData::~PlatformInstanceData() {
            glDeleteBuffers(1, &_vbo);
        }

        GLuint PlatformInstanceData::getVBO() const {
            return _vbo;
        }

        void PlatformInstanceData::update(const void *data, unsigned instanceCount) {
            _owner->resetLastVBO();
            glBindBuffer(GL_ARRAY_BUFFER, _vbo);
            glBufferData(GL_ARRAY_BUFFER, instanceCount * __instanceTypeSizes[unsigned(_type)], data, GL_DYNAMIC_DRAW);
        }

        bool PlatformInstanceData::valid() const {
            return _vbo != 0;
        }

        platform::InstanceDataType PlatformInstanceData::getType() const {
            return _type;
        }

        //---

        PlatformRenderState::~PlatformRenderState() {

        }

        void PlatformRenderState::set() {
            if (_owner->cmpxLastCullMode(_cullMode)) {
                if (_cullMode == platform::CullMode::NONE) {
                    glDisable(GL_CULL_FACE);
                }
                else {
                    glEnable(GL_CULL_FACE);
                    glFrontFace(_cullMode == platform::CullMode::BACK ? GL_CCW : GL_CW);
                }
            }
            
            if (_owner->cmpxLastBlendMode(_blendMode)) {
                if (_blendMode == platform::BlendMode::NOBLEND) {
                    glDisable(GL_BLEND);
                }
                else if (_blendMode == platform::BlendMode::ALPHA_LERP) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glBlendEquation(GL_FUNC_ADD);
                }
                else if (_blendMode == platform::BlendMode::ALPHA_ADD) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    glBlendEquation(GL_FUNC_ADD);
                }
                else if (_blendMode == platform::BlendMode::MIN_VALUE) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE, GL_ONE);
                    glBlendEquation(GL_MIN);
                }
                else if (_blendMode == platform::BlendMode::MAX_VALUE) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE, GL_ONE);
                    glBlendEquation(GL_MAX);
                }
            }

            if (_owner->cmpxLastDepthEnabled(_depthEnabled)) {
                if (_depthEnabled) {
                    glEnable(GL_DEPTH_TEST);
                }
                else {
                    glDisable(GL_DEPTH_TEST);
                }
            }

            if (_owner->cmpxLastDepthCmpFunc(_cmpFunc)) {
                glDepthFunc(_cmpFunc);
            }
        }

        bool PlatformRenderState::valid() const {
            return true;
        }

        PlatformRenderState::PlatformRenderState(OpenGLES3Win32Platform *owner, platform::CullMode cullMode, platform::BlendMode blendMode, bool depthEnabled, platform::DepthFunc depthCmpFunc, bool depthWriteEnabled) : PlatformObject(owner) {
            _cullMode = cullMode;
            _blendMode = blendMode;
            _depthEnabled = depthEnabled;
            _depthWriteEnabled = depthWriteEnabled;
            _cmpFunc = __nativeCmpFunc[unsigned(depthCmpFunc)];
        }    

        //---

        PlatformSampler::PlatformSampler(OpenGLES3Win32Platform *owner, platform::TextureFilter filter, platform::TextureAddressMode addrMode, float minLod, float bias) : PlatformObject(owner) {
            glGenSamplers(1, &_self);

            glSamplerParameteri(_self, GL_TEXTURE_MIN_LOD, (GLint)(minLod));
            glSamplerParameteri(_self, GL_TEXTURE_MIN_FILTER, filter == platform::TextureFilter::POINT ? GL_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
            glSamplerParameteri(_self, GL_TEXTURE_MAG_FILTER, filter == platform::TextureFilter::POINT ? GL_NEAREST : GL_LINEAR);
            glSamplerParameteri(_self, GL_TEXTURE_WRAP_S, addrMode == platform::TextureAddressMode::CLAMP ? GL_CLAMP_TO_EDGE : GL_REPEAT);
            glSamplerParameteri(_self, GL_TEXTURE_WRAP_T, addrMode == platform::TextureAddressMode::CLAMP ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        }

        PlatformSampler::~PlatformSampler() {
            glDeleteSamplers(1, &_self);
        }

        void PlatformSampler::set(platform::TextureSlot slot) {
            glBindSampler(unsigned(slot), _self);
        }

        bool PlatformSampler::valid() const {
            return _self != 0;
        }

        GLuint PlatformSampler::getSampler() const {
            return _self;
        }

        //---

        PlatformShader::PlatformShader(OpenGLES3Win32Platform *owner, const byteinput &binary, const diag::LogInterface &log) : PlatformObject(owner) {
            _owner->resetLastProgram();
            _program = glCreateProgram();
            _vShader = glCreateShader(GL_VERTEX_SHADER);
            _fShader = glCreateShader(GL_FRAGMENT_SHADER);

            GLint status = 0;
            const char *vshader = binary.getPtr();
            const char *fshader = vshader;

            while(*(unsigned *)(++fshader) != *(unsigned *)"-@@-") ;
            
            GLint vlen = unsigned(fshader - vshader);
            GLint flen = binary.getSize() - vlen - sizeof(unsigned);

            fshader += sizeof(unsigned);

            glShaderSource(_vShader, 1, &vshader, &vlen);
            glCompileShader(_vShader);
            glGetShaderiv(_vShader, GL_COMPILE_STATUS, &status);

            if(status != GL_TRUE) {
                GLint  length = 0;
                glGetShaderInfoLog(_vShader, __BUFFER_MAX, &length, __buffer);
                log.msgError("PlatformShader::PlatformShader / vshader / %s", __buffer);
                glDeleteShader(_vShader);
                _vShader = 0;
            }

            glShaderSource(_fShader, 1, &fshader, &flen);
            glCompileShader(_fShader);
            glGetShaderiv(_fShader, GL_COMPILE_STATUS, &status);

            if(status != GL_TRUE) {
                GLint  length = 0;
                glGetShaderInfoLog(_fShader, __BUFFER_MAX, &length, __buffer);
                log.msgError("PlatformShader::PlatformShader / fshader / %s", __buffer);
                glDeleteShader(_fShader);
                _fShader = 0;
            }

            glAttachShader(_program, _vShader);
            glAttachShader(_program, _fShader);

            for (unsigned i = 0; i < _countof(__vertexAttributes); i++) {
                glBindAttribLocation(_program, __vertexAttributes[i].index, __vertexAttributes[i].name);
            }
            for (unsigned i = 0; i < _countof(__instanceAttributes); i++) {
                glBindAttribLocation(_program, __instanceAttributes[i].index + __INSTANCE_ATTRIBUTE_LOCATION_START, __instanceAttributes[i].name);
            }

            glLinkProgram(_program);
            glGetProgramiv(_program, GL_LINK_STATUS, &status);

            if(status != GL_TRUE) {
                GLint  length = 0;
                glGetProgramInfoLog(_program, __BUFFER_MAX, &length, __buffer);
                log.msgError("PlatformShader::PlatformShader / program / %s", __buffer);
                glDeleteProgram(_program);
                _program = 0;
            }

            for (unsigned i = 0; i < unsigned(platform::ShaderConstBufferUsing::_ShaderConstBufferUsingCount); i++) {
                GLuint uboPos = glGetUniformBlockIndex(_program, __uboNames[i]);

                if (uboPos != -1) {
                    glUniformBlockBinding(_program, uboPos, i);
                }
            }

            glUseProgram(_program);

            for(unsigned i = 0; i < FG_TEXTURE_UNITS_MAX; i++) {
                _textureLocations[i] = glGetUniformLocation(_program, __textureSamplerNames[i]);

                if(_textureLocations[i] != -1) {
                    glUniform1i(_textureLocations[i], i);
                }
                else {
                    _textureLocations[i] = 0;
                }
            }
        }

        PlatformShader::~PlatformShader() {
            glDeleteShader(_vShader);
            glDeleteShader(_fShader);
            glDeleteProgram(_program);
        }

        void PlatformShader::set() {
            glUseProgram(_program);
        }

        bool PlatformShader::valid() const {
            return _vShader != 0 && _fShader != 0 && _program != 0;
        }

        GLuint PlatformShader::getProgram() const {
            return _program;
        }

        //---

        PlatformShaderConstantBuffer::PlatformShaderConstantBuffer(OpenGLES3Win32Platform *owner, unsigned index, unsigned length) : PlatformObject(owner) {
            _index = index;
            _length = length;
            glGenBuffers(1, &_ubo);
        }

        PlatformShaderConstantBuffer::~PlatformShaderConstantBuffer() {
            glDeleteBuffers(1, &_ubo);
        }

        void PlatformShaderConstantBuffer::set() {
            glBindBufferBase(GL_UNIFORM_BUFFER, _index, _ubo);
        }

        void PlatformShaderConstantBuffer::update(const void *data, unsigned byteWidth) {
            glBindBuffer(GL_UNIFORM_BUFFER, _ubo);
            glBufferData(GL_UNIFORM_BUFFER, _length, data, GL_DYNAMIC_DRAW);
        }

        bool PlatformShaderConstantBuffer::valid() const {
            return _ubo != 0;
        }

        GLuint PlatformShaderConstantBuffer::getUBO() const {
            return _ubo;
        }

        //---
        
        PlatformTexture2D::PlatformTexture2D() : PlatformObject(nullptr) {
            _format = platform::TextureFormat::UNKNOWN;
        }

        PlatformTexture2D::PlatformTexture2D(OpenGLES3Win32Platform *owner, platform::TextureFormat fmt, unsigned originWidth, unsigned originHeight, unsigned mipCount) : PlatformObject(owner) {
            _owner->resetLastTextures();
            _format = fmt;
            _width = originWidth;
            _height = originHeight;
            _mipCount = mipCount;

            glGenTextures(1, &_texture);
            glBindTexture(GL_TEXTURE_2D, _texture);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTexStorage2D(GL_TEXTURE_2D, mipCount, __nativeTextureInternalFormat[unsigned(fmt)], originWidth, originHeight);            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        
        PlatformTexture2D::PlatformTexture2D(OpenGLES3Win32Platform *owner, unsigned char *const *imgMipsBinaryData, unsigned originWidth, unsigned originHeight, unsigned mipCount, platform::TextureFormat fmt) : PlatformObject(owner) {
            _owner->resetLastTextures();
            _format = fmt;
            _width = originWidth;
            _height = originHeight;
            _mipCount = mipCount;

            glGenTextures(1, &_texture);
            glBindTexture(GL_TEXTURE_2D, _texture);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTexStorage2D(GL_TEXTURE_2D, mipCount, __nativeTextureInternalFormat[unsigned(_format)], originWidth, originHeight);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
            for(unsigned i = 0; i < mipCount; i++) {
                unsigned curWidth = originWidth >> i;
                unsigned curHeight = originHeight >> i;

                glTexSubImage2D(GL_TEXTURE_2D, i, 0, 0, curWidth, curHeight, __nativeTextureFormat[unsigned(_format)], GL_UNSIGNED_BYTE, imgMipsBinaryData[i]);
            }

            glBindTexture(GL_TEXTURE_2D, 0);
        }

        PlatformTexture2D::~PlatformTexture2D() {
            glDeleteTextures(1, &_texture);
        }

        unsigned PlatformTexture2D::getWidth() const {
            return _width;
        }

        unsigned PlatformTexture2D::getHeight() const {
            return _height;
        }

        unsigned PlatformTexture2D::getMipCount() const {
            return _mipCount;
        }

        void PlatformTexture2D::update(unsigned mip, unsigned x, unsigned y, unsigned w, unsigned h, void *src) {
            _owner->resetLastTextures();
            glBindTexture(GL_TEXTURE_2D, _texture);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexSubImage2D(GL_TEXTURE_2D, mip, x, y, w, h, __nativeTextureFormat[unsigned(_format)], GL_UNSIGNED_BYTE, src);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        void PlatformTexture2D::set(platform::TextureSlot slot) {            
            glActiveTexture(GL_TEXTURE0 + unsigned(slot)); //
            glBindTexture(GL_TEXTURE_2D, _texture);            
        }

        bool PlatformTexture2D::valid() const {
            return _texture != 0;
        }

        GLuint PlatformTexture2D::get() const {
            return _texture;
        }

        //---

        PlatformTextureCube::PlatformTextureCube() : PlatformObject(nullptr) {

        }

        PlatformTextureCube::PlatformTextureCube(OpenGLES3Win32Platform *owner, unsigned char **imgMipsBinaryData[6], unsigned originSize, unsigned mipCount, platform::TextureFormat format) : PlatformObject(owner) {
            _owner->resetLastTextures();
            _format = format;
            
            glGenTextures(1, &_texture);
            glBindTexture(GL_TEXTURE_CUBE_MAP, _texture);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            unsigned faces[] = {
                GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
            };

            glTexStorage2D(GL_TEXTURE_CUBE_MAP, mipCount, __nativeTextureInternalFormat[unsigned(_format)], originSize, originSize);

            for (unsigned i = 0; i < 6; i++) {
                for (unsigned c = 0; c < mipCount; c++) {
                    unsigned curSize = originSize >> c;
                    glTexSubImage2D(faces[i], c, 0, 0, curSize, curSize, __nativeTextureFormat[unsigned(_format)], GL_UNSIGNED_BYTE, imgMipsBinaryData[i][c]);
                }
            }

            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        }

        PlatformTextureCube::~PlatformTextureCube() {
            glDeleteTextures(1, &_texture);
        }

        void PlatformTextureCube::set(platform::TextureSlot slot) {
            glActiveTexture(GL_TEXTURE0 + unsigned(slot)); //
            glBindTexture(GL_TEXTURE_CUBE_MAP, _texture);
        }

        bool PlatformTextureCube::valid() const {
            return _texture != 0;
        }

        GLuint PlatformTextureCube::get() const {
            return _texture;
        }

        //---

        PlatformRenderTarget::PlatformRenderTarget() : PlatformObject(nullptr) {

        }

        PlatformRenderTarget::PlatformRenderTarget(OpenGLES3Win32Platform *owner, unsigned colorTargetCount, unsigned originWidth, unsigned originHeight, platform::RenderTargetType type, platform::TextureFormat fmt) : PlatformObject(owner) {
            _owner->resetLastTextures();

            _type = type;
            _colorTargetCount = colorTargetCount;

            _depthTexture._format = platform::TextureFormat::UNKNOWN;
            _depthTexture._width = originWidth;
            _depthTexture._height = originHeight;
            _depthTexture._mipCount = 1;

            if (colorTargetCount > FG_RENDERTARGETS_MAX) {
                return;
            }

            glGenFramebuffers(1, &_fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

            if (_type == platform::RenderTargetType::Normal || _type == platform::RenderTargetType::OnlyDepthNullColor) {
                glGenTextures(1, &_depthTexture._texture);
                glBindTexture(GL_TEXTURE_2D, _depthTexture._texture);
                glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, originWidth, originHeight);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture._texture, 0);
            }

            if (_type == platform::RenderTargetType::Normal || _type == platform::RenderTargetType::OnlyColorNullDepth) {
                for (unsigned i = 0; i < _colorTargetCount; i++) {
                    _renderTextures[i]._format = fmt;
                    _renderTextures[i]._width = originWidth;
                    _renderTextures[i]._height = originHeight;
                    _renderTextures[i]._mipCount = 1;

                    glGenTextures(1, &_renderTextures[i]._texture);
                    glBindTexture(GL_TEXTURE_2D, _renderTextures[i]._texture);
                    glTexStorage2D(GL_TEXTURE_2D, 1, __nativeTextureFormat[unsigned(fmt)], originWidth, originHeight);

                    for (unsigned i = 0; i < _colorTargetCount; i++) {
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, _renderTextures[i]._texture, 0);
                    }
                }
            }

            _isValid = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        PlatformRenderTarget::~PlatformRenderTarget() {
            for (unsigned i = 0; i < FG_RENDERTARGETS_MAX; i++) {
                if (_renderTextures[i]._texture) {
                    glDeleteTextures(1, &_renderTextures[i]._texture);
                }
            }

            if (_depthTexture._texture) {
                glDeleteTextures(1, &_depthTexture._texture);
            }

            glDeleteFramebuffers(1, &_fbo);
        }

        const platform::Texture2DInterface *PlatformRenderTarget::getDepthBuffer() const {
            return &_depthTexture;
        }

        const platform::Texture2DInterface *PlatformRenderTarget::getRenderBuffer(unsigned index) const {
            return &_renderTextures[index];
        }
        
        unsigned PlatformRenderTarget::getRenderBufferCount() const {
            return _colorTargetCount;
        }

        unsigned PlatformRenderTarget::getWidth() const {
            return _width;
        }

        unsigned PlatformRenderTarget::getHeight() const {
            return _height;
        }
        
        void PlatformRenderTarget::set() {
            if (_owner->cmpxLastFBO(_fbo)) {
                glViewport(0, 0, _width, _height);
                glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
            }
        }

        bool PlatformRenderTarget::valid() const {
            return _isValid;
        }

        GLuint PlatformRenderTarget::getFBO() const {
            return _fbo;
        }

        //---

        PlatformCubeRenderTarget::PlatformCubeRenderTarget() : PlatformObject(nullptr) {

        }

        PlatformCubeRenderTarget::PlatformCubeRenderTarget(OpenGLES3Win32Platform *owner, unsigned originSize, platform::RenderTargetType type, platform::TextureFormat fmt) : PlatformObject(owner) {
            _owner->resetLastTextures();
            _type = type;
            _depthTexture._format = platform::TextureFormat::UNKNOWN;
            _depthTexture._width = originSize;
            _depthTexture._height = originSize;
            _depthTexture._mipCount = 1;
            _renderCube._format = fmt;
            _depthTexture._width = originSize;
            _depthTexture._height = originSize;
            _depthTexture._mipCount = 1;
            _size = originSize;

            glGenFramebuffers(1, &_fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

            if (_type == platform::RenderTargetType::Normal || _type == platform::RenderTargetType::OnlyDepthNullColor) {
                glGenTextures(1, &_depthTexture._texture);
                glBindTexture(GL_TEXTURE_2D, _depthTexture._texture);
                glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, originSize, originSize);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture._texture, 0);
            }

            if (_type == platform::RenderTargetType::Normal || _type == platform::RenderTargetType::OnlyColorNullDepth) {
                glGenTextures(1, &_renderCube._texture);
                glBindTexture(GL_TEXTURE_CUBE_MAP, _renderCube._texture);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

                glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, __nativeTextureFormat[unsigned(fmt)], originSize, originSize);
                glBindTexture(GL_TEXTURE_CUBE_MAP, 0);     

                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, _renderCube._texture, 0);
            }
            
            _isValid = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        PlatformCubeRenderTarget::~PlatformCubeRenderTarget() {
            if (_renderCube._texture) {
                glDeleteTextures(1, &_renderCube._texture);
            }

            if (_depthTexture._texture) {
                glDeleteTextures(1, &_depthTexture._texture);
            }

            glDeleteFramebuffers(1, &_fbo);
        }

        const platform::Texture2DInterface *PlatformCubeRenderTarget::getDepthBuffer() const {
            return &_depthTexture;
        }

        const platform::TextureCubeInterface *PlatformCubeRenderTarget::getRenderBuffer() const {
            return &_renderCube;
        }

        unsigned PlatformCubeRenderTarget::getSize() const {
            return _size;
        }

        void PlatformCubeRenderTarget::set(unsigned faceIndex) {
            if (_owner->cmpxLastFBO(_fbo)) {
                glViewport(0, 0, _size, _size);
                glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
            }

            unsigned faces[] = {
                GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
            };
            
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, faces[faceIndex], _renderCube._texture, 0);
        }

        bool PlatformCubeRenderTarget::valid() const {
            return _isValid;
        }

        GLuint PlatformCubeRenderTarget::getFBO() const {
            return _fbo;
        }

        //---

        OpenGLES3Win32Platform::OpenGLES3Win32Platform(const diag::LogInterface &log) : _log(log) {

        }

        bool OpenGLES3Win32Platform::init(const platform::InitParams &initParams) {
            EGLint lastError = EGL_SUCCESS;
            OpenGLES3Win32PlatformInitParams &params = (OpenGLES3Win32PlatformInitParams &)initParams;
            
            _hwnd = params.hWindow;
            _hdc = GetDC(_hwnd);
            _eglDisplay = eglGetDisplay(_hdc);

            _nativeWidth = params.scrWidth;
            _nativeHeight = params.scrHeight;
            
            if(_eglDisplay != EGL_NO_DISPLAY) {
                EGLint eglMajorVersion;
                EGLint eglMinorVersion;
                
                if(eglInitialize(_eglDisplay, &eglMajorVersion, &eglMinorVersion)) {
                    EGLint configsReturned;
                    EGLint configurationAttributes[] = {
                        EGL_DEPTH_SIZE, 24,
                        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                        EGL_NONE,
                    };

                    if(eglChooseConfig(_eglDisplay, configurationAttributes, &_eglConfig, 1, &configsReturned) && configsReturned == 1) {
                        _eglSurface = eglCreateWindowSurface(_eglDisplay, _eglConfig, _hwnd, nullptr);
                        lastError = eglGetError();
                        
                        if(_eglSurface != EGL_NO_SURFACE && lastError == EGL_SUCCESS) {                            
                            eglBindAPI(EGL_OPENGL_ES_API);
                            lastError = eglGetError();

                            if(lastError == EGL_SUCCESS) {
                                EGLint contextAttributes[] = {
                                    EGL_CONTEXT_CLIENT_VERSION, 3,
                                    //0x30FB, 1,
                                    EGL_NONE,
                                };

                                _eglContext = eglCreateContext(_eglDisplay, _eglConfig, NULL, contextAttributes);
                                lastError = eglGetError();
                                
                                if(lastError == EGL_SUCCESS) {
                                    eglMakeCurrent(_eglDisplay, _eglSurface, _eglSurface, _eglContext);
                                    lastError = eglGetError();

                                    if(lastError == EGL_SUCCESS) {
                                        eglSwapInterval(_eglDisplay, params.syncInterval);

                                        glEnable(GL_CULL_FACE);
                                        glFrontFace(GL_CCW);

                                        glDisable(GL_BLEND);
                                        glEnable(GL_DEPTH_TEST);

                                        for(unsigned i = 0; i < FG_TEXTURE_UNITS_MAX; i++) {
                                            _lastTextureWidth[i] = 0.0f;
                                            _lastTextureHeight[i] = 0.0f;
                                        }

                                        _curRTWidth = unsigned(_nativeWidth);
                                        _curRTHeight = unsigned(_nativeHeight);

                                        
                                        //glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, (GLint *)&_defaultRenderTarget._renderTextures[0]._texture);
                                        //glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, (GLint *)&_defaultRenderTarget._depthTexture._texture);

                                        _defaultRenderTarget._owner = this;
                                        _defaultRenderTarget._colorTargetCount = 1;
                                        _defaultRenderTarget._depthTexture._width = unsigned(_nativeWidth);
                                        _defaultRenderTarget._depthTexture._height = unsigned(_nativeHeight);
                                        _defaultRenderTarget._depthTexture._mipCount = 1;
                                        _defaultRenderTarget._renderTextures[0]._format = fg::platform::TextureFormat::BGRA8;
                                        _defaultRenderTarget._renderTextures[0]._width = unsigned(_nativeWidth);
                                        _defaultRenderTarget._renderTextures[0]._height = unsigned(_nativeHeight);
                                        _defaultRenderTarget._renderTextures[0]._mipCount = 1; 
                                        _defaultRenderTarget._width = unsigned(_nativeWidth);
                                        _defaultRenderTarget._height = unsigned(_nativeHeight);
                                        return true;
                                    }
                                }
                            }
                        }

                        _log.msgError("OpenGLES3Win32Platform::init / can't initialize OpelGL ES 3: lastError = %x", lastError);
                        eglMakeCurrent(_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                        eglTerminate(_eglDisplay);
                        ReleaseDC(_hwnd, _hdc);
                        return false;
                    }
                }
            }

            _log.msgError("OpenGLES3Win32Platform::init / can't initialize OpelGL ES 3: display/config initialization error", lastError);
            ReleaseDC(_hwnd, _hdc);
            return false;
        }

        void OpenGLES3Win32Platform::destroy() {
            eglMakeCurrent(_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            eglTerminate(_eglDisplay);
            ReleaseDC(_hwnd, _hdc);

            _eglDisplay = nullptr;
            _eglConfig = nullptr;
            _eglSurface = nullptr;
            _eglContext = nullptr;
        }

        float OpenGLES3Win32Platform::getScreenWidth() const {
            return _nativeWidth;
        }

        float OpenGLES3Win32Platform::getScreenHeight() const {
            return _nativeHeight;
        }

        float OpenGLES3Win32Platform::getCurrentRTWidth() const {
            return float(_curRTWidth);
        }

        float OpenGLES3Win32Platform::getCurrentRTHeight() const {
            return float(_curRTHeight);
        }

        float OpenGLES3Win32Platform::getTextureWidth(platform::TextureSlot slot) const {
            return _lastTextureWidth[unsigned(slot)];
        }

        float OpenGLES3Win32Platform::getTextureHeight(platform::TextureSlot slot) const {
            return _lastTextureHeight[unsigned(slot)];
        }

        unsigned OpenGLES3Win32Platform::getMemoryUsing() const {
            return 0;
        }

        unsigned OpenGLES3Win32Platform::getMemoryLimit() const {
            return 0;
        }

        unsigned long long OpenGLES3Win32Platform::getTimeMs() const {
            unsigned __int64 ttime;
            GetSystemTimeAsFileTime((FILETIME *)&ttime);
            return ttime / 10000;
        }

        void OpenGLES3Win32Platform::updateOrientation() {
        
        }

        void OpenGLES3Win32Platform::resize(float width, float height) {
            glViewport(0, 0, GLsizei(width), GLsizei(height));
        }

        const math::m3x3 &OpenGLES3Win32Platform::getInputTransform() const {
            static math::m3x3 _idmat;
            return _idmat;
        }

        void OpenGLES3Win32Platform::fsFormFilesList(const char *path, std::string &out) {
            struct fn {
                static void formList(const char *path, std::string &out) {
                    char     searchStr[260];
                    unsigned soff = 0;

                    for(; path[soff] != 0; soff++) searchStr[soff] = path[soff];
                    searchStr[soff++] = '\\';
                    searchStr[soff++] = '*';
                    searchStr[soff++] = '.';
                    searchStr[soff++] = '*';
                    searchStr[soff++] = 0;

                    WIN32_FIND_DATAA data;
                    HANDLE findHandle = FindFirstFileA(searchStr, &data);

                    if(findHandle != HANDLE(-1)) {
                        do {
                            if(data.cFileName[0] != '.') {
                                if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                                    unsigned int toff = 0;
                                    char         tpath[260];

                                    for(; path[toff] != 0; toff++) tpath[toff] = path[toff];
                                    tpath[toff++] = '\\';
                                    for(unsigned c = 0; data.cFileName[c] != 0; c++, toff++) tpath[toff] = data.cFileName[c];
                                    tpath[toff] = 0;

                                    formList(tpath, out);
                                }
                                else {
                                    out += path;
                                    out += "\\";
                                    out += data.cFileName;
                                    out += "\n";
                                }
                            }
                        }
                        while(FindNextFileA(findHandle, &data));
                    }
                }
            };

            fn::formList(path, out);
        }

        bool OpenGLES3Win32Platform::fsLoadFile(const char *path, void **oBinaryDataPtr, unsigned int *oSize) {
            FILE *fp = nullptr;
            fopen_s(&fp, path, "rb");

            if(fp) {
                fseek(fp, 0, SEEK_END);
                *oSize = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                *oBinaryDataPtr = new char[*oSize];
                fread(*oBinaryDataPtr, 1, *oSize, fp);

                fclose(fp);
                return true;
            }
            return false;
        }

        bool OpenGLES3Win32Platform::fsSaveFile(const char *path, void *iBinaryDataPtr, unsigned iSize) {
            FILE *fp = nullptr;
            fopen_s(&fp, path, "wb");

            if(fp) {
                fwrite(iBinaryDataPtr, 1, iSize, fp);
                fclose(fp);
                return true;
            }
            return false;
        }

        void OpenGLES3Win32Platform::sndSetGlobalVolume(float volume) {
        
        }

        platform::SoundEmitterInterface *OpenGLES3Win32Platform::sndCreateEmitter(unsigned sampleRate, unsigned channels) {
            return nullptr;
        }

        platform::VertexBufferInterface *OpenGLES3Win32Platform::rdCreateVertexBuffer(platform::VertexType vtype, unsigned vcount, bool isDynamic, const void *data) {
            auto result = new PlatformVertexBuffer (this, vtype, vcount, isDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

            if (result->valid()) {
                if (data) {
                    memcpy(result->lock(false), data, result->getLength());
                    result->unlock();
                }
            }
            else {
                _log.msgError("cant't create vertex buffer");
                delete result;
                result = nullptr;
            }

            return result;
        }

        platform::IndexedVertexBufferInterface *OpenGLES3Win32Platform::rdCreateIndexedVertexBuffer(platform::VertexType vtype, unsigned vcount, unsigned ushortIndexCount, bool isDynamic, const void *vdata, const void *idata) {
            auto result = new PlatformIndexedVertexBuffer(this, vtype, vcount, ushortIndexCount, isDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

            if (result->valid()) {
                if (vdata) {
                    memcpy(result->lockVertices(false), vdata, result->getVertexDataLength());
                    result->unlockVertices();
                }
                if (idata) {
                    memcpy(result->lockIndices(false), idata, result->getIndexDataLength());
                    result->unlockIndices();
                }
            }
            else {
                _log.msgError("cant't create indexed vertex buffer");
                delete result;
                result = nullptr;
            }

            return result;
        }

        platform::InstanceDataInterface *OpenGLES3Win32Platform::rdCreateInstanceData(platform::InstanceDataType type, unsigned instanceCount) {
            auto result = new PlatformInstanceData (this, type, instanceCount);

            if (result->valid() == false) {
                _log.msgError("cant't create instance data");
                delete result;
                result = nullptr;
            }

            return result;
        }

        platform::ShaderInterface *OpenGLES3Win32Platform::rdCreateShader(const byteinput &binary) {
            auto result = new PlatformShader (this, binary, _log);

            if (result->valid() == false) {
                _log.msgError("cant't create shader");
                delete result;
                result = nullptr;
            }

            return result;
        }

        platform::RenderStateInterface *OpenGLES3Win32Platform::rdCreateRenderState(platform::CullMode cullMode, platform::BlendMode blendMode, bool depthEnabled, platform::DepthFunc depthCmpFunc, bool depthWriteEnabled) {
            auto result = new PlatformRenderState (this, cullMode, blendMode, depthEnabled, depthCmpFunc, depthWriteEnabled);

            if (result->valid() == false) {
                _log.msgError("cant't create rasterizer params");
                delete result;
                result = nullptr;
            }

            return result;
        }

        platform::SamplerInterface *OpenGLES3Win32Platform::rdCreateSampler(platform::TextureFilter filter, platform::TextureAddressMode addrMode, float minLod, float bias) {
            auto result = new PlatformSampler (this, filter, addrMode, minLod, bias);

            if (result->valid() == false) {
                _log.msgError("cant't create sampler");
                delete result;
                result = nullptr;
            }

            return result;
        }

        platform::ShaderConstantBufferInterface *OpenGLES3Win32Platform::rdCreateShaderConstantBuffer(platform::ShaderConstBufferUsing appoint, unsigned byteWidth) {
            auto result = new PlatformShaderConstantBuffer (this, unsigned(appoint), byteWidth);

            if (result->valid() == false) {
                _log.msgError("cant't create uniform buffer");
                delete result;
                result = nullptr;
            }

            return result;
        }

        platform::Texture2DInterface *OpenGLES3Win32Platform::rdCreateTexture2D(unsigned char *const *imgMipsBinaryData, unsigned originWidth, unsigned originHeight, unsigned mipCount, platform::TextureFormat fmt) {
            auto result = new PlatformTexture2D (this, imgMipsBinaryData, originWidth, originHeight, mipCount, fmt);

            if (result->valid() == false) {
                _log.msgError("cant't create 2d texture from memory");
                delete result;
                result = nullptr;
            }

            return result;
        }

        platform::Texture2DInterface *OpenGLES3Win32Platform::rdCreateTexture2D(platform::TextureFormat format, unsigned originWidth, unsigned originHeight, unsigned mipCount) {
            auto result = new PlatformTexture2D (this, format, originWidth, originHeight, mipCount);

            if (result->valid() == false) {
                _log.msgError("cant't create 2d texture");
                delete result;
                result = nullptr;
            }

            return result;
        }

        platform::TextureCubeInterface *OpenGLES3Win32Platform::rdCreateTextureCube(unsigned char **imgMipsBinaryData[6], unsigned originSize, unsigned mipCount, platform::TextureFormat fmt) {
            auto result = new PlatformTextureCube (this, imgMipsBinaryData, originSize, mipCount, fmt);

            if (result->valid() == false) {
                _log.msgError("cant't create cube texture");
                delete result;
                result = nullptr;
            }

            return result;
        }

        platform::RenderTargetInterface *OpenGLES3Win32Platform::rdCreateRenderTarget(unsigned colorTargetCount, unsigned originWidth, unsigned originHeight, platform::RenderTargetType type, platform::TextureFormat fmt) {
            auto result = new PlatformRenderTarget (this, colorTargetCount, originWidth, originHeight, type, fmt);

            if (result->valid() == false) {
                _log.msgError("cant't create render target");
                delete result;
                result = nullptr;
            }

            return result;
        }

        platform::CubeRenderTargetInterface *OpenGLES3Win32Platform::rdCreateCubeRenderTarget(unsigned originSize, platform::RenderTargetType type, platform::TextureFormat fmt) {
            auto result = new PlatformCubeRenderTarget (this, originSize, type, fmt);

            if (result->valid() == false) {
                _log.msgError("cant't create cube render target");
                delete result;
                result = nullptr;
            }

            return result;
        }

        platform::RenderTargetInterface *OpenGLES3Win32Platform::rdGetDefaultRenderTarget() {
            return &_defaultRenderTarget;
        }

        void OpenGLES3Win32Platform::rdClearCurrentRenderTarget(const color &c, float depth) {
            glClearDepthf(depth);
            glClearColor(std::pow(c.r, 1.0f / 2.2f), std::pow(c.g, 1.0f / 2.2f), std::pow(c.b, 1.0f / 2.2f), std::pow(c.a, 1.0f / 2.2f));
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        void OpenGLES3Win32Platform::rdSetRenderTarget(const platform::RenderTargetInterface *rt) {
            PlatformRenderTarget *platformObject = (PlatformRenderTarget *)rt;
            
            if (platformObject) {
                platformObject->set();

                _lastFBO = platformObject->getFBO();
                _curRTWidth = platformObject->getWidth();
                _curRTHeight = platformObject->getHeight();
                _curRTColorTargetCount = platformObject->getRenderBufferCount();
            }
        }

        void OpenGLES3Win32Platform::rdSetCubeRenderTarget(const platform::CubeRenderTargetInterface *rt, unsigned faceIndex) {
            PlatformCubeRenderTarget *platformObject = (PlatformCubeRenderTarget *)rt;
            
            if (platformObject) {
                platformObject->set(faceIndex);

                _lastFBO = platformObject->getFBO();
                _curRTWidth = platformObject->getSize();
                _curRTHeight = platformObject->getSize();
            }
        }

        void OpenGLES3Win32Platform::rdSetShader(const platform::ShaderInterface *vshader) {
            PlatformShader *platformObject = (PlatformShader *)vshader;
            
            if (platformObject && _lastProgram != platformObject->getProgram()) {
                _lastProgram = platformObject->getProgram();

                if (platformObject) {
                    platformObject->set();
                }

                resetLastVBO();
                resetLastIBO();
            }
        }

        void OpenGLES3Win32Platform::rdSetRenderState(const platform::RenderStateInterface *params) {
            PlatformRenderState *platformObject = (PlatformRenderState *)params;
            
            if (platformObject) {
                platformObject->set();
            }
        }

        void OpenGLES3Win32Platform::rdSetSampler(platform::TextureSlot slot, const platform::SamplerInterface *sampler) {
            PlatformSampler *platformObject = (PlatformSampler *)sampler;
            
            if (platformObject && _lastSampler != platformObject->getSampler()) {
                platformObject->set(slot);
                _lastSampler = platformObject->getSampler();
            }
        }

        void OpenGLES3Win32Platform::rdSetShaderConstBuffer(const platform::ShaderConstantBufferInterface *cbuffer) {
            PlatformShaderConstantBuffer *platformObject = (PlatformShaderConstantBuffer *)cbuffer;
            
            if (platformObject && _lastUBO != platformObject->getUBO()) {
                platformObject->set();
                _lastUBO = platformObject->getUBO();
            }
        }

        void OpenGLES3Win32Platform::rdSetTexture2D(platform::TextureSlot slot, const platform::Texture2DInterface *texture) {
            PlatformTexture2D *platformObject = (PlatformTexture2D *)texture;
            
            if (platformObject && _lastTextures[unsigned(slot)] != platformObject->get()) {
                platformObject->set(slot);

                _lastTextureWidth[unsigned(slot)] = float(platformObject->getWidth());
                _lastTextureHeight[unsigned(slot)] = float(platformObject->getHeight());
                _lastTextures[unsigned(slot)] = platformObject->get();
            }
        }

        void OpenGLES3Win32Platform::rdSetTextureCube(platform::TextureSlot slot, const platform::TextureCubeInterface *texture) {
            PlatformTextureCube *platformObject = (PlatformTextureCube *)texture;

            if (platformObject && _lastTextures[unsigned(slot)] != platformObject->get()) {
                platformObject->set(slot);
                _lastTextures[unsigned(slot)] = platformObject->get();
            }
        }
        
        void OpenGLES3Win32Platform::rdSetScissorRect(const math::p2d &topLeft, const math::p2d &bottomRight) {
            glScissor(int(topLeft.x), int(topLeft.y), int(bottomRight.x - topLeft.x), int(bottomRight.y - topLeft.y));
        }
        
        void OpenGLES3Win32Platform::rdDrawGeometry(const platform::VertexBufferInterface *vbuffer, const platform::InstanceDataInterface *instanceData, platform::PrimitiveTopology topology, unsigned vertexCount, unsigned instanceCount) {
            PlatformVertexBuffer *platformObject = (PlatformVertexBuffer *)vbuffer;
            PlatformInstanceData *platformInstanceData = (PlatformInstanceData *)instanceData;

            unsigned instIndex = unsigned(platformInstanceData->getType());
            unsigned vtypeIndex = unsigned(platformObject->getType());
            int vertexLayoutSize = __inputVertexLayoutSizes[vtypeIndex];
            int instanceLayoutSize = __inputInstanceLayoutSizes[instIndex];

            __LayoutDesc *vertexLayoutDesc = __inputVertexLayouts[vtypeIndex];
            __LayoutDesc *instanceLayoutDesc = __inputInstanceLayouts[instIndex];

            if (vertexLayoutDesc && instanceLayoutDesc) {
                if (_lastVBO != platformObject->getVBO() || _lastVertexLayoutDesc != vertexLayoutDesc) {
                    _lastVBO = platformObject->getVBO();
                    _lastVertexLayoutDesc = vertexLayoutDesc;

                    glBindBuffer(GL_ARRAY_BUFFER, _lastVBO);

                    for (int i = 0; i < vertexLayoutSize; i++) {
                        glVertexAttribPointer(i, vertexLayoutDesc[i].componentCount, vertexLayoutDesc[i].componentType, vertexLayoutDesc[i].normalized ? GL_TRUE : GL_FALSE, vertexLayoutDesc[i].stride, (const GLvoid *)vertexLayoutDesc[i].offset);

                        if (i > _lastEnabledVertexAttibArray) {
                            glVertexAttribDivisor(i, 0);
                            glEnableVertexAttribArray(i);
                        }
                    }

                    for (int i = vertexLayoutSize; i < _lastEnabledVertexAttibArray; i++) {
                        glDisableVertexAttribArray(i);
                    }

                    _lastEnabledVertexAttibArray = vertexLayoutSize;
                }

                if (_lastInstanceLayoutDesc != instanceLayoutDesc) {
                    _lastInstanceLayoutDesc = instanceLayoutDesc;

                    glBindBuffer(GL_ARRAY_BUFFER, platformInstanceData->getVBO());

                    for (int i = 0; i < instanceLayoutSize; i++) {
                        glVertexAttribPointer(__INSTANCE_ATTRIBUTE_LOCATION_START + i, instanceLayoutDesc[i].componentCount, instanceLayoutDesc[i].componentType, instanceLayoutDesc[i].normalized ? GL_TRUE : GL_FALSE, instanceLayoutDesc[i].stride, (const GLvoid *)instanceLayoutDesc[i].offset);

                        if (i > _lastEnabledInstanceAttibArray) {
                            glVertexAttribDivisor(__INSTANCE_ATTRIBUTE_LOCATION_START + i, 1);
                            glEnableVertexAttribArray(__INSTANCE_ATTRIBUTE_LOCATION_START + i);
                        }
                    }

                    for (int i = instanceLayoutSize; i < _lastEnabledInstanceAttibArray; i++) {
                        glDisableVertexAttribArray(__INSTANCE_ATTRIBUTE_LOCATION_START + i);
                    }

                    _lastEnabledInstanceAttibArray = instanceLayoutSize;
                }

                glDrawArraysInstanced(__nativeTopology[unsigned(topology)], 0, vertexCount, instanceCount); //
            }
        }
        
        void OpenGLES3Win32Platform::rdDrawIndexedGeometry(const platform::IndexedVertexBufferInterface *ivbuffer, const platform::InstanceDataInterface *instanceData, platform::PrimitiveTopology topology, unsigned indexCount, unsigned instanceCount) {
            PlatformIndexedVertexBuffer *platformObject = (PlatformIndexedVertexBuffer *)ivbuffer;
            PlatformInstanceData *platformInstanceData = (PlatformInstanceData *)instanceData;

            unsigned instIndex = unsigned(platformInstanceData->getType());
            unsigned vtypeIndex = unsigned(platformObject->getType());
            int vertexLayoutSize = __inputVertexLayoutSizes[vtypeIndex];
            int instanceLayoutSize = __inputInstanceLayoutSizes[instIndex];

            __LayoutDesc *vertexLayoutDesc = __inputVertexLayouts[vtypeIndex];
            __LayoutDesc *instanceLayoutDesc = __inputInstanceLayouts[instIndex];

            if (vertexLayoutDesc && instanceLayoutDesc) {
                if (_lastVBO != platformObject->getVBO() || _lastVertexLayoutDesc != vertexLayoutDesc) {
                    _lastVBO = platformObject->getVBO();
                    _lastVertexLayoutDesc = vertexLayoutDesc;

                    glBindBuffer(GL_ARRAY_BUFFER, _lastVBO);

                    for (int i = 0; i < vertexLayoutSize; i++) {
                        glVertexAttribPointer(i, vertexLayoutDesc[i].componentCount, vertexLayoutDesc[i].componentType, vertexLayoutDesc[i].normalized ? GL_TRUE : GL_FALSE, vertexLayoutDesc[i].stride, (const GLvoid *)vertexLayoutDesc[i].offset);

                        if (i >= _lastEnabledVertexAttibArray) {
                            glVertexAttribDivisor(i, 0);
                            glEnableVertexAttribArray(i);
                        }
                    }

                    for (int i = vertexLayoutSize; i < _lastEnabledVertexAttibArray; i++) {
                        glDisableVertexAttribArray(i);
                    }

                    _lastEnabledVertexAttibArray = vertexLayoutSize;
                }
                if (_lastIBO != platformObject->getIBO()) {
                    _lastIBO = platformObject->getIBO();
                    
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _lastIBO);
                }

                if (_lastInstanceLayoutDesc != instanceLayoutDesc) {
                    _lastInstanceLayoutDesc = instanceLayoutDesc;

                    glBindBuffer(GL_ARRAY_BUFFER, platformInstanceData->getVBO());

                    for (int i = 0; i < instanceLayoutSize; i++) {
                        glVertexAttribPointer(__INSTANCE_ATTRIBUTE_LOCATION_START + i, instanceLayoutDesc[i].componentCount, instanceLayoutDesc[i].componentType, instanceLayoutDesc[i].normalized ? GL_TRUE : GL_FALSE, instanceLayoutDesc[i].stride, (const GLvoid *)instanceLayoutDesc[i].offset);

                        if (i >= _lastEnabledInstanceAttibArray) {
                            glVertexAttribDivisor(__INSTANCE_ATTRIBUTE_LOCATION_START + i, 1);
                            glEnableVertexAttribArray(__INSTANCE_ATTRIBUTE_LOCATION_START + i);
                        }
                    }

                    for (int i = instanceLayoutSize; i < _lastEnabledInstanceAttibArray; i++) {
                        glDisableVertexAttribArray(__INSTANCE_ATTRIBUTE_LOCATION_START + i);
                    }

                    _lastEnabledInstanceAttibArray = instanceLayoutSize;
                }

                glDrawElementsInstanced(__nativeTopology[unsigned(topology)], indexCount, GL_UNSIGNED_SHORT, nullptr, instanceCount); //
            }
        }

        void OpenGLES3Win32Platform::rdPreframe() {

        }
        
        void OpenGLES3Win32Platform::rdPresent() {            
            if(!eglSwapBuffers(_eglDisplay, _eglSurface)) {
                _log.msgError("OpenGLES3Win32Platform::rdPresent / opengl %x error\n", eglGetError());
            }

            GLenum invalidate[] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
            glInvalidateFramebuffer(GL_FRAMEBUFFER, 3, invalidate);
        }

        bool OpenGLES3Win32Platform::isInited() const {
            return _eglContext != nullptr;
        }

        void OpenGLES3Win32Platform::resetLastVBO() {
            _lastVBO = 0;
            _lastInstanceLayoutDesc = nullptr;
            _lastVertexLayoutDesc = nullptr;
        }

        void OpenGLES3Win32Platform::resetLastIBO() {
            _lastIBO = 0;
        }

        void OpenGLES3Win32Platform::resetLastProgram() {
            _lastProgram = 0;
        }

        void OpenGLES3Win32Platform::resetLastTextures() {
            for (unsigned i = 0; i < unsigned(platform::TextureSlot::_TextureSlotCount); i++) {
                _lastTextures[i] = 0;
            }
        }

        bool OpenGLES3Win32Platform::cmpxLastFBO(GLuint fbo) {
            if (_lastFBO != fbo) {
                _lastFBO = fbo;
                return true;
            }

            return false;
        }

        bool OpenGLES3Win32Platform::cmpxLastDepthCmpFunc(GLuint func) {
            if (_lastDepthFunc != func) {
                _lastDepthFunc = func;
                return true;
            }

            return false;
        }

        bool OpenGLES3Win32Platform::cmpxLastDepthEnabled(bool enabled) {
            if (_lastDepthEnabled != enabled) {
                _lastDepthEnabled = enabled;
                return true;
            }

            return false;
        }

        bool OpenGLES3Win32Platform::cmpxLastDepthWriteEnabled(bool enabled) {
            if (_lastDepthWriteEnabled != enabled) {
                _lastDepthWriteEnabled = enabled;
                return true;
            }

            return false;
        }

        bool OpenGLES3Win32Platform::cmpxLastCullMode(platform::CullMode cullMode) {
            if (_lastCullMode != cullMode) {
                _lastCullMode = cullMode;
                return true;
            }

            return false;
        }

        bool OpenGLES3Win32Platform::cmpxLastBlendMode(platform::BlendMode blendMode) {
            if (_lastBlendMode != blendMode) {
                _lastBlendMode = blendMode;
                return true;
            }

            return false;
        }
    }
}




