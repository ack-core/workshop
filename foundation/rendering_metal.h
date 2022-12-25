
#include "rendering.h"
#include <MetalKit/MetalKit.h>
#include <unordered_map>
#include <unordered_set>

namespace foundation {
    class MetalShader : public RenderShader {
    public:
        MetalShader(const std::string &name, id<MTLLibrary> library, MTLVertexDescriptor *vdesc, std::uint32_t constBufferLength);
        ~MetalShader() override;
        
        id<MTLFunction> getVertexShader() const;
        id<MTLFunction> getFragmentShader() const;
        
        MTLVertexDescriptor *getVertexDesc() const;
        std::uint32_t getConstBufferLength() const;
        const std::string &getName() const;
        
    private:
        std::string _name;
        id<MTLLibrary> _library;
        MTLVertexDescriptor *_vdesc;
        std::uint32_t _constBufferLength;
    };

    class MetalComputeShader : public ComputeShader {
    public:
        MetalComputeShader(const std::string &name, id<MTLLibrary> library);
        ~MetalComputeShader() override;

        id<MTLFunction> getShader() const;
        const std::string &getName() const;

    private:
        std::string _name;
        id<MTLLibrary> _library;
    };

    class MetalTexBase : public RenderTexture {
    public:
        ~MetalTexBase() override {}
        
        virtual id<MTLTexture> getNativeTexture() const = 0;
    };

    class MetalTexture : public MetalTexBase {
    public:
        MetalTexture(id<MTLTexture> texture, RenderTextureFormat fmt, std::uint32_t w, std::uint32_t h, std::uint32_t mipCount);
        ~MetalTexture() override;
        
        std::uint32_t getWidth() const override;
        std::uint32_t getHeight() const override;
        std::uint32_t getMipCount() const override;
        
        RenderTextureFormat getFormat() const override;
        id<MTLTexture> getNativeTexture() const override;
        
    private:
        RenderTextureFormat _format;
        id<MTLTexture> _texture;

        std::uint32_t _width;
        std::uint32_t _height;
        std::uint32_t _mipCount;
    };
    
    class MetalTarget : public RenderTarget {
    public:
        class RTTexture : public MetalTexBase {
        public:
            RTTexture(MetalTarget &target, id<MTLTexture> texture) : _target(target), _texture(texture) {}
            ~RTTexture() override {}
            
            std::uint32_t getWidth() const override { return _target._width; }
            std::uint32_t getHeight() const override { return _target._height; }
            std::uint32_t getMipCount() const override { return 1; }
            RenderTextureFormat getFormat() const override { return _target._format; }
            id<MTLTexture> getNativeTexture() const override { return _texture; }

        private:
            MetalTarget &_target;
            id<MTLTexture> _texture;
        };

        MetalTarget(__strong id<MTLTexture> * targets, unsigned count, id<MTLTexture> depth, RenderTextureFormat fmt, std::uint32_t w, std::uint32_t h);
        ~MetalTarget() override;
        
        std::uint32_t getWidth() const override;
        std::uint32_t getHeight() const override;
        
        RenderTextureFormat getFormat() const override;
        bool hasDepthBuffer() const override;
        
        std::uint32_t getTextureCount() const override;
        std::shared_ptr<RenderTexture> getTexture(unsigned index) override;
        
        MTLPixelFormat getDepthFormat() const;
        
    private:
        RenderTextureFormat _format;
        MTLPixelFormat _depthFormat;
        
        std::shared_ptr<RTTexture> _textures[RenderTarget::MAXCOUNT] = {nil, nil, nil, nil};
        unsigned _count;
        
        id<MTLTexture> _depth;
    
        std::uint32_t _width;
        std::uint32_t _height;
    };

    class MetalData : public RenderData {
    public:
        MetalData(id<MTLBuffer> buffer, std::uint32_t count, std::uint32_t stride);
        ~MetalData() override;
        
        std::uint32_t getCount() const override;
        std::uint32_t getStride() const override;
        id<MTLBuffer> get() const;
        
