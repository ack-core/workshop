
#include <sys/types.h>

#include <dirent.h>
#include <unistd.h>
#include <x11/xlib.h>
#include <egl/egl.h>
#include <gles3/gl31.h>

namespace fg {
    namespace opengl {
        class OpenGLES3X11Platform;
        class PlatformObject : public uncopyable{
        public:
            PlatformObject(OpenGLES3X11Platform *owner) : _owner(owner) {}
            virtual ~PlatformObject() {}
            virtual bool valid() const {
                return true;
            }

        protected:
            OpenGLES3X11Platform *_owner = nullptr;
        };

        //---

        class PlatformVertexBuffer : public PlatformObject, public platform::VertexBufferInterface {
            friend class OpenGLES3X11Platform;

        public:
            ~PlatformVertexBuffer() override;
            
            GLuint getVBO() const;
            platform::VertexType getType() const;
            
            void *lock(bool doNotWait) override;
            void unlock() override;
            bool valid() const override;

            unsigned getLength() const;

        protected:
            PlatformVertexBuffer(OpenGLES3X11Platform *owner, platform::VertexType type, unsigned vcount, GLenum usage);
            
            GLenum    _usage;
            GLuint    _vbo = 0;
            unsigned  _length = 0;
        
            platform::VertexType _type;
        };

        //---

        class PlatformIndexedVertexBuffer : public PlatformObject, public platform::IndexedVertexBufferInterface {
            friend class OpenGLES3X11Platform;
        
        public:
            ~PlatformIndexedVertexBuffer() override;

            GLuint getVBO() const;
            GLuint getIBO() const;
            platform::VertexType getType() const;

            void *lockVertices(bool doNotWait) override;
            void *lockIndices(bool doNotWait) override;
            void unlockVertices() override;
            void unlockIndices() override;
            bool valid() const override;

            unsigned getVertexDataLength() const;
            unsigned getIndexDataLength() const;

        protected:
            PlatformIndexedVertexBuffer(OpenGLES3X11Platform *owner, platform::VertexType type, unsigned vcount, unsigned icount, GLenum usage);
            
            GLenum    _usage;
            GLuint    _vbo = 0;
            GLuint    _ibo = 0;
            unsigned  _ilength = 0;
            unsigned  _vlength = 0;

            platform::VertexType _type;
        };

        //---

        class PlatformInstanceData : public PlatformObject, public platform::InstanceDataInterface {
            friend class OpenGLES3X11Platform;

        public:
            ~PlatformInstanceData() override;

            GLuint getVBO() const;
            
            void update(const void *data, unsigned instanceCount) override;
            bool valid() const override;

            platform::InstanceDataType getType() const;

        protected:
            PlatformInstanceData(OpenGLES3X11Platform *owner, platform::InstanceDataType type, unsigned instanceCount);
            
            GLuint    _vbo = 0;
            unsigned  _length = 0;
            platform::InstanceDataType _type;
        };

        //---

        class PlatformRenderState : public PlatformObject, public platform::RenderStateInterface {
            friend class OpenGLES3X11Platform;

        public:
            ~PlatformRenderState() override;

            void set();
            bool valid() const override;

        protected:
            PlatformRenderState(OpenGLES3X11Platform *owner, platform::CullMode cullMode, platform::BlendMode blendMode, bool depthEnabled, platform::DepthFunc depthCmpFunc, bool depthWriteEnabled);
            
            platform::CullMode  _cullMode;
            platform::BlendMode _blendMode;
            
            bool   _depthEnabled;
            bool   _depthWriteEnabled;            
            GLenum _cmpFunc;
        };

        //---

        class PlatformSampler : public PlatformObject, public platform::SamplerInterface {
            friend class OpenGLES3X11Platform;

        public:
            ~PlatformSampler() override;

            void set(platform::TextureSlot slot);
            bool valid() const override;

            GLuint getSampler() const;

        protected:
            PlatformSampler(OpenGLES3X11Platform *owner, platform::TextureFilter filter, platform::TextureAddressMode addrMode, float minLod, float bias);
            GLuint _self = 0;
        };

        //---

        class PlatformShader : public PlatformObject, public platform::ShaderInterface {
            friend class OpenGLES3X11Platform;

