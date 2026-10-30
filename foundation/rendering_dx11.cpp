
#include "rendering.h"

#ifdef PLATFORM_WINDOWS

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "shader.h"
#include "util.h"
#include "rendering_dx11.h"

#include <algorithm>
#include <cstring>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace {
    std::weak_ptr<foundation::RenderingInterface> g_instance;

    const std::uint32_t MAX_TEXTURES = 4; // texture slots in shader

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

    const struct {
        DXGI_FORMAT format;
        std::uint32_t pixelSize;
    }
    g_textureFormatTable[] = { // index is RenderTextureFormat value
        {DXGI_FORMAT_R8_UNORM, 1},
        {DXGI_FORMAT_R16_FLOAT, 2},
        {DXGI_FORMAT_R32_FLOAT, 4},
        {DXGI_FORMAT_R8G8_UNORM, 2},
        {DXGI_FORMAT_R16G16_FLOAT, 4},
        {DXGI_FORMAT_R32G32_FLOAT, 8},
        {DXGI_FORMAT_R8G8B8A8_UNORM, 4},
        {DXGI_FORMAT_R16G16B16A16_FLOAT, 8},
        {DXGI_FORMAT_R32G32B32A32_FLOAT, 16},
    };

    const D3D11_PRIMITIVE_TOPOLOGY g_topologies[] = { // index is RenderTopology value
        D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
        D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
        D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
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

    void Direct3D11Shader::apply(const ComPtr<ID3D11DeviceContext1> &context) const {
        context->IASetInputLayout(_inputLayout.Get());
        context->VSSetShader(_vshader.Get(), nullptr, 0);
        context->PSSetShader(_pshader.Get(), nullptr, 0);

        if (_constBuffer) {
            context->VSSetConstantBuffers(1, 1, _constBuffer.GetAddressOf());
            context->PSSetConstantBuffers(1, 1, _constBuffer.GetAddressOf());
        }
    }

    void Direct3D11Shader::updateConstants(const ComPtr<ID3D11DeviceContext1> &context, const void *constants) const {
        D3D11_MAPPED_SUBRESOURCE mapped;

        // discard gives a fresh memory region, so there is no stall on buffer used by previous draws
        if (context->Map(_constBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped) == S_OK) {
            std::memcpy(mapped.pData, constants, _constBufferLength);
            context->Unmap(_constBuffer.Get(), 0);
        }
    }
}

namespace foundation {
    Direct3D11Texture::Direct3D11Texture(const ComPtr<ID3D11Texture2D> &texture, const ComPtr<ID3D11ShaderResourceView> &view, RenderTextureFormat format, std::uint32_t w, std::uint32_t h, std::uint32_t mipCount)
        : _texture(texture)
        , _view(view)
        , _format(format)
        , _width(w)
        , _height(h)
        , _mipCount(mipCount)
    {}

    Direct3D11Texture::~Direct3D11Texture() {}

    auto Direct3D11Texture::getWidth() const -> std::uint32_t {
        return _width;
    }

    auto Direct3D11Texture::getHeight() const -> std::uint32_t {
        return _height;
    }

    auto Direct3D11Texture::getMipCount() const -> std::uint32_t {
        return _mipCount;
    }

    auto Direct3D11Texture::getFormat() const -> RenderTextureFormat {
        return _format;
    }

    auto Direct3D11Texture::getShaderResourceView() const -> ID3D11ShaderResourceView * {
        return _view.Get();
    }
}

namespace foundation {
    Direct3D11Target::Direct3D11Target(const RenderTexturePtr *textures, const ComPtr<ID3D11RenderTargetView> *views, std::uint32_t count, const RenderTexturePtr &depth, RenderTextureFormat format, std::uint32_t w, std::uint32_t h)
        : _depth(depth)
        , _format(format)
        , _count(count)
        , _width(w)
        , _height(h)
    {
        for (std::uint32_t i = 0; i < count; i++) {
            _textures[i] = textures[i];
            _views[i] = views[i];
        }
    }

