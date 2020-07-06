
#include "interfaces.h"

#include <cctype>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <regex>

#ifdef PLATFORM_WINDOWS

#define NOMINMAX

#include <d3d11_1.h>
#pragma comment(lib, "d3d11.lib")

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

#include <wrl.h>

using namespace Microsoft::WRL;

#include "rendering_dx11.h"

namespace {
    template <typename = void> std::istream &expect(std::istream &stream) {
        return stream;
    }
    template <char Ch, char... Chs> std::istream &expect(std::istream &stream) {
        (stream >> std::ws).peek() == Ch ? stream.ignore() : stream.setstate(std::ios_base::failbit);
        return expect<Chs...>(stream);
    }

    struct braced {
        explicit braced(std::string &out) : _out(out) {}

        inline friend std::istream &operator >>(std::istream &stream, const braced &target) {
            char dbg = (stream >> std::ws).peek();
            (stream >> std::ws).peek() == '[' ? (void)stream.ignore() : stream.setstate(std::ios_base::failbit);

            while (stream.fail() == false && stream.peek() != ']') {
                target._out.push_back(stream.get());
            }
            if (stream.fail() == false) {
                (stream >> std::ws).peek() == ']' ? (void)stream.ignore() : stream.setstate(std::ios_base::failbit);
            }

            return stream;
        }

    private:
        std::string &_out;
    };

    static const D3D_PRIMITIVE_TOPOLOGY g_topologies[] = {
        D3D_PRIMITIVE_TOPOLOGY_LINELIST,
        D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
    };
}

namespace foundation {
    Direct3D11Shader::Direct3D11Shader(const ComPtr<ID3D11InputLayout> &layout, const ComPtr<ID3D11VertexShader> &vs, const ComPtr<ID3D11PixelShader> &ps, const ComPtr<ID3D11Buffer> &cb, std::size_t csz)
        : _layout(layout)
        , _vshader(vs)
        , _pshader(ps)
        , _constBuffer(cb)
        , _constSize(csz)
    {}

    Direct3D11Shader::~Direct3D11Shader() {

    }

    void Direct3D11Shader::apply(ComPtr<ID3D11DeviceContext1> &context, const void *constants) {
        context->IASetInputLayout(_layout.Get());
        context->VSSetShader(_vshader.Get(), nullptr, 0);
        context->PSSetShader(_pshader.Get(), nullptr, 0);

        if (constants) {
            context->UpdateSubresource(_constBuffer.Get(), 0, nullptr, constants, 0, 0);
        }

        context->VSSetConstantBuffers(1, 1, _constBuffer.GetAddressOf());
        context->PSSetConstantBuffers(1, 1, _constBuffer.GetAddressOf());
    }

    Direct3D11Texture2D::Direct3D11Texture2D() {}
    Direct3D11Texture2D::~Direct3D11Texture2D() {}

    std::uint32_t Direct3D11Texture2D::getWidth() const {
        return _width;
    }

    std::uint32_t Direct3D11Texture2D::getHeight() const {
        return _height;
    }

    std::uint32_t Direct3D11Texture2D::getMipCount() const {
        return _mipCount;
    }

    RenderingTextureFormat Direct3D11Texture2D::getFormat() const {
        return _format;
    }

    Direct3D11StructuredData::Direct3D11StructuredData() {}
    Direct3D11StructuredData::~Direct3D11StructuredData() {}

    std::uint32_t Direct3D11StructuredData::getCount() const {
        return _count;
    }

    std::uint32_t Direct3D11StructuredData::getStride() const {
        return _stride;
    }

    Direct3D11Rendering::Direct3D11Rendering(std::shared_ptr<PlatformInterface> &platform) : _platform(platform) {
        _platform->logMsg("[RENDER] Initialization : D3D11");

        HRESULT hresult;
        unsigned flags = D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        D3D_FEATURE_LEVEL features[] = {
            D3D_FEATURE_LEVEL_10_0,
        };

        // device & context

        D3D_FEATURE_LEVEL featureLevel;
        ComPtr<ID3D11DeviceContext> tmpContext;
        ComPtr<ID3D11Device> tmpDevice;

        if ((hresult = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, features, 1, D3D11_SDK_VERSION, &tmpDevice, &featureLevel, &tmpContext)) != S_OK) {
            _platform->logError("[RENDER] D3D11CreateDevice failed with result = %p", hresult);
            return;
        }