    private:
        id<MTLBuffer> _buffer;
        std::uint32_t _count;
        std::uint32_t _stride;
    };
    
    class MetalRendering final : public RenderingInterface {
    public:
        MetalRendering(const std::shared_ptr<PlatformInterface> &platform);
        ~MetalRendering() override;
        
        const std::shared_ptr<PlatformInterface> &getPlatformInterface() const override;
        
        void updateFrameConstants(const float(&view)[16], const float(&proj)[16], const float(&camPos)[3], const float(&camDir)[3]) override;
        
        RenderShaderPtr createShader(const char *name, const char *src, const RenderShaderInputDesc &vtx, const RenderShaderInputDesc &itc) override;
        ComputeShaderPtr createComputeShader(const char *name, const char *src) override;
        RenderTexturePtr createTexture(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, const std::initializer_list<const void *> &mipsData) override;
        RenderTargetPtr createRenderTarget(RenderTextureFormat format, unsigned textureCount, std::uint32_t w, std::uint32_t h, bool withZBuffer) override;
        RenderDataPtr createData(const void *data, std::uint32_t count, std::uint32_t stride) override;
        
        float getBackBufferWidth() const override;
        float getBackBufferHeight() const override;
        
        void applyState(const RenderShaderPtr &shader, const RenderPassConfig &cfg) override;
        void applyShaderConstants(const void *constants) override;
        void applyTextures(const RenderTexturePtr *textures, std::uint32_t count) override;
        
        void drawGeometry(const RenderDataPtr &vertexData, std::uint32_t vcount, RenderTopology topology) override;
        void drawGeometry(const RenderDataPtr &vertexData, const RenderDataPtr &instanceData, std::uint32_t vcount, std::uint32_t icount, RenderTopology topology) override;

        void compute(const ComputeShaderPtr &shader, const RenderTexturePtr *textures, std::uint32_t count, ComputeGridSize grid) override;
        void presentFrame() override;
        
    private:
        void _appendConstantBuffer(const void *buffer, std::uint32_t size, std::uint32_t vIndex, std::uint32_t fIndex);
        
        static const std::uint32_t CONSTANT_BUFFER_FRAMES_MAX = 3;
        static const std::uint32_t CONSTANT_BUFFER_OFFSET_MAX = 1024 * 64;
        
        struct FrameConstants {
            //float vewProjMatrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};     // view * proj matrix
            //float invVewProjMatrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};  // inverted view * proj matrix
            float viewMatrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};        // view matrix
            float projMatrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};        // proj matrix
            float cameraPosition[4] = {0, 0, 0, 1};
            float cameraDirection[4] = {1, 0, 0, 1};
            float rtBounds[4] = {0, 0, 0, 0};
        }
        _frameConstants;
        
        std::shared_ptr<PlatformInterface> _platform;
        MTKView *_view = nil;
        
        id<MTLDevice> _device = nil;
        id<MTLCommandQueue> _commandQueue = nil;
        
        std::unordered_set<std::string> _shaderNames;
        std::unordered_map<std::string, id<MTLRenderPipelineState>> _renderPipelineStates;
        std::unordered_map<std::string, id<MTLComputePipelineState>> _computePipelineStates;

        id<MTLDepthStencilState> _zBehaviorStates[int(foundation::ZBehaviorType::_count)];
        std::shared_ptr<RenderShader> _currentShader;
        
        id<MTLCommandBuffer> _currentCommandBuffer = nil;
        id<MTLRenderCommandEncoder> _currentRenderCommandEncoder = nil;
        id<MTLComputeCommandEncoder> _currentComputeCommandEncoder = nil;
        MTLRenderPassDescriptor *_currentPassDescriptor = nil;
        
        bool _constantsBuffersInUse[CONSTANT_BUFFER_FRAMES_MAX] = {false};
        id<MTLBuffer> _constantsBuffers[CONSTANT_BUFFER_FRAMES_MAX];
        std::uint32_t _constantsBuffersIndex = 0;
        std::uint32_t _constantsBufferOffset = 0;
    };
}
