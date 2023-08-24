#pragma once
#define NOMINMAX

#include <d3d11_1.h>
#pragma comment(lib, "d3d11.lib")

#include <wrl.h>

using namespace Microsoft::WRL;

namespace native {
    class UWDirect3D11Render final : public RenderInterface {
    public:
        UWDirect3D11Render(const std::shared_ptr<PlatformInterface> &platform);
        ~UWDirect3D11Render();

        void updateCameraTransform(const float(&camPos)[3], const float(&camDir)[3], const float(&camVP)[16]) override;

        RenderShader   createShader(const std::initializer_list<RenderShaderInput> &input, const char *shadersrc, const char *name) override;
        RenderTexture  createTexture(RenderTexture::Format format, std::uint32_t width, std::uint32_t height, std::uint32_t mipCount, const std::uint8_t *const *imgMipsBinaryData = nullptr) override;
        RenderGeometry createGeometry(const void *data, std::uint32_t count, std::uint32_t stride) override;
        
        void applyShader(const RenderShader &shader, const std::initializer_list<const void *> &constants = {}) override;
        void applyShaderConstants(const RenderShader &shader, const std::initializer_list<const void *> &constants = {}) override;
        void applyTextures(const std::initializer_list<const RenderTexture *> &textures) override;
        
        void drawGeometry(const RenderGeometry *geometry, std::uint32_t vertexCount, std::uint32_t instanceCount, RenderGeometry::Topology topology) override;

        void prepareFrame() override;
        void presentFrame(float dt) override;

    private:
        struct FrameConstants {
            float vp[16];     // view * proj matrix
            float camPos[4] = {0, 0, 0, 1};
            float camDir[4] = {0, 0, 0, 1};
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

        void _initialize();
        bool _compileShader(const std::string &shader, const char *name, const char *target, ComPtr<ID3DBlob> &out);
        void _drawQuad();
    };
}