        tmpDevice.As<ID3D11Device1>(&_device);
        tmpContext.As<ID3D11DeviceContext1>(&_context);

        unsigned width = unsigned(_platform->getNativeScreenWidth());
        unsigned height = unsigned(_platform->getNativeScreenHeight());

        _frameConstantsData.renderTargetBounds[0] = _platform->getNativeScreenWidth();
        _frameConstantsData.renderTargetBounds[1] = _platform->getNativeScreenHeight();

        _swapChain.Attach(reinterpret_cast<IDXGISwapChain1 *>(_platform->attachNativeRenderingContext(_device.Get())));

        // default RT

        if (_swapChain) {
            ComPtr<ID3D11Texture2D> defRTTexture;
            _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(defRTTexture.GetAddressOf()));

            D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = { DXGI_FORMAT_B8G8R8A8_UNORM };
            renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            renderTargetViewDesc.Texture2D.MipSlice = 0;

            _device->CreateRenderTargetView(defRTTexture.Get(), &renderTargetViewDesc, &_defaultRTView);
        }
        else {
            _platform->logError("[RENDER] Swapchain attach failed");
            return;
        }

        D3D11_VIEWPORT viewPort = {};
        viewPort.TopLeftX = 0;
        viewPort.TopLeftY = 0;
        viewPort.Width = float(width);
        viewPort.Height = float(height);
        viewPort.MinDepth = 0.0f;
        viewPort.MaxDepth = 1.0f;

        _context->RSSetViewports(1, &viewPort);

        // default Depth
        
        ComPtr<ID3D11Texture2D> defDepthTexture;
        DXGI_FORMAT  depthTexFormat = DXGI_FORMAT_R32_TYPELESS;
        DXGI_FORMAT  depthFormat = DXGI_FORMAT_D32_FLOAT;

        D3D11_TEXTURE2D_DESC depthTexDesc = {};
        depthTexDesc.Width = width;
        depthTexDesc.Height = height;
        depthTexDesc.MipLevels = 1;
        depthTexDesc.ArraySize = 1;
        depthTexDesc.Format = depthTexFormat;
        depthTexDesc.SampleDesc.Count = 1;
        depthTexDesc.SampleDesc.Quality = 0;
        depthTexDesc.Usage = D3D11_USAGE_DEFAULT;
        depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        depthTexDesc.CPUAccessFlags = 0;
        depthTexDesc.MiscFlags = 0;

        if ((hresult = _device->CreateTexture2D(&depthTexDesc, 0, &defDepthTexture)) == S_OK) {
            D3D11_DEPTH_STENCIL_VIEW_DESC depthDesc = { depthFormat };
            depthDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            depthDesc.Texture2D.MipSlice = 0;

            D3D11_SHADER_RESOURCE_VIEW_DESC depthShaderViewDesc = { DXGI_FORMAT_R32_FLOAT };
            depthShaderViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            depthShaderViewDesc.Texture2D.MipLevels = 1; //!
            depthShaderViewDesc.Texture2D.MostDetailedMip = 0;

            _device->CreateDepthStencilView(defDepthTexture.Get(), &depthDesc, &_defaultDepthView);
            _device->CreateShaderResourceView(defDepthTexture.Get(), &depthShaderViewDesc, &_defaultDepthShaderResourceView);
        }
        else {
            _platform->logError("[RENDER] Default depth texture creation failed with result = %p", hresult);
            return;
        }

        // raster state

        D3D11_RASTERIZER_DESC rasterDsc = {};
        rasterDsc.FillMode = D3D11_FILL_SOLID;
        rasterDsc.CullMode = D3D11_CULL_NONE;
        rasterDsc.FrontCounterClockwise = FALSE;
        rasterDsc.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
        rasterDsc.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
        rasterDsc.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rasterDsc.DepthClipEnable = TRUE;
        rasterDsc.ScissorEnable = FALSE;
        rasterDsc.MultisampleEnable = FALSE;
        rasterDsc.AntialiasedLineEnable = FALSE;

        if ((hresult = _device->CreateRasterizerState(&rasterDsc, &_defaultRasterState)) == S_OK) {
            _context->RSSetState(_defaultRasterState.Get());
        }
        else {
            _platform->logError("[RENDER] Default rasterizer creation failed with result = %p", hresult);
            return;
        }

        // blend state

        D3D11_BLEND_DESC blendDsc = {};
        blendDsc.AlphaToCoverageEnable = FALSE;
        blendDsc.IndependentBlendEnable = FALSE;
        blendDsc.RenderTarget[0].BlendEnable = TRUE;
        blendDsc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDsc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDsc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDsc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDsc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDsc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        blendDsc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        if ((hresult = _device->CreateBlendState(&blendDsc, &_defaultBlendState)) == S_OK) {
            _context->OMSetBlendState(_defaultBlendState.Get(), nullptr, 0xffffffff);
        }
        else {
            _platform->logError("[RENDER] Default blend state creation failed with result = %p", hresult);
            return;
        }

        // depth state

        D3D11_DEPTH_STENCIL_DESC ddesc = {};
        ddesc.DepthEnable = TRUE;
        ddesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        ddesc.DepthFunc = D3D11_COMPARISON_GREATER;
        ddesc.StencilEnable = FALSE;
        ddesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
        ddesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
        ddesc.BackFace.StencilFunc = ddesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        ddesc.BackFace.StencilDepthFailOp = ddesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        ddesc.BackFace.StencilPassOp = ddesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        ddesc.BackFace.StencilFailOp = ddesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

        if ((hresult = _device->CreateDepthStencilState(&ddesc, &_defaultDepthState)) == S_OK) {
            _context->OMSetDepthStencilState(_defaultDepthState.Get(), 0);
        }
        else {
            _platform->logError("[RENDER] Default depth/stencil state creation failed with result = %p", hresult);
            return;
        }

        // frame shader constants

        D3D11_BUFFER_DESC bdsc = {};
        bdsc.ByteWidth = sizeof(FrameConstants);
        bdsc.Usage = D3D11_USAGE_DEFAULT;
        bdsc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bdsc.CPUAccessFlags = 0;
        bdsc.MiscFlags = 0;
        bdsc.StructureByteStride = 0;

        if ((hresult = _device->CreateBuffer(&bdsc, nullptr, &_frameConstantsBuffer)) != S_OK) {
            _platform->logError("[RENDER] Default frame constant buffer creation failed with result = %p", hresult);
            return;
        }
        else {
            _context->UpdateSubresource(_frameConstantsBuffer.Get(), 0, nullptr, &_frameConstantsData, 0, 0);
        }

        // samplers

        D3D11_SAMPLER_DESC sdesc;
        sdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sdesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sdesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sdesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sdesc.MipLODBias = 0;
        sdesc.MaxAnisotropy = 1;
        sdesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sdesc.BorderColor[0] = 1.0f;
        sdesc.BorderColor[1] = 1.0f;
        sdesc.BorderColor[2] = 1.0f;
        sdesc.BorderColor[3] = 1.0f;
        sdesc.MinLOD = -FLT_MAX;
        sdesc.MaxLOD = FLT_MAX;

        if ((hresult = _device->CreateSamplerState(&sdesc, &_defaultSamplerState)) == S_OK) {
            _context->PSSetSamplers(0, 1, _defaultSamplerState.GetAddressOf());
        }
        else {
            _platform->logError("[RENDER] Default sampler creation failed with result = %p", hresult);
            return;
        }

        _platform->logMsg("[RENDER] Initialization : complete");
    }

    Direct3D11Rendering::~Direct3D11Rendering() {}

    void Direct3D11Rendering::updateCameraTransform(const float(&camPos)[3], const float(&camDir)[3], const float(&camVP)[16]) {
        ::memcpy(_frameConstantsData.vewProjMatrix, camVP, 16 * sizeof(float));
        ::memcpy(_frameConstantsData.cameraPosition, camPos, 3 * sizeof(float));
        ::memcpy(_frameConstantsData.cameraDirection, camDir, 3 * sizeof(float));

        _context->UpdateSubresource(_frameConstantsBuffer.Get(), 0, nullptr, &_frameConstantsData, 0, 0);
        _context->VSSetConstantBuffers(0, 1, _frameConstantsBuffer.GetAddressOf());
        _context->PSSetConstantBuffers(0, 1, _frameConstantsBuffer.GetAddressOf());
    }

    std::shared_ptr<RenderingShader> Direct3D11Rendering::createShader(const char *name, const char *shadersrc, const std::initializer_list<RenderingShader::Input> &vertex, const std::initializer_list<RenderingShader::Input> &instance) {
        std::istringstream stream(shadersrc);
        std::string varname, arg;

        const std::string indent = "    ";
        bool error = false;

        auto shaderGetTypeSize = [](const std::string &varname, const std::string &format, bool extended, std::string &outFormat, std::size_t *arrayCount = nullptr) {
            struct {
                const char *inputFormat;
                const char *outputFormat;
                int size;
                DXGI_FORMAT dxFormat;
            }
            typeSizeTable[] = {
                {"float4",  "float4",   16},
                {"int4",    "int4",     16},
                {"uint4",   "uint4",    16},
                {"matrix4", "float4x4", 64},
            },
            typeSizeTableEx[] = {
                {"float1",  "float1",   4},
                {"float2",  "float2",   8},
                {"float3",  "float3",   12},
                {"float4",  "float4",   16},
                {"int1",    "int1",     4},
                {"int2",    "int2",     8},
                {"int3",    "int3",     12},
                {"int4",    "int4",     16},
                {"uint1",   "uint1",    4},
                {"uint2",   "uint2",    8},
                {"uint3",   "uint3",    12},
                {"uint4",   "uint4",    16},
                {"matrix3", "float3x3", 36},
                {"matrix4", "float4x4", 64},
            };

            int multiply = 1;
            auto braceStart = varname.find('[');
            auto braceEnd = varname.rfind(']');

            if (braceStart != std::string::npos && braceEnd != std::string::npos) {
                multiply = std::max(std::stoi(varname.substr(braceStart + 1, braceEnd - braceStart - 1)), multiply);
            }

            auto begin = extended ? std::begin(typeSizeTableEx) : std::begin(typeSizeTable);
            auto end = extended ? std::end(typeSizeTableEx) : std::end(typeSizeTable);
            unsigned  result = 0;

            for (auto index = begin; index != end; ++index) {
                if (index->inputFormat == format) {
                    outFormat = index->outputFormat;
                    result = index->size * multiply;
                    break;
                }
            }

            if (arrayCount && multiply > 1) {
                *arrayCount = multiply;
            }

            return result;
        };

        auto readConstBlock = [&](const char *blockName, bool &isCreated) {
            std::string result;
            
            if (isCreated == false) {
                isCreated = true;

                while (!error && stream >> varname && varname[0] != '}') {
                    if (stream >> expect<':'> >> arg >> expect<'='>) {
                        std::size_t arrayCount = 0;
                        
                        if (unsigned typeSize = shaderGetTypeSize(varname, arg, true, arg, &arrayCount)) {
                            result += "static const " + arg + " " + varname;
                            result += " = ";

                            if (arrayCount) {
                                result += "{\n";

                                for (std::size_t i = 0; i < arrayCount; i++) {
                                    result += indent + arg + "(";

                                    if (stream >> braced(result)) {
                                        result += "),\n";
                                    }
                                    else {
                                        _platform->logError("[RENDER shader '%s'] Invalid constant '%s' initialization at '%s' block", name, varname.c_str(), blockName);
                                        error = true;
                                    }
                                }

                                result += "};\n";
                            }
                            else {
                                result += arg + "(";

                                if (stream >> braced(result)) {
                                    result += ");\n";
                                }
                                else {
                                    _platform->logError("[RENDER shader '%s'] Invalid constant '%s' initialization at '%s' block", name, varname.c_str(), blockName);
                                    error = true;
                                }
                            }                            
                        }
                        else {
                            _platform->logError("[RENDER shader '%s'] Unknown type of constant '%s' at '%s' block", name, varname.c_str(), blockName);
                            error = true;
                        }
                    }
                    else {
                        _platform->logError("[RENDER shader '%s'] Constant block '%s' syntax error", name, blockName);
                        error = true;
                    }
                }
            }
            else {
                _platform->logError("[RENDER shader '%s'] Only one '%s' block is allowed", name, blockName);
                error = true;
            }

            return result;
        };

        auto readVarsBlock = [&](const char *blockName, bool isInout, bool &isCreated, unsigned *bufferSize) {
            std::string result;
            unsigned counter = 0;

            if (isCreated == false) {
                isCreated = true;

                while (!error && stream >> varname && varname[0] != '}') {
                    if (stream >> expect<':'> >> arg) {
                        if (unsigned typeSize = shaderGetTypeSize(varname, arg, isInout, arg)) {
                            if (bufferSize) {
                                *bufferSize += typeSize;
                            }
                            result += indent + arg + " " + varname;
                            
                            if (isInout) {
                                result += ": TEXCOORD" + std::to_string(counter++);
                            }

                            result += ";\n";
                        }
                        else {
                            _platform->logError("[RENDER shader '%s'] Unknown type of constant '%s' at '%s' block", name, varname.c_str(), blockName);
                            error = true;
                        }
                    }
                    else {
                        _platform->logError("[RENDER shader '%s'] Constant block '%s' syntax error", name, blockName);
                        error = true;
                    }
                }
            }
            else {
                _platform->logError("[RENDER shader '%s'] Only one '%s' block is allowed", name, blockName);
                error = true;
            }

            return result;
        };

        auto readCodeBlock = [&](const char *blockName, bool &isCreated) {
            std::string result = indent;
            stream >> std::ws;

            if (isCreated == false) {
                isCreated = true;

                int  braceCounter = 1;
                char next;

                while (stream && ((next = stream.get()) != '}' || --braceCounter > 0)) {
                    if (next == '{') {
                        braceCounter++;
                    }
                    if (next == '\n') {
                        stream >> std::ws;
                        result += next;
                        result += indent;
                    }
                    else {
                        result += next;
                    }
                }

                if (braceCounter == 0) {
                    std::regex rgIn(R"(([^._])input_)");
                    result = std::regex_replace(result, rgIn, "$1input.");

                    std::regex rgOut(R"(([^._])output_)");
                    result = std::regex_replace(result, rgOut, "$1output.");

                    std::regex rgVertex(R"(([^._])vertex_)");
                    result = std::regex_replace(result, rgVertex, "$1input.");

                    std::regex rgInstance(R"(([^._])instance_)");
                    result = std::regex_replace(result, rgInstance, "$1input.");
                }
                else {
                    _platform->logError("[RENDER shader '%s'] Block '%s' isn't complete", name, blockName);
                    result.clear();
                    error = true;
                }
            }
            else {
                _platform->logError("[RENDER shader '%s'] Only one '%s' block is allowed", name, blockName);
                error = true;
            }

            return result;
        };

        auto makeLines = [](const std::string &src) {
            int counter = 0;

            std::string result;
            std::string::const_iterator left = std::begin(src);
            std::string::const_iterator right;

            while ((right = std::find(left, std::end(src), '\n')) != std::end(src)) {
                right++;
                
                char line[64];
                ::sprintf_s(line, "/* %3d */  ", ++counter);

                result += std::string(line) + std::string(left, right);
                left = right;
            }

            return result;
        };

        std::string vshader =
            "#define _sign(a) sign(a)\n"
            "#define _sin(a) sin(a)\n"
            "#define _cos(a) cos(a)\n"
            "#define _transform(a, b) mul(b, a)\n"
            "#define _dot(a, b) dot(a, b)\n"
            "#define _norm(a) normalize(a)\n"
            "#define _lerp(a, b, k) lerp(a, b, k)\n"
            "#define _tex2d(i, a) __t##i.Sample(__s##i, a)\n"
            "\n";

        std::string shaderConsts =
            "\n"
            "cbuffer _FrameData : register(b0) {\n"
            "    float4x4 _viewProjMatrix;\n"
            "    float4 _cameraPosition;\n"
            "    float4 _cameraDirection;\n"
            "    float4 _renderTargetBounds;\n"
            "};\n\n";

        std::string fshader = vshader;
        std::string textures =
            "Texture2D __t0 : register(t0); SamplerState __s0 : register(s0);\n"
            "Texture2D __t1 : register(t1); SamplerState __s1 : register(s1);\n"
            "Texture2D __t2 : register(t2); SamplerState __s2 : register(s2);\n"
            "Texture2D __t3 : register(t3); SamplerState __s3 : register(s3);\n"
            "Texture2D __t4 : register(t4); SamplerState __s4 : register(s4);\n"
            "Texture2D __t5 : register(t5); SamplerState __s5 : register(s5);\n"
            "Texture2D __t6 : register(t6); SamplerState __s6 : register(s6);\n"
            "Texture2D __t7 : register(t7); SamplerState __s7 : register(s7);\n"
            "\n";

        std::string vsInput;
        std::string vsOutput = "struct _Out {\n    float4 position : SV_Position;\n";
        std::string fsInput = "struct _In {\n    float4 position : SV_Position;\n";
        std::string fsOutput;
        std::string vsBlock;
        std::string fsBlock;

        std::vector<D3D11_INPUT_ELEMENT_DESC> layout;
        unsigned shaderConstSize = 0;

        bool shaderFixedCreated = false;
        bool shaderConstCreated = false;
        bool shaderInterCreated = false;
        bool shaderVssrcCreated = false;
        bool shaderFssrcCreated = false;

        while (error == false && bool(stream >> arg >> expect<'{'>)) {
            if (arg == "fixed") {
                std::string block = readConstBlock("fixed", shaderFixedCreated);
                vshader += block;
                fshader += block;
            }
            else if (arg == "const") {
                shaderConsts += "cbuffer _Constants : register(b1) {\n";
                shaderConsts += readVarsBlock("const", false, shaderConstCreated, &shaderConstSize);
                shaderConsts += "};\n\n";
            }
            else if (arg == "inout") {
                std::string block = readVarsBlock("inout", true, shaderInterCreated, nullptr);
                vsOutput += block;
                fsInput += block;
            }
            else if (arg == "vssrc") {
                vsInput = "struct _VSIn {\n";

                struct {
                    const char *hlsl;
                    DXGI_FORMAT format;
                } 
                formatTable[] = {
                    {"uint",   DXGI_FORMAT_UNKNOWN},
                    {"float2", DXGI_FORMAT_R16G16_FLOAT},
                    {"float4", DXGI_FORMAT_R16G16B16A16_FLOAT},
                    {"float1", DXGI_FORMAT_R32_FLOAT},
                    {"float2", DXGI_FORMAT_R32G32_FLOAT},
                    {"float3", DXGI_FORMAT_R32G32B32_FLOAT},
                    {"float4", DXGI_FORMAT_R32G32B32A32_FLOAT},
                    {"int2",   DXGI_FORMAT_R16G16_SINT},
                    {"int4",   DXGI_FORMAT_R16G16B16A16_SINT},
                    {"float2", DXGI_FORMAT_R16G16_SNORM},
                    {"float4", DXGI_FORMAT_R16G16B16A16_SNORM},
                    {"uint4",  DXGI_FORMAT_R8G8B8A8_UINT},
                    {"float4", DXGI_FORMAT_R8G8B8A8_UNORM},
                    {"int1",   DXGI_FORMAT_R32_UINT},
                    {"int2",   DXGI_FORMAT_R32G32_UINT},
                    {"int3",   DXGI_FORMAT_R32G32B32_UINT},
                    {"int4",   DXGI_FORMAT_R32G32B32A32_UINT}
                };

                std::size_t index = 0;

                for (const auto &item : vertex) {
                    if (item.format == RenderingShaderInputFormat::VERTEX_ID) {
                        vsInput += std::string("    uint ") + item.name + " : SV_VertexID;\n";
                    }
                    else {
                        vsInput += indent + formatTable[unsigned(item.format)].hlsl + std::string(" ") + item.name + " : VTX" + std::to_string(index) + ";\n";

                        unsigned align = index == 0 ? 0 : D3D11_APPEND_ALIGNED_ELEMENT;
                        layout.emplace_back(D3D11_INPUT_ELEMENT_DESC {"VTX", unsigned(index), formatTable[unsigned(item.format)].format, 0, align, D3D11_INPUT_PER_VERTEX_DATA, 0});
                    }

                    index++;
                }

                index = 0;

                for (const auto &item : instance) {
                    if (item.format != RenderingShaderInputFormat::VERTEX_ID) {
                        vsInput += indent + formatTable[unsigned(item.format)].hlsl + std::string(" ") + item.name + " : INST" + std::to_string(index) + ";\n";
                        layout.emplace_back(D3D11_INPUT_ELEMENT_DESC{ "INST", unsigned(index), formatTable[unsigned(item.format)].format, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 });
                    }

                    index++;
                }

                vsInput += "};\n\n";
                vsOutput += "};\n\n";
                vsBlock = readCodeBlock("fssrc", shaderVssrcCreated);

                if (vsBlock.empty() == false) {
                    vsBlock = "_Out main(_VSIn input) {\n    _Out output;\n" + vsBlock + "return output;\n}\n";
                    vshader += shaderConsts + vsInput + vsOutput + vsBlock;
                }
            }
            else if (arg == "fssrc") {
                fsInput += "};\n\n";
                fsBlock = readCodeBlock("fssrc", shaderFssrcCreated);

                if (fsBlock.empty() == false) {
                    fsBlock = textures + "struct _PSOut {\n    float4 color : SV_Target;\n};\n\n_PSOut main(_In input) {\n    _PSOut output;\n" + fsBlock + "return output;\n}\n";
                    fshader += shaderConsts + fsInput + fsOutput + fsBlock;
                }
            }
            else {
                _platform->logError("[RENDER shader '%s'] Undefined block", name);
                break;
            }
        }

        vshader = makeLines(vshader);
        fshader = makeLines(fshader);

        HRESULT hresult;

        ComPtr<ID3DBlob> vshaderBinary;
        ComPtr<ID3DBlob> pshaderBinary;
        ComPtr<ID3DBlob> errorBlob;

        ComPtr<ID3D11Buffer> constBuffer;

        D3D11_BUFFER_DESC bdsc = {};
        bdsc.Usage = D3D11_USAGE_DEFAULT;
        bdsc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bdsc.CPUAccessFlags = 0;
        bdsc.MiscFlags = 0;
        bdsc.StructureByteStride = 0;

        if (shaderConstSize) {
            bdsc.ByteWidth = shaderConstSize;

            if ((hresult = _device->CreateBuffer(&bdsc, nullptr, &constBuffer)) != S_OK) {
                _platform->logError("[RENDER shader '%s'] Per-apply const buffer creation failed with result = %p", name, hresult);
            }
        }

        ComPtr<ID3D11InputLayout> inputLayout;

        if (layout.empty() == false) {
            if ((hresult = _device->CreateInputLayout(&layout[0], unsigned(layout.size()), vshaderBinary->GetBufferPointer(), vshaderBinary->GetBufferSize(), &inputLayout)) != S_OK) {
                _platform->logError("[RENDER shader '%s'] Input layout creation failed with result = %p", name, hresult);
            }
        }


        ComPtr<ID3D11VertexShader> vertexShader;
        ComPtr<ID3D11PixelShader> pixelShader;

        std::shared_ptr<RenderingShader> result;

        if (D3DCompile(vshader.c_str(), vshader.length(), "vs", nullptr, nullptr, "main", "vs_4_0", D3DCOMPILE_DEBUG, 0, &vshaderBinary, &errorBlob) == S_OK) {
            if (D3DCompile(fshader.c_str(), fshader.length(), "ps", nullptr, nullptr, "main", "ps_4_0", D3DCOMPILE_DEBUG, 0, &pshaderBinary, &errorBlob) == S_OK) {
                if ((hresult = _device->CreateVertexShader(vshaderBinary->GetBufferPointer(), vshaderBinary->GetBufferSize(), nullptr, &vertexShader)) == S_OK) {
                    if ((hresult = _device->CreatePixelShader(pshaderBinary->GetBufferPointer(), pshaderBinary->GetBufferSize(), nullptr, &pixelShader)) == S_OK) {
                        result = std::make_shared<Direct3D11Shader>(inputLayout, vertexShader, pixelShader, constBuffer, shaderConstSize);
                    }
                    else {
                        _platform->logError("[RENDER shader '%s'] Pixel shader creation failed with result = %p", name, hresult);
                    }
                }
                else {
                    _platform->logError("[RENDER shader '%s'] Vertex shader creation failed with result = %p", name, hresult);
                }
            }
            else {
                _platform->logError("[RENDER shader '%s'] Fragment shader compilation error:\n%s\n%s\n", name, (const char *)errorBlob->GetBufferPointer(), fshader.data());
            }
        }
        else {
            _platform->logError("[RENDER shader '%s'] Vertex shader compilation error:\n%s\n%s\n", name, (const char *)errorBlob->GetBufferPointer(), vshader.data());
        }

        return result;
    }

    std::shared_ptr<RenderingTexture2D> Direct3D11Rendering::createTexture(RenderingTextureFormat format, std::uint32_t width, std::uint32_t height, const std::initializer_list<const std::uint8_t *> &mipsData) {
        return {};
    }

    std::shared_ptr<RenderingStructuredData> Direct3D11Rendering::createData(const void *data, std::uint32_t count, std::uint32_t stride) {
        return {};
    }

    void Direct3D11Rendering::applyShader(const std::shared_ptr<RenderingShader> &shader, const void *constants) {
        std::static_pointer_cast<Direct3D11Shader>(shader)->apply(_context, constants);

        _context->VSSetConstantBuffers(0, 1, _frameConstantsBuffer.GetAddressOf());
        _context->PSSetConstantBuffers(0, 1, _frameConstantsBuffer.GetAddressOf());
    }

    void Direct3D11Rendering::applyTextures(const std::initializer_list<const RenderingTexture2D *> &textures) {}

    void Direct3D11Rendering::drawGeometry(std::uint32_t vertexCount, RenderingTopology topology) {
        ID3D11Buffer *tmpBuffer = nullptr;
        std::uint32_t tmpStride = 0;
        std::uint32_t tmpOffset = 0;

        _context->IASetPrimitiveTopology(g_topologies[unsigned(topology)]);
        _context->IASetVertexBuffers(0, 1, &tmpBuffer, &tmpStride, &tmpOffset);
        _context->Draw(unsigned(vertexCount), 0); //
    }

    void Direct3D11Rendering::drawGeometry(const std::shared_ptr<RenderingStructuredData> &vertexData, const std::shared_ptr<RenderingStructuredData> &instanceData, std::uint32_t vertexCount, std::uint32_t instanceCount, RenderingTopology topology) {}

    void Direct3D11Rendering::prepareFrame() {
        if (_context) {
            float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };

            _context->OMSetRenderTargets(1, _defaultRTView.GetAddressOf(), _defaultDepthView.Get());
            _context->ClearRenderTargetView(_defaultRTView.Get(), clearColor);
            _context->ClearDepthStencilView(_defaultDepthView.Get(), D3D11_CLEAR_DEPTH, 0.0f, 0);
        }
    }

    void Direct3D11Rendering::presentFrame(float dt) {
        if (_swapChain) {
            _swapChain->Present(1, 0);
        }
    }

    void Direct3D11Rendering::getFrameBufferData(std::uint8_t *imgFrame) {}
}

namespace foundation {
    std::shared_ptr<RenderingInterface> RenderingInterface::instance(std::shared_ptr<PlatformInterface> &platform) {
        return std::make_shared<Direct3D11Rendering>(platform);
    }
}

#endif // PLATFORM_WINDOWS