        public:
            ~PlatformShader() override;

            void set();
            bool valid() const override;
            
            GLuint getProgram() const;

        public:
            PlatformShader(OpenGLES3X11Platform *owner, const byteinput &binary, const diag::LogInterface &log);
            
            GLuint _program = 0;
            GLuint _vShader = 0;
            GLuint _fShader = 0;
            GLuint _textureLocations[FG_TEXTURE_UNITS_MAX] = {0};
        };

        //---

        class PlatformShaderConstantBuffer : public PlatformObject, public platform::ShaderConstantBufferInterface {
            friend class OpenGLES3X11Platform;

        public:
            ~PlatformShaderConstantBuffer() override;

            void set();
            void update(const void *data, unsigned byteWidth) override;
            bool valid() const override;

            GLuint getUBO() const;

        protected:
            PlatformShaderConstantBuffer(OpenGLES3X11Platform *owner, unsigned index, unsigned length);
            
            GLuint    _ubo = 0;
            unsigned  _length = 0;
            unsigned  _index;
        };

        //---

        class PlatformTexture2D : public PlatformObject, public platform::Texture2DInterface {
            friend class PlatformRenderTarget;
            friend class PlatformCubeRenderTarget;
            friend class OpenGLES3X11Platform;

        public:
            ~PlatformTexture2D() override;

            unsigned getWidth() const override;
            unsigned getHeight() const override;
            unsigned getMipCount() const override;

            void update(unsigned mip, unsigned x, unsigned y, unsigned w, unsigned h, void *src) override;

            void set(platform::TextureSlot slot);
            bool valid() const override;

            GLuint get() const;

        protected:
            PlatformTexture2D();
            PlatformTexture2D(OpenGLES3X11Platform *owner, platform::TextureFormat fmt, unsigned originWidth, unsigned originHeight, unsigned mipCount);
            PlatformTexture2D(OpenGLES3X11Platform *owner, unsigned char *const *imgMipsBinaryData, unsigned originWidth, unsigned originHeight, unsigned mipCount, platform::TextureFormat fmt);
            
            platform::TextureFormat _format;
            GLuint    _texture = 0;
            unsigned  _width = 0;
            unsigned  _height = 0;
            unsigned  _mipCount = 0;
        };

        //---

        class PlatformTextureCube : public PlatformObject, public platform::TextureCubeInterface {
            friend class PlatformCubeRenderTarget;
            friend class OpenGLES3X11Platform;

        public:
            ~PlatformTextureCube() override;

            void set(platform::TextureSlot slot);
            bool valid() const override;

            GLuint get() const;

        protected:
            PlatformTextureCube();
            PlatformTextureCube(OpenGLES3X11Platform *owner, unsigned char **imgMipsBinaryData[6], unsigned originSize, unsigned mipCount, platform::TextureFormat format);
            
            platform::TextureFormat _format;
            GLuint _texture = 0;
        };

        //---

        class PlatformRenderTarget : public PlatformObject, public platform::RenderTargetInterface {
            friend class OpenGLES3X11Platform;

        public:
            ~PlatformRenderTarget() override;

            const platform::Texture2DInterface *getDepthBuffer() const override;
            const platform::Texture2DInterface *getRenderBuffer(unsigned index) const override;

            unsigned getRenderBufferCount() const override;
            unsigned getWidth() const override;
            unsigned getHeight() const override;

            void  set();
            bool  valid() const override;

            GLuint getFBO() const;

        protected:
            PlatformRenderTarget();
            PlatformRenderTarget(OpenGLES3X11Platform *owner, unsigned colorTargetCount, unsigned originWidth, unsigned originHeight, platform::RenderTargetType type, platform::TextureFormat fmt);

            platform::RenderTargetType _type = platform::RenderTargetType::Normal;
            
            unsigned _colorTargetCount = 0;
            unsigned _width = 0;
            unsigned _height = 0;
            bool _isValid = false;

            PlatformTexture2D   _renderTextures[FG_RENDERTARGETS_MAX];
            PlatformTexture2D   _depthTexture;
            GLuint _fbo = 0;
        };

        //---

