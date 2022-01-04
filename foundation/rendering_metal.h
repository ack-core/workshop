
#include "rendering.h"
#include <MetalKit/MetalKit.h>
#include <unordered_map>

namespace foundation {
    class MetalShader : public RenderingShader {
    public:
        MetalShader(id<MTLLibrary> library, MTLVertexDescriptor *vdesc, id<MTLBuffer> constBuffer);
        ~MetalShader() override;
        
        id<MTLFunction> getVertexShader() const;
        id<MTLFunction> getFragmentShader() const;
        id<MTLBuffer> getConstantBuffer() const;
        MTLVertexDescriptor *getVertexDesc() const;
        
    private:
        id<MTLLibrary> _library;
        id<MTLBuffer> _constBuffer;
        MTLVertexDescriptor *_vdesc;
    };

    class MetalTexture : public RenderingTexture {
    public:
        MetalTexture(RenderingTextureFormat fmt, std::uint32_t w, std::uint32_t h, std::uint32_t mipCount);
        ~MetalTexture() override;

        std::uint32_t getWidth() const override;
        std::uint32_t getHeight() const override;
        std::uint32_t getMipCount() const override;
        RenderingTextureFormat getFormat() const override;

    private:
        RenderingTextureFormat _format;
        std::uint32_t _width;
        std::uint32_t _height;
        std::uint32_t _mipCount;
    };

    class MetalData : public RenderingData {
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

        void updateFrameConstants(const float(&camPos)[3], const float(&camDir)[3], const float(&camVP)[16]) override;

        RenderingShaderPtr createShader(const char *name, const char *src, const RenderingShaderInputDesc &vtx, const RenderingShaderInputDesc &itc) override;
        RenderingTexturePtr createTexture(RenderingTextureFormat format, std::uint32_t w, std::uint32_t h, const std::uint8_t * const *mips, std::uint32_t mcount) override;
        RenderingDataPtr createData(const void *data, std::uint32_t count, std::uint32_t stride) override;

        void beginPass(const char *name, const RenderingShaderPtr &shader, const RenderingPassConfig &cfg) override;
        void applyShaderConstants(const void *constants) override;
        void applyTextures(const RenderingTexture * const * textures, std::uint32_t tcount) override;

        void drawGeometry(const RenderingDataPtr &vertexData, std::uint32_t vcount, RenderingTopology topology) override;
        void drawGeometry(const RenderingDataPtr &vertexData, const RenderingDataPtr &instanceData, std::uint32_t vcount, std::uint32_t icount, RenderingTopology topology) override;

        void endPass() override;
        void presentFrame() override;

        void getFrameBufferData(std::uint8_t *imgFrame) override;

    private:
        struct FrameConstants {
            float vewProjMatrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};     // view * proj matrix
            float cameraPosition[4] = {0, 0, 0, 1};
            float cameraDirection[4] = {1, 0, 0, 1};
            float screenBounds[4] = {0, 0, 0, 0};
        };
        
        std::shared_ptr<PlatformInterface> _platform;

        MTKView *_view = nil;
        id<MTLDevice> _device = nil;
        id<MTLCommandQueue> _commandQueue = nil;
        
        std::unordered_map<std::string, id<MTLRenderPipelineState>> _pipelineStates;
        std::shared_ptr<RenderingShader> _currentShader;

        id<MTLCommandBuffer> _currentCommandBuffer = nil;
        id<MTLRenderCommandEncoder> _currentCommandEncoder = nil;
        MTLRenderPassDescriptor *_currentPassDescriptor = nil;

        id<MTLBuffer> _frameConstantsBuffer;
    };
}
