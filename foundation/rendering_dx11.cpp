
#include "rendering.h"

#ifdef PLATFORM_WINDOWS

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "shader.h"
#include "util.h"
#include "rendering_dx11.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace {
    std::weak_ptr<foundation::RenderingInterface> g_instance;

    const DXGI_FORMAT g_vertexFormatTable[] = { // index is InputAttributeFormat value
        DXGI_FORMAT_R16G16_FLOAT,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_R32_FLOAT,
        DXGI_FORMAT_R32G32_FLOAT,
        DXGI_FORMAT_R32G32B32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R16G16_SINT,
        DXGI_FORMAT_R16G16B16A16_SINT,
        DXGI_FORMAT_R16G16_UINT,
        DXGI_FORMAT_R16G16B16A16_UINT,
        DXGI_FORMAT_R16G16_SNORM,
        DXGI_FORMAT_R16G16B16A16_SNORM,
        DXGI_FORMAT_R16G16_UNORM,
        DXGI_FORMAT_R16G16B16A16_UNORM,
        DXGI_FORMAT_R8G8B8A8_UINT,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R32_SINT,
        DXGI_FORMAT_R32G32_SINT,
        DXGI_FORMAT_R32G32B32_SINT,
        DXGI_FORMAT_R32G32B32A32_SINT,
        DXGI_FORMAT_R32_UINT,
        DXGI_FORMAT_R32G32_UINT,
        DXGI_FORMAT_R32G32B32_UINT,
        DXGI_FORMAT_R32G32B32A32_UINT,
    };
}

namespace foundation {
    Direct3D11Shader::Direct3D11Shader(const InputLayout &layout, const ComPtr<ID3D11InputLayout> &inputLayout, const ComPtr<ID3D11VertexShader> &vs, const ComPtr<ID3D11PixelShader> &ps, const ComPtr<ID3D11Buffer> &cb, std::uint32_t constBufferLength)
        : _layout(layout)
        , _constBufferLength(constBufferLength)
        , _inputLayout(inputLayout)
        , _vshader(vs)
        , _pshader(ps)
        , _constBuffer(cb)
    {}

    Direct3D11Shader::~Direct3D11Shader() {}

    auto Direct3D11Shader::getInputLayout() const -> const InputLayout & {
        return _layout;
    }

    auto Direct3D11Shader::getConstBufferLength() const -> std::uint32_t {
        return _constBufferLength;
    }

    void Direct3D11Shader::apply(const ComPtr<ID3D11DeviceContext1> &context, const void *constants) const {
        context->IASetInputLayout(_inputLayout.Get());
        context->VSSetShader(_vshader.Get(), nullptr, 0);
        context->PSSetShader(_pshader.Get(), nullptr, 0);

        if (_constBuffer) {
            if (constants) {
                context->UpdateSubresource(_constBuffer.Get(), 0, nullptr, constants, 0, 0);
            }

            context->VSSetConstantBuffers(1, 1, _constBuffer.GetAddressOf());
            context->PSSetConstantBuffers(1, 1, _constBuffer.GetAddressOf());
        }
    }
}

namespace foundation {
    Direct3D11Rendering::Direct3D11Rendering(const std::shared_ptr<PlatformInterface> &platform) : _platform(platform) {
        _platform->logMsg("[RENDER] Initialization : D3D11");

        HRESULT hresult;
        unsigned flags = D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_SINGLETHREADED;

        D3D_FEATURE_LEVEL features[] = {
            D3D_FEATURE_LEVEL_10_0,
        };

        D3D_FEATURE_LEVEL featureLevel;
        ComPtr<ID3D11DeviceContext> tmpContext;
        ComPtr<ID3D11Device> tmpDevice;

        if ((hresult = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, features, 1, D3D11_SDK_VERSION, &tmpDevice, &featureLevel, &tmpContext)) != S_OK) {
            _platform->logError("[RENDER] D3D11CreateDevice failed with result = %p", hresult);
            return;
        }

        tmpDevice.As<ID3D11Device1>(&_device);
        tmpContext.As<ID3D11DeviceContext1>(&_context);