        class PlatformCubeRenderTarget : public PlatformObject, public platform::CubeRenderTargetInterface {
            friend class OpenGLES3X11Platform;

        public:
            ~PlatformCubeRenderTarget() override;

            const platform::Texture2DInterface *getDepthBuffer() const override;
            const platform::TextureCubeInterface *getRenderBuffer() const override;
            
            unsigned getSize() const override;

            void  set(unsigned faceIndex);
            bool  valid() const override;

            GLuint getFBO() const;

        protected:
            PlatformCubeRenderTarget();
            PlatformCubeRenderTarget(OpenGLES3X11Platform *owner, unsigned originSize, platform::RenderTargetType type, platform::TextureFormat fmt);

            platform::RenderTargetType _type = platform::RenderTargetType::Normal;
            
            unsigned _size = 0;
            bool _isValid = false;

            PlatformTextureCube _renderCube;
            PlatformTexture2D   _depthTexture;
            GLuint _fbo = 0;
        };

        //---

        struct OpenGLES3X11PlatformInitParams : public platform::InitParams {
            Window  window;
            Display *display;
        };

        class OpenGLES3X11Platform : public platform::EnginePlatformInterface, public uncopyable {
        public:
            OpenGLES3X11Platform(const diag::LogInterface &log);

            bool  init(const platform::InitParams &initParams) override;
            void  destroy() override;

            float getScreenWidth() const override;
            float getScreenHeight() const override;
            float getCurrentRTWidth() const override;
            float getCurrentRTHeight() const override;

            float getTextureWidth(platform::TextureSlot slot) const override;
            float getTextureHeight(platform::TextureSlot slot) const override;

            unsigned  getMemoryUsing() const override;
            unsigned  getMemoryLimit() const override;
            unsigned  long long getTimeMs() const override;

            void  updateOrientation() override;
            void  resize(float width, float height) override;
            
            const math::m3x3  &getInputTransform() const override;

            void  fsFormFilesList(const char *path, std::string &out) override;
            bool  fsLoadFile(const char *path, void **oBinaryDataPtr, unsigned int *oSize) override;
            bool  fsSaveFile(const char *path, void *iBinaryDataPtr, unsigned iSize) override;
            void  sndSetGlobalVolume(float volume) override;

            platform::SoundEmitterInterface          *sndCreateEmitter(unsigned sampleRate, unsigned channels) override;
            platform::VertexBufferInterface          *rdCreateVertexBuffer(platform::VertexType vtype, unsigned vcount, bool isDynamic, const void *data) override;
            platform::IndexedVertexBufferInterface   *rdCreateIndexedVertexBuffer(platform::VertexType vtype, unsigned vcount, unsigned ushortIndexCount, bool isDynamic, const void *vdata, const void *idata) override;
            platform::InstanceDataInterface          *rdCreateInstanceData(platform::InstanceDataType type, unsigned instanceCount) override;
            platform::ShaderInterface                *rdCreateShader(const byteinput &binary) override;
            platform::RenderStateInterface           *rdCreateRenderState(platform::CullMode cullMode, platform::BlendMode blendMode, bool depthEnabled, platform::DepthFunc depthCmpFunc, bool depthWriteEnabled) override;
            platform::SamplerInterface               *rdCreateSampler(platform::TextureFilter filter, platform::TextureAddressMode addrMode, float minLod, float bias) override;
            platform::ShaderConstantBufferInterface  *rdCreateShaderConstantBuffer(platform::ShaderConstBufferUsing appoint, unsigned byteWidth) override;
            platform::Texture2DInterface             *rdCreateTexture2D(unsigned char *const *imgMipsBinaryData, unsigned originWidth, unsigned originHeight, unsigned mipCount, platform::TextureFormat fmt) override;
            platform::Texture2DInterface             *rdCreateTexture2D(platform::TextureFormat format, unsigned originWidth, unsigned originHeight, unsigned mipCount) override;
            platform::TextureCubeInterface           *rdCreateTextureCube(unsigned char **imgMipsBinaryData[6], unsigned originSize, unsigned mipCount, platform::TextureFormat fmt) override;
            platform::RenderTargetInterface          *rdCreateRenderTarget(unsigned colorTargetCount, unsigned originWidth, unsigned originHeight, platform::RenderTargetType type, platform::TextureFormat fmt) override;
            platform::CubeRenderTargetInterface      *rdCreateCubeRenderTarget(unsigned originSize, platform::RenderTargetType type, platform::TextureFormat fmt) override;
            platform::RenderTargetInterface          *rdGetDefaultRenderTarget() override;

