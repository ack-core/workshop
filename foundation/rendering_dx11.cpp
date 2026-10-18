
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

        if ((hresult = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, features, 1, D3D11_SDK_VERSION, &tmpDevice, &featureLevel, &tmpContext)) != S_OK) {
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
        _context->OMSetRenderTargets(1, _defaultRTView.GetAddressOf(), _defaultDepthView.Get());
        _context->OMSetBlendState(_blendStates[int(BlendType::DISABLED)].Get(), nullptr, 0xffffffff);
        _context->OMSetDepthStencilState(_depthStates[int(DepthBehavior::DISABLED)].Get(), 0);
        _context->VSSetConstantBuffers(0, 1, _frameConstantsBuffer.GetAddressOf());
        _context->PSSetConstantBuffers(0, 1, _frameConstantsBuffer.GetAddressOf());
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

    }

    void Direct3D11Rendering::applyShader(const RenderShaderPtr &shader, RenderTopology topology, BlendType blendType, DepthBehavior depthBehavior) {

    }

    void Direct3D11Rendering::applyTextures(const std::initializer_list<std::pair<RenderTexturePtr, SamplerType>> &textures) {
    
    }
    void Direct3D11Rendering::applyTextures(const std::vector<std::pair<RenderTexturePtr, SamplerType>> &textures) {
    
    }
    void Direct3D11Rendering::applyShaderConstants(const void *constants) {
    
    }

    void Direct3D11Rendering::draw(std::uint32_t vertexCount) {
    
    }
    void Direct3D11Rendering::draw(const RenderDataPtr &inputData, std::uint32_t instanceCount) {

    }
    void Direct3D11Rendering::draw(const void *data, std::uint32_t vcnt, const std::uint32_t *indexes, std::uint32_t icnt) {
    
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

        // flip model unbinds back buffer after Present
        _context->OMSetRenderTargets(1, _defaultRTView.GetAddressOf(), _defaultDepthView.Get());
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
