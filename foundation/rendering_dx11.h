
#include <d3d11_1.h>
#pragma comment(lib, "d3d11.lib")

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

#include <wrl.h>

using namespace Microsoft::WRL;

namespace foundation {
    class Direct3D11Shader : public RenderingShader {
    public:
        Direct3D11Shader(const ComPtr<ID3D11InputLayout> &layout, const ComPtr<ID3D11VertexShader> &vs, const ComPtr<ID3D11PixelShader> &ps, const ComPtr<ID3D11Buffer> &cb, std::size_t csz);
        ~Direct3D11Shader() override;

        void apply(ComPtr<ID3D11DeviceContext1> &context, const void *constants) const;

    private:
        ComPtr<ID3D11InputLayout> _layout;
        ComPtr<ID3D11VertexShader> _vshader;
        ComPtr<ID3D11PixelShader> _pshader;
        ComPtr<ID3D11Buffer> _constBuffer;
        std::size_t _constSize;
    };

    class Direct3D11Texture2D : public RenderingTexture2D {
    public:
        Direct3D11Texture2D(const ComPtr<ID3D11Texture2D> &tx, const ComPtr<ID3D11ShaderResourceView> &view, RenderingTextureFormat fmt, std::uint32_t w, std::uint32_t h, std::uint32_t mipCount);
        ~Direct3D11Texture2D() override;

        std::uint32_t getWidth() const override;
        std::uint32_t getHeight() const override;
        std::uint32_t getMipCount() const override;
        RenderingTextureFormat getFormat() const override;

        ID3D11ShaderResourceView *getSRV() const;

    private:
        ComPtr<ID3D11Texture2D>  _texture;
        ComPtr<ID3D11ShaderResourceView> _shaderResourceView;
        RenderingTextureFormat _format;
        std::uint32_t _width;
        std::uint32_t _height;
        std::uint32_t _mipCount;
    };

    class Direct3D11StructuredData : public RenderingStructuredData {
    public:
        Direct3D11StructuredData(const ComPtr<ID3D11Buffer> &buffer, std::uint32_t count, std::uint32_t stride);
        ~Direct3D11StructuredData() override;

        std::uint32_t getCount() const override;
        std::uint32_t getStride() const override;

        ID3D11Buffer *getBuffer() const;
    
    private:
        ComPtr<ID3D11Buffer> _buffer;
        std::uint32_t _count;
        std::uint32_t _stride;
    };

    class Direct3D11Rendering final : public RenderingInterface {
    public:
        Direct3D11Rendering(const std::shared_ptr<PlatformInterface> &platform);
        ~Direct3D11Rendering() override;

        void updateCameraTransform(const float(&camPos)[3], const float(&camDir)[3], const float(&camVP)[16]) override;

        std::shared_ptr<RenderingShader> createShader(const char *name, const char *shadersrc, const std::initializer_list<RenderingShader::Input> &vertex, const std::initializer_list<RenderingShader::Input> &instance) override;
        std::shared_ptr<RenderingTexture2D> createTexture(RenderingTextureFormat format, std::uint32_t width, std::uint32_t height, const std::initializer_list<const std::uint8_t *> &mipsData) override;
        std::shared_ptr<RenderingStructuredData> createData(const void *data, std::uint32_t count, std::uint32_t stride) override;

        void applyShader(const std::shared_ptr<RenderingShader> &shader, const void *constants) override;
        void applyTextures(const std::initializer_list<const RenderingTexture2D *> &textures) override;

        void drawGeometry(std::uint32_t vertexCount, RenderingTopology topology) override;
        void drawGeometry(const std::shared_ptr<RenderingStructuredData> &vdata, const std::shared_ptr<RenderingStructuredData> &idata, std::uint32_t vstart, std::uint32_t vcount, std::uint32_t istart, std::uint32_t icount, RenderingTopology topology) override;

        void prepareFrame() override;
        void presentFrame(float dt) override;

        void getFrameBufferData(std::uint8_t *imgFrame) override;

    private:
        struct FrameConstants {
            float vewProjMatrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};     // view * proj matrix
            float cameraPosition[4] = {0, 0, 0, 1};
            float cameraDirection[4] = {0, 0, 0, 1};
            float renderTargetBounds[4] = {0, 0, 0, 0};
        };

        std::shared_ptr<PlatformInterface> _platform;

        ComPtr<ID3D11Device1> _device;
        ComPtr<ID3D11DeviceContext1> _context;
        ComPtr<IDXGISwapChain1> _swapChain;

        ComPtr<ID3D11RenderTargetView> _defaultRTView;
        ComPtr<ID3D11DepthStencilView> _defaultDepthView;
        ComPtr<ID3D11ShaderResourceView> _defaultDepthShaderResourceView;
        ComPtr<ID3D11RasterizerState> _defaultRasterState;
        ComPtr<ID3D11BlendState> _defaultBlendState;
        ComPtr<ID3D11DepthStencilState> _defaultDepthState;

        ComPtr<ID3D11SamplerState> _defaultSamplerState;

        FrameConstants _frameConstantsData;
        ComPtr<ID3D11Buffer> _frameConstantsBuffer;
    };
}