            void  rdClearCurrentRenderTarget(const color &c, float depth) override;

            void  rdSetRenderTarget(const platform::RenderTargetInterface *rt) override;
            void  rdSetCubeRenderTarget(const platform::CubeRenderTargetInterface *rt, unsigned faceIndex) override;
            void  rdSetShader(const platform::ShaderInterface *vshader) override;
            void  rdSetRenderState(const platform::RenderStateInterface *params) override;
            void  rdSetSampler(platform::TextureSlot slot, const platform::SamplerInterface *sampler) override;
            void  rdSetShaderConstBuffer(const platform::ShaderConstantBufferInterface *cbuffer) override;
            void  rdSetTexture2D(platform::TextureSlot, const platform::Texture2DInterface *texture) override;
            void  rdSetTextureCube(platform::TextureSlot slot, const platform::TextureCubeInterface *texture) override;
            void  rdSetScissorRect(const math::p2d &topLeft, const math::p2d &bottomRight) override; 

            void  rdDrawGeometry(const platform::VertexBufferInterface *vbuffer, const platform::InstanceDataInterface *instanceData, platform::PrimitiveTopology topology, unsigned vertexCount, unsigned instanceCount) override;
            void  rdDrawIndexedGeometry(const platform::IndexedVertexBufferInterface *ivbuffer, const platform::InstanceDataInterface *instanceData, platform::PrimitiveTopology topology, unsigned indexCount, unsigned instanceCount) override;
            
            void  rdPreframe() override;
            void  rdPresent() override;

            bool  isInited() const override;

        public:
            void  resetLastVBO();
            void  resetLastIBO();
            void  resetLastProgram();
            void  resetLastTextures();

            bool cmpxLastFBO(GLuint fbo);
            bool cmpxLastDepthCmpFunc(GLuint func);
            bool cmpxLastDepthEnabled(bool enabled);
            bool cmpxLastDepthWriteEnabled(bool enabled);
            
            bool cmpxLastCullMode(platform::CullMode cullMode);
            bool cmpxLastBlendMode(platform::BlendMode blendMode);


        protected:
            const diag::LogInterface     &_log;
            PlatformRenderTarget  _defaultRenderTarget;
            
            void     *_lastInstanceLayoutDesc = nullptr;
            void     *_lastVertexLayoutDesc = nullptr;
            
            int      _lastEnabledInstanceAttibArray = -1;
            int      _lastEnabledVertexAttibArray = -1;
            GLuint   _lastVBO = 0;
            GLuint   _lastIBO = 0;
            GLuint   _lastFBO = 0;
            GLuint   _lastUBO = 0;
            GLuint   _lastSampler = 0;
            GLuint   _lastProgram = 0;
            GLuint   _lastTextures[unsigned(platform::TextureSlot::_TextureSlotCount)] = {0};
            GLuint   _lastDepthFunc = 0;
            bool     _lastDepthEnabled = true;
            bool     _lastDepthWriteEnabled = true;
            
            platform::CullMode  _lastCullMode = platform::CullMode::_CullModeCount;
            platform::BlendMode _lastBlendMode = platform::BlendMode::_BlendModeCount;

            unsigned    _curRTWidth = 1;
            unsigned    _curRTHeight = 1;
            unsigned    _curRTColorTargetCount = 1;

            Window      _window = 0;
            Display     *_display = nullptr;
            EGLDisplay  _eglDisplay = 0;
            EGLConfig	_eglConfig = 0;
            EGLSurface	_eglSurface = 0;
            EGLContext	_eglContext = 0;

            float       _nativeWidth = 0.0f;
            float       _nativeHeight = 0.0f;

            float       _lastTextureWidth[FG_TEXTURE_UNITS_MAX];
            float       _lastTextureHeight[FG_TEXTURE_UNITS_MAX];
        };
    }
}

