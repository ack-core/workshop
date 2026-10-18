
#include "rendering.h"

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <wrl.h>

namespace foundation {
    using Microsoft::WRL::ComPtr;

    class Direct3D11Shader : public RenderShader {
    public:
        Direct3D11Shader(const InputLayout &layout, const ComPtr<ID3D11InputLayout> &inputLayout, const ComPtr<ID3D11VertexShader> &vs, const ComPtr<ID3D11PixelShader> &ps, const ComPtr<ID3D11Buffer> &cb, std::uint32_t constBufferLength);
        ~Direct3D11Shader() override;

        auto getInputLayout() const -> const InputLayout & override;
        auto getConstBufferLength() const -> std::uint32_t override;

        void apply(const ComPtr<ID3D11DeviceContext1> &context, const void *constants) const;

    private:
        const InputLayout _layout;
        const std::uint32_t _constBufferLength;

        ComPtr<ID3D11InputLayout> _inputLayout;
        ComPtr<ID3D11VertexShader> _vshader;
        ComPtr<ID3D11PixelShader> _pshader;
        ComPtr<ID3D11Buffer> _constBuffer;
    };

    class Direct3D11Texture : public RenderTexture {
    public:
        Direct3D11Texture(const ComPtr<ID3D11Texture2D> &texture, const ComPtr<ID3D11ShaderResourceView> &view, RenderTextureFormat format, std::uint32_t w, std::uint32_t h, std::uint32_t mipCount);
        ~Direct3D11Texture() override;

        auto getWidth() const -> std::uint32_t override;
        auto getHeight() const -> std::uint32_t override;
        auto getMipCount() const -> std::uint32_t override;
        auto getFormat() const -> RenderTextureFormat override;

        auto getShaderResourceView() const -> ID3D11ShaderResourceView *;

    private:
        ComPtr<ID3D11Texture2D> _texture;
        ComPtr<ID3D11ShaderResourceView> _view;
        RenderTextureFormat _format;
        std::uint32_t _width;
        std::uint32_t _height;
        std::uint32_t _mipCount;
    };

    class Direct3D11Target : public RenderTarget {
    public:
        class DepthTexture : public Direct3D11Texture {
        public:
            DepthTexture(const ComPtr<ID3D11Texture2D> &texture, const ComPtr<ID3D11ShaderResourceView> &view, const ComPtr<ID3D11DepthStencilView> &depthView, std::uint32_t w, std::uint32_t h)
                : Direct3D11Texture(texture, view, RenderTextureFormat::R32F, w, h, 1)
                , _depthView(depthView)
            {}

            auto getDepthStencilView() const -> ID3D11DepthStencilView * { return _depthView.Get(); }

        private:
            ComPtr<ID3D11DepthStencilView> _depthView;
        };

        Direct3D11Target(const RenderTexturePtr *textures, const ComPtr<ID3D11RenderTargetView> *views, std::uint32_t count, const RenderTexturePtr &depth, RenderTextureFormat format, std::uint32_t w, std::uint32_t h);
        ~Direct3D11Target() override;

        auto getWidth() const -> std::uint32_t override;
        auto getHeight() const -> std::uint32_t override;
        auto getFormat() const -> RenderTextureFormat override;
        auto getTextureCount() const -> std::uint32_t override;
        auto getTexture(unsigned index) const -> const std::shared_ptr<RenderTexture> & override;
        auto getDepth() const -> const std::shared_ptr<RenderTexture> & override;

        auto getRenderTargetViews() const -> ID3D11RenderTargetView *const *;

    private:
        RenderTexturePtr _textures[RenderTarget::MAX_TEXTURE_COUNT];
        ComPtr<ID3D11RenderTargetView> _views[RenderTarget::MAX_TEXTURE_COUNT];
        RenderTexturePtr _depth;
        RenderTextureFormat _format;
        std::uint32_t _count;
        std::uint32_t _width;
        std::uint32_t _height;
    };

    class Direct3D11Data : public RenderData {
    public:
        Direct3D11Data(const ComPtr<ID3D11Buffer> &vertexes, const ComPtr<ID3D11Buffer> &indexes, std::uint32_t vcnt, std::uint32_t icnt, std::uint32_t stride);
        ~Direct3D11Data() override;