        _platform->logMsg("[RENDER] Initialization : complete");
    }

    Direct3D11Rendering::~Direct3D11Rendering() {}

    void Direct3D11Rendering::updateFrameConstants(const math::transform3f &vp, const math::transform3f &svp, const math::transform3f &ivp, const math::vector3f &camPos, const math::vector3f &camDir) {
        _stdVPMatrix = svp;
    }

    auto Direct3D11Rendering::createShader(const char *src, const InputLayout &layout) -> RenderShaderPtr {
        std::string error;
        const auto &[vs, fs, constLength] = foundation::makePlatformShaderSource(src, layout, error);

        if (error.empty() == false) {
            _platform->logError("[Direct3D11Rendering::createShader] shader error : %s\n", error.data());
            return nullptr;
        }

        HRESULT hresult;
        RenderShaderPtr result;

        ComPtr<ID3DBlob> vsBinary;
        ComPtr<ID3DBlob> psBinary;
        ComPtr<ID3DBlob> errorBlob;
        ComPtr<ID3D11Buffer> constBuffer;

        if (constLength) {
            D3D11_BUFFER_DESC bdsc = {};
            bdsc.ByteWidth = constLength;
            bdsc.Usage = D3D11_USAGE_DEFAULT;
            bdsc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

            if ((hresult = _device->CreateBuffer(&bdsc, nullptr, &constBuffer)) != S_OK) {
                _platform->logError("[Direct3D11Rendering::createShader] const buffer creation failed with result = %p", hresult);
            }
        }

        std::vector<D3D11_INPUT_ELEMENT_DESC> elements;
        const D3D11_INPUT_CLASSIFICATION classification = layout.repeat > 1 ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;

        for (std::size_t i = 0; i < layout.attributes.size(); i++) {
            const DXGI_FORMAT format = g_vertexFormatTable[int(layout.attributes[i].format)];
            elements.emplace_back(D3D11_INPUT_ELEMENT_DESC{"VTX", UINT(i), format, 0, D3D11_APPEND_ALIGNED_ELEMENT, classification, layout.repeat > 1 ? 1u : 0u});
        }

        ComPtr<ID3D11InputLayout> inputLayout;
        ComPtr<ID3D11VertexShader> vertexShader;
        ComPtr<ID3D11PixelShader> pixelShader;

        if (D3DCompile(vs.data(), vs.length(), "vs", nullptr, nullptr, "main", "vs_4_0", D3DCOMPILE_DEBUG, 0, &vsBinary, &errorBlob) == S_OK) {
            if (D3DCompile(fs.data(), fs.length(), "ps", nullptr, nullptr, "main", "ps_4_0", D3DCOMPILE_DEBUG, 0, &psBinary, &errorBlob) == S_OK) {
                if ((hresult = _device->CreateVertexShader(vsBinary->GetBufferPointer(), vsBinary->GetBufferSize(), nullptr, &vertexShader)) == S_OK) {
                    if (elements.empty() == false) {
                        if ((hresult = _device->CreateInputLayout(elements.data(), UINT(elements.size()), vsBinary->GetBufferPointer(), vsBinary->GetBufferSize(), &inputLayout)) != S_OK) {
                            _platform->logError("[Direct3D11Rendering::createShader] input layout creation failed with result = %p", hresult);
                        }
                    }

                    if ((hresult = _device->CreatePixelShader(psBinary->GetBufferPointer(), psBinary->GetBufferSize(), nullptr, &pixelShader)) == S_OK) {
                        result = std::make_shared<Direct3D11Shader>(layout, inputLayout, vertexShader, pixelShader, constBuffer, constLength);
                    }
                    else {
                        _platform->logError("[Direct3D11Rendering::createShader] pixel shader creation failed with result = %p", hresult);
                    }
                }
                else {
                    _platform->logError("[Direct3D11Rendering::createShader] vertex shader creation failed with result = %p", hresult);
                }
            }
            else {
                _platform->logError("[Direct3D11Rendering::createShader] pixel shader compilation error:\n%s\n%s\n", (const char *)errorBlob->GetBufferPointer(), shaderUtils::makeLines(fs).data());
            }
        }
        else {
            _platform->logError("[Direct3D11Rendering::createShader] vertex shader compilation error:\n%s\n%s\n", (const char *)errorBlob->GetBufferPointer(), shaderUtils::makeLines(vs).data());
        }

        return result;
    }

    auto Direct3D11Rendering::createTexture(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, const std::initializer_list<const void *> &mipsData) -> RenderTexturePtr {
        return nullptr;
    }

    auto Direct3D11Rendering::createRenderTarget(RenderTextureFormat format, std::uint32_t textureCount, std::uint32_t w, std::uint32_t h, bool withZBuffer) -> RenderTargetPtr {
        return nullptr;
    }

    auto Direct3D11Rendering::createData(const InputLayout &layout, const void *data, std::uint32_t vcnt, const std::uint32_t *indexes, std::uint32_t icnt) -> RenderDataPtr {
        return nullptr;
    }

    auto Direct3D11Rendering::getBackBufferWidth() const -> float {
        return 0.0f;
    }

    auto Direct3D11Rendering::getBackBufferHeight() const -> float {
        return 0.0f;
    }

    auto Direct3D11Rendering::getStdVPMatrix() const -> math::transform3f {
        return _stdVPMatrix;
    }

    void Direct3D11Rendering::forTarget(const RenderTargetPtr &target, const RenderTexturePtr &depth, const std::optional<math::color> &rgba, util::callback<void(RenderingInterface &)> &&pass) {

    }

    void Direct3D11Rendering::applyShader(const RenderShaderPtr &shader, RenderTopology topology, BlendType blendType, DepthBehavior depthBehavior) {}
    void Direct3D11Rendering::applyTextures(const std::initializer_list<std::pair<RenderTexturePtr, SamplerType>> &textures) {}
    void Direct3D11Rendering::applyTextures(const std::vector<std::pair<RenderTexturePtr, SamplerType>> &textures) {}
    void Direct3D11Rendering::applyShaderConstants(const void *constants) {}

    void Direct3D11Rendering::draw(std::uint32_t vertexCount) {}
    void Direct3D11Rendering::draw(const RenderDataPtr &inputData, std::uint32_t instanceCount) {}
    void Direct3D11Rendering::draw(const void *data, std::uint32_t vcnt, const std::uint32_t *indexes, std::uint32_t icnt) {}

    std::unique_ptr<std::uint8_t[]> Direct3D11Rendering::presentFrame() {
        return nullptr;
    }
}

namespace foundation {
    std::shared_ptr<RenderingInterface> RenderingInterface::instance(const std::shared_ptr<PlatformInterface> &platform) {
        std::shared_ptr<RenderingInterface> result;

        if (g_instance.use_count() == 0) {
            g_instance = result = std::make_shared<Direct3D11Rendering>(platform);
        }
        else {
            result = g_instance.lock();
        }

        return result;
    }
}

#endif // PLATFORM_WINDOWS
