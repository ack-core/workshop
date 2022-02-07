
#include "rendering.h"
#include <MetalKit/MetalKit.h>
#include <unordered_map>

namespace foundation {
    class MetalShader : public RenderShader {
    public:
        MetalShader(id<MTLLibrary> library, MTLVertexDescriptor *vdesc, std::uint32_t constBufferLength);
        ~MetalShader() override;
        
        id<MTLFunction> getVertexShader() const;
        id<MTLFunction> getFragmentShader() const;
        MTLVertexDescriptor *getVertexDesc() const;
        std::uint32_t getConstBufferLength() const;
        
    private:
        id<MTLLibrary> _library;
        MTLVertexDescriptor *_vdesc;
        std::uint32_t _constBufferLength;
    };
    
    class MetalTexture : public RenderTexture {
    public:
        MetalTexture(id<MTLTexture> texture, id<MTLTexture> depth, RenderTextureFormat fmt, std::uint32_t w, std::uint32_t h, std::uint32_t mipCount);
        ~MetalTexture() override;
        
        void replace(id<MTLTexture> texture);
        
        std::uint32_t getWidth() const override;
        std::uint32_t getHeight() const override;
        std::uint32_t getMipCount() const override;
        
        RenderTextureFormat getFormat() const override;
        MTLPixelFormat getDepthFormat() const;
        
        id<MTLTexture> getColor() const;
        id<MTLTexture> getDepth() const;
        
    private:
        RenderTextureFormat _format;
        MTLPixelFormat _depthFormat;

        id<MTLTexture> _texture;
        id<MTLTexture> _depth;

        std::uint32_t _width;
        std::uint32_t _height;
        std::uint32_t _mipCount;
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
        
        void updateFrameConstants(const float(&VP)[16], const float(&invVP)[16], const float(&view)[16], const float(&proj)[16], const float(&camPos)[3], const float(&camDir)[3]) override;
        
        RenderShaderPtr createShader(const char *name, const char *src, const RenderShaderInputDesc &vtx, const RenderShaderInputDesc &itc) override;
        RenderTexturePtr createTexture(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, const std::initializer_list<const void *> &mipsData) override;
        RenderTexturePtr createRenderTarget(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, bool withZBuffer) override;
        RenderDataPtr createData(const void *data, std::uint32_t count, std::uint32_t stride) override;
        
        float getBackBufferWidth() const override;
        float getBackBufferHeight() const override;
        
        void beginPass(const char *name, const RenderShaderPtr &shader, const RenderPassConfig &cfg) override;
        
        void applyShaderConstants(const void *constants) override;
        void applyTextures(const RenderTexturePtr *textures, std::uint32_t count) override;
        
        void drawGeometry(const RenderDataPtr &vertexData, std::uint32_t vcount, RenderTopology topology) override;
        void drawGeometry(const RenderDataPtr &vertexData, const RenderDataPtr &instanceData, std::uint32_t vcount, std::uint32_t icount, RenderTopology topology) override;
        
        void endPass() override;
        void presentFrame() override;
        
    private:
        static const std::uint32_t CONSTANT_BUFFER_FRAMES_MAX = 4;
        static const std::uint32_t CONSTANT_BUFFER_OFFSET_MAX = 4 * 1024;
        
        struct FrameConstants {
            float vewProjMatrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};     // view * proj matrix
            float invVewProjMatrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};     // inverted view * proj matrix
            float viewMatrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};     // view matrix
            float projMatrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};     // proj matrix
            float cameraPosition[4] = {0, 0, 0, 1};
            float cameraDirection[4] = {1, 0, 0, 1};
            float rtBounds[4] = {0, 0, 0, 0};
        }
        _frameConstants;
        
        std::shared_ptr<PlatformInterface> _platform;
        MTKView *_view = nil;
        
        id<MTLDevice> _device = nil;
        id<MTLCommandQueue> _commandQueue = nil;
        
        std::unordered_map<std::string, id<MTLRenderPipelineState>> _pipelineStates;
        std::shared_ptr<RenderShader> _currentShader;
        
        id<MTLCommandBuffer> _currentCommandBuffer = nil;
        id<MTLRenderCommandEncoder> _currentCommandEncoder = nil;
        MTLRenderPassDescriptor *_currentPassDescriptor = nil;
        
        id<MTLDepthStencilState> _defaultDepthStencilState = nil;
        
        bool _constantsBuffersInUse[CONSTANT_BUFFER_FRAMES_MAX] = {false};
        id<MTLBuffer> _constantsBuffers[CONSTANT_BUFFER_FRAMES_MAX];
        std::uint32_t _constantsBuffersIndex = 0;
        std::uint32_t _constantsBufferOffset = 0;
    };
}