        auto getVertexCount() const -> std::uint32_t override;
        auto getIndexCount() const -> std::uint32_t override;
        auto getStride() const -> std::uint32_t override;

        auto getVertexes() const -> ID3D11Buffer *;
        auto getIndexes() const -> ID3D11Buffer *;

    private:
        ComPtr<ID3D11Buffer> _vertexes;
        ComPtr<ID3D11Buffer> _indexes;
        std::uint32_t _vcount;
        std::uint32_t _icount;
        std::uint32_t _stride;
    };

    class Direct3D11Rendering final : public RenderingInterface {
    public:
        Direct3D11Rendering(const std::shared_ptr<PlatformInterface> &platform);
        ~Direct3D11Rendering() override;

        void updateFrameConstants(const math::transform3f &vp, const math::transform3f &svp, const math::transform3f &ivp, const math::vector3f &camPos, const math::vector3f &camDir) override;

        auto createShader(const char *src, const InputLayout &layout) -> RenderShaderPtr override;
        auto createTexture(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, const std::initializer_list<const void *> &mipsData) -> RenderTexturePtr override;
        auto createRenderTarget(RenderTextureFormat format, std::uint32_t textureCount, std::uint32_t w, std::uint32_t h, bool withZBuffer) -> RenderTargetPtr override;
        auto createData(const InputLayout &layout, const void *data, std::uint32_t vcnt, const std::uint32_t *indexes, std::uint32_t icnt) -> RenderDataPtr override;

        auto getBackBufferWidth() const -> float override;
        auto getBackBufferHeight() const -> float override;
        auto getStdVPMatrix() const -> math::transform3f override;

        void forTarget(const RenderTargetPtr &target, const RenderTexturePtr &depth, const std::optional<math::color> &rgba, util::callback<void(RenderingInterface &)> &&pass) override;
        void applyShader(const RenderShaderPtr &shader, RenderTopology topology, BlendType blendType, DepthBehavior depthBehavior) override;
        void applyTextures(const std::initializer_list<std::pair<RenderTexturePtr, SamplerType>> &textures) override;
        void applyTextures(const std::vector<std::pair<RenderTexturePtr, SamplerType>> &textures) override;
        void applyShaderConstants(const void *constants) override;

        void draw(std::uint32_t vertexCount) override;
        void draw(const RenderDataPtr &inputData, std::uint32_t instanceCount) override;
        void draw(const void *data, std::uint32_t vcnt, const std::uint32_t *indexes, std::uint32_t icnt) override;

        auto presentFrame() -> std::unique_ptr<std::uint8_t[]> override;

    private:
        struct FrameConstants {
            math::transform3f plmVPMatrix = math::transform3f::identity();
            math::transform3f stdVPMatrix = math::transform3f::identity();
            math::transform3f invVPMatrix = math::transform3f::identity();
            math::vector4f cameraPosition = math::vector4f(0, 0, 0, 1);
            math::vector4f cameraDirection = math::vector4f(1, 0, 0, 0);
            math::vector4f rtBounds = {0, 0, 0, 0};
        }
        _frameConstants;

        std::shared_ptr<PlatformInterface> _platform;

        std::shared_ptr<Direct3D11Shader> _currentShader;
        RenderTopology _currentTopology = RenderTopology::TRIANGLES;

        ComPtr<ID3D11Device1> _device;
        ComPtr<ID3D11DeviceContext1> _context;
        ComPtr<IDXGISwapChain1> _swapChain;

        ComPtr<ID3D11RenderTargetView> _defaultRTView;
        ComPtr<ID3D11DepthStencilView> _defaultDepthView;
        ComPtr<ID3D11Buffer> _frameConstantsBuffer;

        ComPtr<ID3D11RasterizerState> _rasterState;
        ComPtr<ID3D11BlendState> _blendStates[4];               // index is BlendType value
        ComPtr<ID3D11DepthStencilState> _depthStates[4];        // index is DepthBehavior value
        ComPtr<ID3D11SamplerState> _samplerStates[2];           // index is SamplerType value

#if defined(IS_TESTS)
        ComPtr<ID3D11Texture2D> _readbackTexture;
#endif
    };
}
