
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
        std::shared_ptr<PlatformInterface> _platform;
        math::transform3f _stdVPMatrix;

        ComPtr<ID3D11Device1> _device;
        ComPtr<ID3D11DeviceContext1> _context;
    };
}