    Direct3D11Target::~Direct3D11Target() {}

    auto Direct3D11Target::getWidth() const -> std::uint32_t {
        return _width;
    }

    auto Direct3D11Target::getHeight() const -> std::uint32_t {
        return _height;
    }

    auto Direct3D11Target::getFormat() const -> RenderTextureFormat {
        return _format;
    }

    auto Direct3D11Target::getTextureCount() const -> std::uint32_t {
        return _count;
    }

    auto Direct3D11Target::getTexture(unsigned index) const -> const std::shared_ptr<RenderTexture> & {
        return _textures[index];
    }

    auto Direct3D11Target::getDepth() const -> const std::shared_ptr<RenderTexture> & {
        return _depth;
    }

    auto Direct3D11Target::getRenderTargetViews() const -> ID3D11RenderTargetView *const * {
        return _views[0].GetAddressOf();
    }
}

namespace foundation {
    Direct3D11Data::Direct3D11Data(const ComPtr<ID3D11Buffer> &vertexes, const ComPtr<ID3D11Buffer> &indexes, std::uint32_t vcnt, std::uint32_t icnt, std::uint32_t stride)
        : _vertexes(vertexes)
        , _indexes(indexes)
        , _vcount(vcnt)
        , _icount(icnt)
        , _stride(stride)
    {}

    Direct3D11Data::~Direct3D11Data() {}

    auto Direct3D11Data::getVertexCount() const -> std::uint32_t {
        return _vcount;
    }

    auto Direct3D11Data::getIndexCount() const -> std::uint32_t {
        return _icount;
    }

    auto Direct3D11Data::getStride() const -> std::uint32_t {
        return _stride;
    }

    auto Direct3D11Data::getVertexes() const -> ID3D11Buffer * {
        return _vertexes.Get();
    }

    auto Direct3D11Data::getIndexes() const -> ID3D11Buffer * {
        return _indexes.Get();
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

#if defined(IS_TESTS)
        const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_WARP;
#else
        const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;
#endif

        if ((hresult = D3D11CreateDevice(nullptr, driverType, nullptr, flags, features, 1, D3D11_SDK_VERSION, &tmpDevice, &featureLevel, &tmpContext)) != S_OK) {
            _platform->logError("[RENDER] D3D11CreateDevice failed with result = %p", hresult);
            return;
        }

        tmpDevice.As<ID3D11Device1>(&_device);
        tmpContext.As<ID3D11DeviceContext1>(&_context);

        const std::uint32_t width = std::uint32_t(_platform->getScreenWidth());
        const std::uint32_t height = std::uint32_t(_platform->getScreenHeight());

        // default target

        _swapChain.Attach(reinterpret_cast<IDXGISwapChain1 *>(_platform->attachNativeRenderingContext(_device.Get())));

        if (_swapChain == nullptr) {
            _platform->logError("[RENDER] Swap chain attach failed");
            return;
        }

        ComPtr<ID3D11Texture2D> backBuffer;
        _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
        _device->CreateRenderTargetView(backBuffer.Get(), nullptr, &_defaultRTView);

        // default depth

        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = width;
        depthDesc.Height = height;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ComPtr<ID3D11Texture2D> depthTexture;

        if ((hresult = _device->CreateTexture2D(&depthDesc, nullptr, &depthTexture)) != S_OK) {
            _platform->logError("[RENDER] Default depth texture creation failed with result = %p", hresult);
            return;
        }

        _device->CreateDepthStencilView(depthTexture.Get(), nullptr, &_defaultDepthView);

        // frame constants

        D3D11_BUFFER_DESC frameConstantsDesc = {};
        frameConstantsDesc.ByteWidth = sizeof(FrameConstants);
        frameConstantsDesc.Usage = D3D11_USAGE_DEFAULT;
        frameConstantsDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        if ((hresult = _device->CreateBuffer(&frameConstantsDesc, nullptr, &_frameConstantsBuffer)) != S_OK) {
            _platform->logError("[RENDER] Frame constants buffer creation failed with result = %p", hresult);
            return;
        }

        D3D11_BUFFER_DESC drawConstantsDesc = {};
        drawConstantsDesc.ByteWidth = 16;
        drawConstantsDesc.Usage = D3D11_USAGE_DYNAMIC;
        drawConstantsDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        drawConstantsDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        if ((hresult = _device->CreateBuffer(&drawConstantsDesc, nullptr, &_drawConstantsBuffer)) != S_OK) {
            _platform->logError("[RENDER] Draw constants buffer creation failed with result = %p", hresult);
            return;
        }

        D3D11_BUFFER_DESC dynamicDesc = {};
        dynamicDesc.ByteWidth = DYNAMIC_BUFFER_LENGTH;
        dynamicDesc.Usage = D3D11_USAGE_DYNAMIC;
        dynamicDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER;
        dynamicDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        if ((hresult = _device->CreateBuffer(&dynamicDesc, nullptr, &_dynamicBuffer)) != S_OK) {
            _platform->logError("[RENDER] Dynamic buffer creation failed with result = %p", hresult);
            return;
        }

        // rasterizer state

        D3D11_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.DepthClipEnable = TRUE;

        _device->CreateRasterizerState(&rasterDesc, &_rasterState);

        // blend states

        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        _device->CreateBlendState(&blendDesc, &_blendStates[int(BlendType::DISABLED)]);

        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        _device->CreateBlendState(&blendDesc, &_blendStates[int(BlendType::MIXING)]);

        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        _device->CreateBlendState(&blendDesc, &_blendStates[int(BlendType::ADDITIVE)]);

        // depth states

        D3D11_DEPTH_STENCIL_DESC depthStateDesc = {};
        depthStateDesc.DepthEnable = FALSE;
        depthStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        depthStateDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        _device->CreateDepthStencilState(&depthStateDesc, &_depthStates[int(DepthBehavior::DISABLED)]);

        depthStateDesc.DepthEnable = TRUE;
        depthStateDesc.DepthFunc = D3D11_COMPARISON_GREATER;
        _device->CreateDepthStencilState(&depthStateDesc, &_depthStates[int(DepthBehavior::TEST_ONLY)]);

        depthStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        _device->CreateDepthStencilState(&depthStateDesc, &_depthStates[int(DepthBehavior::TEST_AND_WRITE)]);

        // samplers

        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        _device->CreateSamplerState(&samplerDesc, &_samplerStates[int(SamplerType::NEAREST)]);

        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        _device->CreateSamplerState(&samplerDesc, &_samplerStates[int(SamplerType::LINEAR)]);

#if defined(IS_TESTS)
        D3D11_TEXTURE2D_DESC readbackDesc = {};
        backBuffer->GetDesc(&readbackDesc);
        readbackDesc.Usage = D3D11_USAGE_STAGING;
        readbackDesc.BindFlags = 0;
        readbackDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        if ((hresult = _device->CreateTexture2D(&readbackDesc, nullptr, &_readbackTexture)) != S_OK) {
            _platform->logError("[RENDER] Readback texture creation failed with result = %p", hresult);
            return;
        }
#endif

        // initial pipeline state

        const D3D11_VIEWPORT viewport = {0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f};

        _context->RSSetViewports(1, &viewport);
        _context->RSSetState(_rasterState.Get());
        _context->OMSetBlendState(_blendStates[int(BlendType::DISABLED)].Get(), nullptr, 0xffffffff);
        _context->OMSetDepthStencilState(_depthStates[int(DepthBehavior::DISABLED)].Get(), 0);
        _context->VSSetConstantBuffers(0, 1, _frameConstantsBuffer.GetAddressOf());
        _context->PSSetConstantBuffers(0, 1, _frameConstantsBuffer.GetAddressOf());
        _context->VSSetConstantBuffers(2, 1, _drawConstantsBuffer.GetAddressOf());

        ID3D11SamplerState *samplers[] = {
            _samplerStates[int(SamplerType::NEAREST)].Get(),
            _samplerStates[int(SamplerType::NEAREST)].Get(),
            _samplerStates[int(SamplerType::NEAREST)].Get(),
            _samplerStates[int(SamplerType::NEAREST)].Get(),
        };

        _context->VSSetSamplers(0, 4, samplers);
        _context->PSSetSamplers(0, 4, samplers);

        _frameConstants.rtBounds = math::vector4f(float(width), float(height), 0.0f, 0.0f);
        _context->UpdateSubresource(_frameConstantsBuffer.Get(), 0, nullptr, &_frameConstants, 0, 0);

        _platform->logMsg("[RENDER] Initialization : complete");
    }

    Direct3D11Rendering::~Direct3D11Rendering() {}

    void Direct3D11Rendering::updateFrameConstants(const math::transform3f &vp, const math::transform3f &svp, const math::transform3f &ivp, const math::vector3f &camPos, const math::vector3f &camDir) {
        _frameConstants.plmVPMatrix = vp;
        _frameConstants.stdVPMatrix = svp;
        _frameConstants.invVPMatrix = ivp;
        _frameConstants.cameraPosition.xyz = camPos;
        _frameConstants.cameraDirection.xyz = camDir;
        _context->UpdateSubresource(_frameConstantsBuffer.Get(), 0, nullptr, &_frameConstants, 0, 0);
    }

    auto Direct3D11Rendering::createShader(const char *src, const InputLayout &layout, bool suppressErrors) -> RenderShaderPtr {
        std::string error;
        const auto &[vs, fs, constLength] = foundation::makePlatformShaderSource(src, layout, error);

        if (error.empty() == false) {
            if (suppressErrors == false) {
                _platform->logError("[Direct3D11Rendering::createShader] shader error : %s\n", error.data());
            }
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
            bdsc.Usage = D3D11_USAGE_DYNAMIC;
            bdsc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            bdsc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            if ((hresult = _device->CreateBuffer(&bdsc, nullptr, &constBuffer)) != S_OK && suppressErrors == false) {
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
                        if ((hresult = _device->CreateInputLayout(elements.data(), UINT(elements.size()), vsBinary->GetBufferPointer(), vsBinary->GetBufferSize(), &inputLayout)) != S_OK && suppressErrors == false) {
                            _platform->logError("[Direct3D11Rendering::createShader] input layout creation failed with result = %p", hresult);
                        }
                    }

                    if ((hresult = _device->CreatePixelShader(psBinary->GetBufferPointer(), psBinary->GetBufferSize(), nullptr, &pixelShader)) == S_OK) {
                        result = std::make_shared<Direct3D11Shader>(layout, inputLayout, vertexShader, pixelShader, constBuffer, constLength);
                    }
                    else if (suppressErrors == false) {
                        _platform->logError("[Direct3D11Rendering::createShader] pixel shader creation failed with result = %p", hresult);
                    }
                }
                else if (suppressErrors == false) {
                    _platform->logError("[Direct3D11Rendering::createShader] vertex shader creation failed with result = %p", hresult);
                }
            }
            else if (suppressErrors == false) {
                _platform->logError("[Direct3D11Rendering::createShader] pixel shader compilation error:\n%s\n%s\n", (const char *)errorBlob->GetBufferPointer(), foundation::makeLines(fs).data());
            }
        }
        else if (suppressErrors == false) {
            _platform->logError("[Direct3D11Rendering::createShader] vertex shader compilation error:\n%s\n%s\n", (const char *)errorBlob->GetBufferPointer(), foundation::makeLines(vs).data());
        }

        return result;
    }

    auto Direct3D11Rendering::createTexture(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, const std::initializer_list<const void *> &mipsData) -> RenderTexturePtr {
        HRESULT hresult;
        ComPtr<ID3D11Texture2D> texture;
        ComPtr<ID3D11ShaderResourceView> view;

        const auto &fmt = g_textureFormatTable[int(format)];
        const std::uint32_t mipCount = std::max(std::uint32_t(1), std::uint32_t(mipsData.size()));

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = w;
        desc.Height = h;
        desc.MipLevels = mipCount;
        desc.ArraySize = 1;
        desc.Format = fmt.format;
        desc.SampleDesc.Count = 1;
        desc.Usage = mipsData.size() ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        std::vector<D3D11_SUBRESOURCE_DATA> subresources;
        for (const void *item : mipsData) {
            const std::uint32_t mipWidth = std::max(std::uint32_t(1), w >> subresources.size());
            subresources.emplace_back(D3D11_SUBRESOURCE_DATA{item, mipWidth * fmt.pixelSize, 0});
        }

        if ((hresult = _device->CreateTexture2D(&desc, subresources.empty() ? nullptr : subresources.data(), &texture)) != S_OK) {
            _platform->logError("[Direct3D11Rendering::createTexture] texture creation failed with result = %p", hresult);
            return nullptr;
        }
        _device->CreateShaderResourceView(texture.Get(), nullptr, &view);

        return std::make_shared<Direct3D11Texture>(texture, view, format, w, h, mipCount);
    }

    auto Direct3D11Rendering::createRenderTarget(RenderTextureFormat format, std::uint32_t textureCount, std::uint32_t w, std::uint32_t h, bool withZBuffer) -> RenderTargetPtr {
        HRESULT hresult;
        RenderTexturePtr textures[RenderTarget::MAX_TEXTURE_COUNT];
        ComPtr<ID3D11RenderTargetView> views[RenderTarget::MAX_TEXTURE_COUNT];
        RenderTexturePtr depth;

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = w;
        desc.Height = h;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = g_textureFormatTable[int(format)].format;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        for (std::uint32_t i = 0; i < textureCount; i++) {
            ComPtr<ID3D11Texture2D> texture;
            ComPtr<ID3D11ShaderResourceView> view;

            if ((hresult = _device->CreateTexture2D(&desc, nullptr, &texture)) != S_OK) {
                _platform->logError("[Direct3D11Rendering::createRenderTarget] texture creation failed with result = %p", hresult);
                return nullptr;
            }
            _device->CreateRenderTargetView(texture.Get(), nullptr, &views[i]);
            _device->CreateShaderResourceView(texture.Get(), nullptr, &view);

            textures[i] = std::make_shared<Direct3D11Texture>(texture, view, format, w, h, 1);
        }

        if (withZBuffer) {
            ComPtr<ID3D11Texture2D> texture;
            ComPtr<ID3D11ShaderResourceView> view;
            ComPtr<ID3D11DepthStencilView> depthView;

            desc.Format = DXGI_FORMAT_R32_TYPELESS;
            desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

            D3D11_DEPTH_STENCIL_VIEW_DESC dsvdesc = {};
            dsvdesc.Format = DXGI_FORMAT_D32_FLOAT;
            dsvdesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

            D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc = {};
            srvdesc.Format = DXGI_FORMAT_R32_FLOAT;
            srvdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvdesc.Texture2D.MipLevels = 1;

            if ((hresult = _device->CreateTexture2D(&desc, nullptr, &texture)) != S_OK) {
                _platform->logError("[Direct3D11Rendering::createRenderTarget] depth texture creation failed with result = %p", hresult);
                return nullptr;
            }
            _device->CreateDepthStencilView(texture.Get(), &dsvdesc, &depthView);
            _device->CreateShaderResourceView(texture.Get(), &srvdesc, &view);

            depth = std::make_shared<Direct3D11Target::DepthTexture>(texture, view, depthView, w, h);
        }

        return std::make_shared<Direct3D11Target>(textures, views, textureCount, depth, format, w, h);
    }

    auto Direct3D11Rendering::createData(const InputLayout &layout, const void *data, std::uint32_t vcnt, const std::uint32_t *indexes, std::uint32_t icnt) -> RenderDataPtr {
        if (indexes && layout.repeat > 1) {
            _platform->logError("[Direct3D11Rendering::createData] Vertex repeat is incompatible with indexed data");
            return nullptr;
        }

        HRESULT hresult;
        ComPtr<ID3D11Buffer> vbuffer, ibuffer;
        const std::uint32_t stride = layout.getStride();

        D3D11_BUFFER_DESC vdesc = {};
        vdesc.ByteWidth = vcnt * stride;
        vdesc.Usage = D3D11_USAGE_IMMUTABLE;
        vdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vdata = {};
        vdata.pSysMem = data;

        if ((hresult = _device->CreateBuffer(&vdesc, &vdata, &vbuffer)) != S_OK) {
            _platform->logError("[Direct3D11Rendering::createData] vertex buffer creation failed with result = %p", hresult);
            return nullptr;
        }

        if (indexes) {
            D3D11_BUFFER_DESC idesc = {};
            idesc.ByteWidth = icnt * sizeof(std::uint32_t);
            idesc.Usage = D3D11_USAGE_IMMUTABLE;
            idesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

            D3D11_SUBRESOURCE_DATA idata = {};
            idata.pSysMem = indexes;

            if ((hresult = _device->CreateBuffer(&idesc, &idata, &ibuffer)) != S_OK) {
                _platform->logError("[Direct3D11Rendering::createData] index buffer creation failed with result = %p", hresult);
                return nullptr;
            }
        }

        return std::make_shared<Direct3D11Data>(vbuffer, ibuffer, vcnt, icnt, stride);
    }

    auto Direct3D11Rendering::getBackBufferWidth() const -> float {
        return _platform->getScreenWidth();
    }

    auto Direct3D11Rendering::getBackBufferHeight() const -> float {
        return _platform->getScreenHeight();
    }

    auto Direct3D11Rendering::getStdVPMatrix() const -> math::transform3f {
        return _frameConstants.stdVPMatrix;
    }

    void Direct3D11Rendering::forTarget(const RenderTargetPtr &target, const RenderTexturePtr &depth, const std::optional<math::color> &rgba, util::callback<void(RenderingInterface &)> &&pass) {
        ID3D11RenderTargetView *const *rtViews = _defaultRTView.GetAddressOf();
        ID3D11DepthStencilView *depthView = _defaultDepthView.Get();
        std::uint32_t rtCount = 1;

        _frameConstants.rtBounds.x = _platform->getScreenWidth();
        _frameConstants.rtBounds.y = _platform->getScreenHeight();

        if (target) {
            const Direct3D11Target *platformTarget = static_cast<const Direct3D11Target *>(target.get());
            const RenderTexturePtr &targetDepth = platformTarget->getDepth();

            rtViews = platformTarget->getRenderTargetViews();
            rtCount = platformTarget->getTextureCount();
            depthView = targetDepth ? static_cast<const Direct3D11Target::DepthTexture *>(targetDepth.get())->getDepthStencilView() : nullptr;

            _frameConstants.rtBounds.x = float(platformTarget->getWidth());
            _frameConstants.rtBounds.y = float(platformTarget->getHeight());
        }

        if (depth) {
            depthView = static_cast<const Direct3D11Target::DepthTexture *>(depth.get())->getDepthStencilView();
        }
        else if (depthView) {
            _context->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH, 0.0f, 0);
        }

        if (rgba.has_value()) {
            const float color[4] = {rgba->r, rgba->g, rgba->b, rgba->a};

            for (std::uint32_t i = 0; i < rtCount; i++) {
                _context->ClearRenderTargetView(rtViews[i], color);
            }
        }

        // textures of this target can be still bound as shader resources by the previous pass
        ID3D11ShaderResourceView *const nullResources[MAX_TEXTURES] = {nullptr};
        _context->VSSetShaderResources(0, MAX_TEXTURES, nullResources);
        _context->PSSetShaderResources(0, MAX_TEXTURES, nullResources);
        _context->OMSetRenderTargets(rtCount, rtViews, depthView);

        const D3D11_VIEWPORT viewport = {0.0f, 0.0f, _frameConstants.rtBounds.x, _frameConstants.rtBounds.y, 0.0f, 1.0f};
        _context->RSSetViewports(1, &viewport);
        _context->UpdateSubresource(_frameConstantsBuffer.Get(), 0, nullptr, &_frameConstants, 0, 0);

        _isForTarget = true;
        pass(*this);
        _isForTarget = false;
        _currentShader = nullptr;

        // release target textures so the next passes can sample them
        _context->OMSetRenderTargets(0, nullptr, nullptr);
    }

    void Direct3D11Rendering::applyShader(const RenderShaderPtr &shader, RenderTopology topology, BlendType blendType, DepthBehavior depthBehavior) {
        if (_isForTarget && shader) {
            _currentShader = std::static_pointer_cast<Direct3D11Shader>(shader);
            _currentTopology = topology;
            _currentShader->apply(_context);

            _context->IASetPrimitiveTopology(g_topologies[int(topology)]);
            _context->OMSetBlendState(_blendStates[int(blendType)].Get(), nullptr, 0xffffffff);
            _context->OMSetDepthStencilState(_depthStates[int(depthBehavior)].Get(), 0);
        }
    }

    void Direct3D11Rendering::_applyDrawConstants(std::uint32_t vertexCount) {
        D3D11_MAPPED_SUBRESOURCE mapped;

        if (_context->Map(_drawConstantsBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped) == S_OK) {
            *static_cast<std::uint32_t *>(mapped.pData) = vertexCount;
            _context->Unmap(_drawConstantsBuffer.Get(), 0);
        }
    }

    void Direct3D11Rendering::_applyTextures(const std::pair<RenderTexturePtr, SamplerType> *textures, std::size_t size) {
        if (_currentShader) {
            const UINT count = UINT(std::min(size, std::size_t(MAX_TEXTURES)));
            ID3D11ShaderResourceView *views[MAX_TEXTURES] = {nullptr};
            ID3D11SamplerState *samplers[MAX_TEXTURES] = {nullptr};

            for (UINT i = 0; i < count; i++) {
                views[i] = textures[i].first ? static_cast<const Direct3D11Texture *>(textures[i].first.get())->getShaderResourceView() : nullptr;
                samplers[i] = _samplerStates[int(textures[i].second)].Get();
            }

            _context->VSSetShaderResources(0, count, views);
            _context->VSSetSamplers(0, count, samplers);
            _context->PSSetShaderResources(0, count, views);
            _context->PSSetSamplers(0, count, samplers);
        }
    }
    void Direct3D11Rendering::applyTextures(const std::initializer_list<std::pair<RenderTexturePtr, SamplerType>> &textures) {
        _applyTextures(textures.begin(), textures.size());
    }
    void Direct3D11Rendering::applyTextures(const std::vector<std::pair<RenderTexturePtr, SamplerType>> &textures) {
        _applyTextures(textures.data(), textures.size());
    }

    void Direct3D11Rendering::applyShaderConstants(const void *constants) {
        if (_currentShader && _currentShader->getConstBufferLength()) {
            _currentShader->updateConstants(_context, constants);
        }
    }

    void Direct3D11Rendering::draw(std::uint32_t vertexCount) {
        if (_currentShader) {
            const InputLayout &layout = _currentShader->getInputLayout();
            ID3D11Buffer *const nullBuffer = nullptr;
            const UINT zero = 0;

            _context->IASetVertexBuffers(0, 1, &nullBuffer, &zero, &zero);

            if (layout.repeat > 1) {
                _applyDrawConstants(vertexCount);
                _context->DrawInstanced(layout.repeat, vertexCount, 0, 0);
            }
            else {
                _context->Draw(vertexCount, 0);
            }
        }
    }
    void Direct3D11Rendering::draw(const RenderDataPtr &inputData, std::uint32_t instanceCount) {
        if (_currentShader && inputData) {
            const InputLayout &layout = _currentShader->getInputLayout();
            const Direct3D11Data *platformData = static_cast<const Direct3D11Data *>(inputData.get());
            ID3D11Buffer *const vbuffer = platformData->getVertexes();
            const UINT stride = platformData->getStride();
            const UINT offset = 0;

            _context->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);

            if (layout.repeat > 1) {
                _applyDrawConstants(platformData->getVertexCount());
                _context->DrawInstanced(layout.repeat, platformData->getVertexCount() * instanceCount, 0, 0);
            }
            else if (platformData->getIndexes()) {
                _context->IASetIndexBuffer(platformData->getIndexes(), DXGI_FORMAT_R32_UINT, 0);
                _context->DrawIndexedInstanced(platformData->getIndexCount(), instanceCount, 0, 0, 0);
            }
            else {
                _context->DrawInstanced(platformData->getVertexCount(), instanceCount, 0, 0);
            }
        }
    }
    void Direct3D11Rendering::draw(const void *data, std::uint32_t vcnt, const std::uint32_t *indexes, std::uint32_t icnt) {
        if (_currentShader && vcnt) {
            const InputLayout &layout = _currentShader->getInputLayout();
            const UINT stride = layout.getStride();
            const UINT vlen = vcnt * stride;
            const UINT ilen = indexes ? icnt * sizeof(std::uint32_t) : 0;
            D3D11_MAP mapType = D3D11_MAP_WRITE_NO_OVERWRITE;

            if (vlen + ilen > DYNAMIC_BUFFER_LENGTH) {
                _platform->logError("[Direct3D11Rendering::draw] Out of dynamic buffer length\n");
                return;
            }
            if (_dynamicBufferOffset + vlen + ilen > DYNAMIC_BUFFER_LENGTH) {                
                mapType = D3D11_MAP_WRITE_DISCARD; // buffer is full: discard gives a fresh memory region without waiting for previous draws
                _dynamicBufferOffset = 0;
            }

            D3D11_MAPPED_SUBRESOURCE mapped;

            if (_context->Map(_dynamicBuffer.Get(), 0, mapType, 0, &mapped) == S_OK) {
                std::uint8_t *memory = static_cast<std::uint8_t *>(mapped.pData) + _dynamicBufferOffset;
                std::memcpy(memory, data, vlen);
                if (ilen) {
                    std::memcpy(memory + vlen, indexes, ilen);
                }
                _context->Unmap(_dynamicBuffer.Get(), 0);

                const UINT offset = _dynamicBufferOffset;
                _context->IASetVertexBuffers(0, 1, _dynamicBuffer.GetAddressOf(), &stride, &offset);

                if (layout.repeat > 1) {
                    _applyDrawConstants(vcnt);
                    _context->DrawInstanced(layout.repeat, vcnt, 0, 0);
                }
                else if (ilen) {
                    _context->IASetIndexBuffer(_dynamicBuffer.Get(), DXGI_FORMAT_R32_UINT, offset + vlen);
                    _context->DrawIndexed(icnt, 0, 0);
                }
                else {
                    _context->Draw(vcnt, 0);
                }

                _dynamicBufferOffset += (vlen + ilen + 15) & ~15u;
            }
        }
    }

    std::unique_ptr<std::uint8_t[]> Direct3D11Rendering::presentFrame() {
        std::unique_ptr<std::uint8_t[]> result;

#if defined(IS_TESTS)
        ComPtr<ID3D11Texture2D> backBuffer;
        _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
        _context->CopyResource(_readbackTexture.Get(), backBuffer.Get());

        D3D11_MAPPED_SUBRESOURCE mapped = {};

        if (_context->Map(_readbackTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped) == S_OK) {
            const std::uint32_t rowLength = TESTING_RENDER_WIDTH * 4;
            result = std::make_unique<std::uint8_t[]>(rowLength * TESTING_RENDER_HEIGHT);

            for (std::uint32_t i = 0; i < TESTING_RENDER_HEIGHT; i++) {
                std::memcpy(result.get() + i * rowLength, static_cast<const std::uint8_t *>(mapped.pData) + i * mapped.RowPitch, rowLength);
            }

            _context->Unmap(_readbackTexture.Get(), 0);
        }
#endif

        _swapChain->Present(1, 0);
        return result;
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
