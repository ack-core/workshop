
#include "interfaces.h"
#include "d3d11_render.h"

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

#include <vector>
#include <strstream>
#include <iomanip>
#include <cctype>
#include <string>
#include <algorithm>

namespace native {

    struct RenderShader::Data {
        std::unique_ptr<ComPtr<ID3D11Buffer>[]> constants;
        std::size_t constantCount = 0;
        ComPtr<ID3D11InputLayout> layout;
        ComPtr<ID3D11VertexShader> vshader;
        ComPtr<ID3D11PixelShader> pshader;
    };

    struct RenderTexture::Data {
        ComPtr<ID3D11Texture2D>  _texture;
        ComPtr<ID3D11ShaderResourceView> _shaderResourceView;
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t mipCount;
    };

    struct RenderGeometry::Data {
        ComPtr<ID3D11Buffer> buffer;
        std::uint32_t stride;
        std::uint32_t count;
    };

    namespace {
        constexpr unsigned SL_CONST_MAX = 8;
        constexpr unsigned SL_TXC_MAX = 8;

        // TODO: separate layout formats and src/const formats

        struct {
            const char *hlsl;
            DXGI_FORMAT format;
        } slFormatTable[] = {
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

        struct {
            const char *hlsl;
            std::size_t size;
        } slSizeTable[] = {
            {"float1", 4},
            {"float2", 8},
            {"float3", 12},
            {"float4", 16},
            {"int1",   4},
            {"int2",   8},
            {"int3",   12},
            {"int4",   16},
            {"matrix", 64},
        };

        const char *slTexCoord[SL_TXC_MAX] = {
            "TEXCOORD0", "TEXCOORD1", "TEXCOORD2", "TEXCOORD3", "TEXCOORD4", "TEXCOORD5", "TEXCOORD6", "TEXCOORD7"
        };

        const std::string slFrameConstBuffer =
            "cbuffer FrameData : register(b0) {\n"
            "matrix _VP;\n"
            "float3 _CamPos;\n"
            "float _R0;\n"
            "float3 _CamDir;\n"
            "float _R1;\n"
            "};\n\n";

        const std::string slStdFunctions =
            "#define _sign(a) sign(a)\n"
            "#define _mul(a, b) mul(a, b)\n"
            "#define _dot(a, b) dot(a, b)\n"
            "#define _norm(a) normalize(a)\n"
            "#define _lerp(a, b, k) lerp(a, b, k)\n"
            "#define _tex2D(a) __t0.Sample(__s0, a)\n";

        std::size_t slGetTypeSize(const std::string &varname, const std::string &format) {
            int  multiply = 1;
            auto braceStart = varname.find('[');
            auto braceEnd = varname.rfind(']');

            if (braceStart != std::string::npos && braceEnd != std::string::npos) {
                multiply = std::max(std::stoi(varname.substr(braceStart + 1, braceEnd - braceStart - 1)), multiply);
            }

            for (const auto &fmt : slSizeTable) {
                if (fmt.hlsl == format) {
                    return fmt.size * multiply;
                }
            }

            return 0;
        }

        bool slReadShaderBlock(std::string &dest, std::istrstream &stream) {
            char ch[2] = {};
            bool isline = true;

            while ((ch[0] = stream.get()) != '}') {
                if (stream.good()) {
                    if (ch[0] == '\n') {
                        stream >> std::ws;
                    }

                    dest += ch;
                }
                else return false;
            }

            return true;
        }

        DXGI_FORMAT txNativeTextureFormat[] = {DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8_UNORM};

        unsigned txGetTexture2DPitch(RenderTexture::Format fmt, std::uint32_t width) {
            switch (fmt) {
                case RenderTexture::Format::RGBA8UN:
                    return (width * 32 + 7) / 8;
                case RenderTexture::Format::R8UN:
                    return width;
            }

            return 0;
        }

        template <typename = void> std::istream &expect(std::istream &stream) {
            return stream;
        }
        template <char Ch, char... Chs> std::istream &expect(std::istream &stream) {
            (stream >> std::ws).peek() == Ch ? stream.ignore() : stream.setstate(std::ios_base::failbit);
            return expect<Chs...>(stream);
        }

        template <typename T, std::size_t SD, std::size_t SS> void copy(T (&dst)[SD], const T(&src)[SS]) {
            static_assert(SD >= SS, "Destination less than src");
            memcpy(dst, src, SS * sizeof(T));
        }
    }

    std::shared_ptr<RenderInterface> RenderInterface::makeInstance(const std::shared_ptr<PlatformInterface> &platform) {
        return std::make_shared<UWDirect3D11Render>(platform);
    }

    RenderShader::RenderShader() : _data(nullptr) {}
    RenderShader::RenderShader(Data *data) : _data(data) {}
    RenderShader::~RenderShader() {
        if (_data) {
            delete _data;
        }
    }
    
    RenderShader::RenderShader(RenderShader &&other) {
        _data = other._data;
        other._data = nullptr;
    }

    RenderShader& RenderShader::operator =(RenderShader &&other) {
        if (_data) {
            delete _data;
        }

        _data = other._data;
        other._data = nullptr;
        return *this;
    }

    RenderGeometry::RenderGeometry() : _data(nullptr) {}
    RenderGeometry::RenderGeometry(Data *data) : _data(data) {}
    RenderGeometry::~RenderGeometry() {
        if (_data) {
            delete _data;
        }
    }

    RenderGeometry::RenderGeometry(RenderGeometry &&other) {
        _data = other._data;
        other._data = nullptr;
    }

    RenderGeometry& RenderGeometry::operator =(RenderGeometry &&other) {
        if (_data) {
            delete _data;
        }

        _data = other._data;
        other._data = nullptr;
        return *this;
    }

    std::uint32_t RenderGeometry::getCount() const {
        return _data ? _data->count : 0;
    }

    std::uint32_t RenderGeometry::getStride() const {
        return _data ? _data->stride : 0;
    }

    RenderTexture::RenderTexture() : _data(nullptr) {}
    RenderTexture::RenderTexture(Data *data) : _data(data) {}
    RenderTexture::~RenderTexture() {
        if (_data) {
            delete _data;
        }
    }

    RenderTexture::RenderTexture(RenderTexture &&other) {
        _data = other._data;
        other._data = nullptr;
    }

    RenderTexture& RenderTexture::operator =(RenderTexture &&other) {
        if (_data) {
            delete _data;
        }

        _data = other._data;
        other._data = nullptr;
        return *this;
    }

    std::uint32_t RenderTexture::getWidth() const {
        return _data ? _data->width : 0;
    }

    std::uint32_t RenderTexture::getHeight() const {
        return _data ? _data->height : 0;
    }
    
    std::uint32_t RenderTexture::getMipCount() const {
        return _data ? _data->mipCount : 0;
    }

    UWDirect3D11Render::UWDirect3D11Render(const std::shared_ptr<PlatformInterface> &platform) : _platform(platform) {
        unsigned flags = D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        D3D_FEATURE_LEVEL features[] = {
            D3D_FEATURE_LEVEL_11_0,
        };
        
        // device & context
        D3D_FEATURE_LEVEL featureLevel;
        ComPtr<ID3D11DeviceContext> tmpContext;
        ComPtr<ID3D11Device> tmpDevice;

        D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, features, 1, D3D11_SDK_VERSION, &tmpDevice, &featureLevel, &tmpContext);

        tmpDevice.As<ID3D11Device1>(&_device);
        tmpContext.As<ID3D11DeviceContext1>(&_context);
    }

    UWDirect3D11Render::~UWDirect3D11Render() {

    }

    void UWDirect3D11Render::updateCameraTransform(const float(&camPos)[3], const float(&camDir)[3], const float(&camVP)[16]) {
        copy(_frameConstantsData.vp, camVP);
        copy(_frameConstantsData.camPos, camPos);
        copy(_frameConstantsData.camDir, camDir);

        _context->UpdateSubresource(_frameConstantsBuffer.Get(), 0, nullptr, &_frameConstantsData, 0, 0);
        _context->VSSetConstantBuffers(0, 1, _frameConstantsBuffer.GetAddressOf());
        _context->PSSetConstantBuffers(0, 1, _frameConstantsBuffer.GetAddressOf());
    }

    RenderShader UWDirect3D11Render::createShader(const std::initializer_list<RenderShaderInput> &input, const char *shadersrc, const char *name) {
        RenderShader::Data *data = new RenderShader::Data();
        RenderShader result {data};

        unsigned counter = 0;

        std::string varname, arg;
        std::string vshader = slStdFunctions + slFrameConstBuffer + "cbuffer ConstData0 : register(b1) {\n";
        std::string fshader = slStdFunctions + slFrameConstBuffer + "cbuffer ConstData0 : register(b1) {\n";
        std::istrstream stream(shadersrc, std::strlen(shadersrc));
        std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayout;
        std::vector<std::size_t> constBufferSizes;
        
        while (stream >> arg >> expect<'{'>) {
            if (arg == "const") {
                std::size_t cbufferSize = 0;

                if (counter >= SL_CONST_MAX) {
                    _platform->logError("shader '%s': too many constant blocks", name);
                    break;
                }
                if (counter > 0) {
                    std::string index = std::to_string(counter + 1);
                    vshader += "};\ncbuffer ConstData" + index + " : register(b" + index + ") {\n";
                    fshader += "};\ncbuffer ConstData" + index + " : register(b" + index + ") {\n";
                }

                while (stream >> varname && varname[0] != '}') {
                    if (stream >> expect<':'> >> arg) {
                        cbufferSize += slGetTypeSize(varname, arg);

                        vshader += arg + " " + varname + ";\n";
                        fshader += arg + " " + varname + ";\n";
                    }
                    else {
                        _platform->logError("shader '%s' constant block: syntax error", name);
                        break;
                    }
                }

                constBufferSizes.emplace_back(cbufferSize);
                counter++;
            }
            else if (arg == "inter") {
                vshader += "};\nstruct VSInput {\n";
                fshader += "};\nTexture2D __t0 : register(t0);\nSamplerState __s0 : register(s0);\nstruct PSInput {\nfloat4 position : SV_Position;\n";
                
                for (std::size_t i = 0, cnt = input.size(); i < cnt; i++) {
                    const RenderShaderInput &current = *(input.begin() + i);

                    if (current.format == RenderShaderInput::Format::VERTEX_ID) {
                        vshader += std::string("uint ") + current.name + " : SV_VertexID;\n";
                    }
                    else {
                        vshader += slFormatTable[unsigned(current.format)].hlsl + std::string(" ") + current.name + " : VTX" + std::to_string(i) + ";\n";

                        unsigned align = i == 0 ? 0 : D3D11_APPEND_ALIGNED_ELEMENT;
                        unsigned instStepRate = current.perInstance ? 1 : 0;
                        D3D11_INPUT_CLASSIFICATION cls = current.perInstance ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;

                        inputLayout.emplace_back(D3D11_INPUT_ELEMENT_DESC {
                            "VTX", unsigned(i), slFormatTable[unsigned(current.format)].format, 0, align, cls, instStepRate
                        });
                    }
                }
                
                vshader += "};\nstruct VSOutput {\nfloat4 position : SV_Position;\n";
                counter = 0;

                while (stream >> varname && varname[0] != '}') {
                    if (counter > SL_TXC_MAX) {
                        _platform->logError("shader '%s' inter: too many elements", name);
                        break;
                    }

                    if (stream >> expect<':'> >> arg) {
                        vshader += arg + " " + varname + " : " + slTexCoord[counter] + ";\n";
                        fshader += arg + " " + varname + " : " + slTexCoord[counter] + ";\n";
                        counter++;
                    }
                    else {
                        _platform->logError("shader '%s' inter: syntax error", name);
                        break;
                    }
                }

                break;
            }
            else {
                _platform->logError("shader '%s' undefined block: 'const' or 'inter' expected", name);
                return result;
            }
        }

        vshader += "};\nVSOutput main(VSInput input) { VSOutput output;";
        fshader += "};\nstruct PSOutput {\nfloat4 color : SV_Target;\n};\nPSOutput main(PSInput input) { PSOutput output;";

        if (bool(stream >> expect<'v', 's', 's', 'r', 'c'> >> expect<'{'>) == false || slReadShaderBlock(vshader, stream) == false) {
            _platform->logError("shader '%s' vssrc: block not found", name);
            return result;
        }
        if (bool(stream >> expect<'f', 's', 's', 'r', 'c'> >> expect<'{'>) == false || slReadShaderBlock(fshader, stream) == false) {
            _platform->logError("shader '%s' fssrc: block not found", name);
            return result;
        }

        vshader += "return output;\n}";
        fshader += "return output;\n}";

        ComPtr<ID3DBlob> vshaderBinary;
        ComPtr<ID3DBlob> fshaderBinary;

        if (_compileShader(vshader, "vssrc", "vs_4_0", vshaderBinary) && _compileShader(fshader, "fssrc", "ps_4_0", fshaderBinary)) {
            if (inputLayout.empty() == false) {
                _device->CreateInputLayout(&inputLayout[0], unsigned(inputLayout.size()), vshaderBinary->GetBufferPointer(), vshaderBinary->GetBufferSize(), &data->layout);
            }

            _device->CreateVertexShader(vshaderBinary->GetBufferPointer(), vshaderBinary->GetBufferSize(), nullptr, &data->vshader);
            _device->CreatePixelShader(fshaderBinary->GetBufferPointer(), fshaderBinary->GetBufferSize(), nullptr, &data->pshader);

            data->constantCount = constBufferSizes.size();
            data->constants = std::make_unique<ComPtr<ID3D11Buffer>[]>(data->constantCount);

            for (std::size_t i = 0; i < constBufferSizes.size(); i++) {
                D3D11_BUFFER_DESC dsc {unsigned(constBufferSizes[i]), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
                _device->CreateBuffer(&dsc, nullptr, data->constants[i].GetAddressOf());
            }
        }

        return result;
    }

    RenderTexture UWDirect3D11Render::createTexture(RenderTexture::Format format, std::uint32_t width, std::uint32_t height, std::uint32_t mipCount, const std::uint8_t *const *imgMipsBinaryData) {
        RenderTexture::Data *data = new RenderTexture::Data();

        data->width = width;
        data->height = height;
        data->mipCount = mipCount;
        
        D3D11_TEXTURE2D_DESC      texDesc = {0};
        D3D11_SUBRESOURCE_DATA    subResData[64] = {0};
        D3D11_SUBRESOURCE_DATA    *subResDataPtr = nullptr;

        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.Format = txNativeTextureFormat[std::size_t(format)];
        texDesc.Usage = imgMipsBinaryData ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;
        texDesc.MipLevels = mipCount;
        texDesc.ArraySize = 1;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        if (imgMipsBinaryData) {
            for (std::uint32_t i = 0; i < mipCount; i++) {
                subResData[i].pSysMem = imgMipsBinaryData[i];
                subResData[i].SysMemPitch = txGetTexture2DPitch(format, width >> i);
                subResData[i].SysMemSlicePitch = 0;
            }
        
            subResDataPtr = subResData;
        }

        if (_device->CreateTexture2D(&texDesc, subResDataPtr, data->_texture.GetAddressOf()) == S_OK) {
            D3D11_SHADER_RESOURCE_VIEW_DESC texViewDesc = {texDesc.Format};
            texViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            texViewDesc.Texture2D.MipLevels = texDesc.MipLevels;
            texViewDesc.Texture2D.MostDetailedMip = 0;

            _device->CreateShaderResourceView(data->_texture.Get(), &texViewDesc, data->_shaderResourceView.GetAddressOf());
        }

        return {data};
    }

    RenderGeometry UWDirect3D11Render::createGeometry(const void *data, std::uint32_t count, std::uint32_t stride) {
        RenderGeometry::Data *resultData = new RenderGeometry::Data();

        D3D11_BUFFER_DESC dsc {count * stride, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0};
        D3D11_SUBRESOURCE_DATA resdata {data, 0, 0};
        _device->CreateBuffer(&dsc, &resdata, &resultData->buffer);
        resultData->stride = stride;
        resultData->count = count;

        return {resultData};
    }

    void UWDirect3D11Render::applyShader(const RenderShader &shader, const std::initializer_list<const void *> &constants) {
        if (shader._data && shader._data->vshader && shader._data->pshader) {
            _context->IASetInputLayout(shader._data->layout.Get());
            _context->VSSetShader(shader._data->vshader.Get(), nullptr, 0);
            _context->PSSetShader(shader._data->pshader.Get(), nullptr, 0);

            for (std::size_t i = 0, cnt = std::min(shader._data->constantCount, constants.size()); i < cnt; i++) {
                const void *current = *(constants.begin() + i);
                
                if (current) {
                    _context->UpdateSubresource(shader._data->constants[i].Get(), 0, nullptr, current, 0, 0);
                }
            }

            for (std::size_t i = 0; i < shader._data->constantCount; i++) {
                _context->VSSetConstantBuffers(unsigned(i + 1), 1, shader._data->constants[i].GetAddressOf());
                _context->PSSetConstantBuffers(unsigned(i + 1), 1, shader._data->constants[i].GetAddressOf());
            }
        }
    }

    void UWDirect3D11Render::applyShaderConstants(const RenderShader &shader, const std::initializer_list<const void *> &constants) {
        if (shader._data) {
            for (std::size_t i = 0, cnt = std::min(shader._data->constantCount, constants.size()); i < cnt; i++) {
                const void *current = *(constants.begin() + i);

                if (current) {
                    _context->UpdateSubresource(shader._data->constants[i].Get(), 0, nullptr, current, 0, 0);
                }
            }

            for (std::size_t i = 0; i < shader._data->constantCount; i++) {
                _context->VSSetConstantBuffers(unsigned(i + 1), 1, shader._data->constants[i].GetAddressOf());
                _context->PSSetConstantBuffers(unsigned(i + 1), 1, shader._data->constants[i].GetAddressOf());
            }
        }      
    }

    void UWDirect3D11Render::applyTextures(const std::initializer_list<const RenderTexture *> &textures) {
        ID3D11ShaderResourceView *tmpShaderResViews[64];

        for (std::size_t i = 0; i < textures.size(); i++) {
            tmpShaderResViews[i] = nullptr;

            if (auto *current = *(textures.begin() + i)) {
                if (RenderTexture::Data *imp = current->_data) {
                    tmpShaderResViews[i] = imp->_shaderResourceView.Get();
                }
            }
        }

        _context->PSSetShaderResources(0, std::uint32_t(textures.size()), tmpShaderResViews);
    }

    void UWDirect3D11Render::drawGeometry(const RenderGeometry *geometry, std::uint32_t vertexCount, std::uint32_t instanceCount, RenderGeometry::Topology topology) {
        ID3D11Buffer *tmpBuffer = nullptr;
        std::uint32_t tmpStride = 0;
        std::uint32_t tmpOffset = 0;
        
        if (geometry) {
            if (const RenderGeometry::Data *imp = geometry->_data) {
                tmpBuffer = imp->buffer.Get();
                tmpStride = imp->stride;
            }
        }

        static D3D_PRIMITIVE_TOPOLOGY topologies[] = {
            D3D_PRIMITIVE_TOPOLOGY_LINELIST,
            D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
        };

        _context->IASetPrimitiveTopology(topologies[unsigned(topology)]);
        _context->IASetVertexBuffers(0, 1, &tmpBuffer, &tmpStride, &tmpOffset);
        _context->DrawInstanced(unsigned(vertexCount), unsigned(instanceCount), 0, 0); //
    }

    void UWDirect3D11Render::prepareFrame() {
        if (_swapChain == nullptr) {
            _initialize();
        }

        float clearColor[] = {0.2f, 0.2f, 0.2f, 1.0f};
        _context->OMSetRenderTargets(1, _defaultRTView.GetAddressOf(), _defaultDepthView.Get());
        _context->ClearRenderTargetView(_defaultRTView.Get(), clearColor);
        _context->ClearDepthStencilView(_defaultDepthView.Get(), D3D11_CLEAR_DEPTH, 0.0f, 0);
    }

    void UWDirect3D11Render::presentFrame(float dt) {
        _swapChain->Present(1, 0);
    }

    //-------------------------------------------------------------------------

    void UWDirect3D11Render::_initialize() {
        unsigned width = unsigned(_platform->getNativeScreenWidth());
        unsigned height = unsigned(_platform->getNativeScreenHeight());

        _swapChain.Attach(reinterpret_cast<IDXGISwapChain1 *>(_platform->setNativeRenderingContext(_device.Get())));

        { // default RT
            ComPtr<ID3D11Texture2D> defRTTexture;
            _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(defRTTexture.GetAddressOf()));

            D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {DXGI_FORMAT_B8G8R8A8_UNORM};
            renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            renderTargetViewDesc.Texture2D.MipSlice = 0;

            _device->CreateRenderTargetView(defRTTexture.Get(), &renderTargetViewDesc, &_defaultRTView);

            D3D11_VIEWPORT viewPort;
            viewPort.TopLeftX = 0;
            viewPort.TopLeftY = 0;
            viewPort.Width = float(width);
            viewPort.Height = float(height);
            viewPort.MinDepth = 0.0f;
            viewPort.MaxDepth = 1.0f;

            _context->RSSetViewports(1, &viewPort);
        }

        { // default Depth
            ComPtr<ID3D11Texture2D> defDepthTexture;
            DXGI_FORMAT  depthTexFormat = DXGI_FORMAT_R32_TYPELESS; //DXGI_FORMAT_R32_TYPELESS; dx10
            DXGI_FORMAT  depthFormat = DXGI_FORMAT_D32_FLOAT; //DXGI_FORMAT_D32_FLOAT;

            D3D11_TEXTURE2D_DESC depthTexDesc = {0};
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

            if (_device->CreateTexture2D(&depthTexDesc, 0, &defDepthTexture) == S_OK) {
                D3D11_DEPTH_STENCIL_VIEW_DESC depthDesc = {depthFormat};
                depthDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                depthDesc.Texture2D.MipSlice = 0;

                D3D11_SHADER_RESOURCE_VIEW_DESC depthShaderViewDesc = {DXGI_FORMAT_R32_FLOAT};
                depthShaderViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                depthShaderViewDesc.Texture2D.MipLevels = 1; //!
                depthShaderViewDesc.Texture2D.MostDetailedMip = 0;

                _device->CreateDepthStencilView(defDepthTexture.Get(), &depthDesc, &_defaultDepthView);
                _device->CreateShaderResourceView(defDepthTexture.Get(), &depthShaderViewDesc, &_defaultDepthShaderResourceView);
            }
        }

        // raster state
        D3D11_RASTERIZER_DESC rasterDsc;

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

        _device->CreateRasterizerState(&rasterDsc, _defaultRasterState.GetAddressOf());
        _context->RSSetState(_defaultRasterState.Get());

        // blend state
        D3D11_BLEND_DESC blendDsc;
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

        _device->CreateBlendState(&blendDsc, _defaultBlendState.GetAddressOf());
        _context->OMSetBlendState(_defaultBlendState.Get(), nullptr, 0xffffffff);

        // depth state
        D3D11_DEPTH_STENCIL_DESC ddesc;

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

        _device->CreateDepthStencilState(&ddesc, &_defaultDepthState);
        _context->OMSetDepthStencilState(_defaultDepthState.Get(), 0);

        // frame shader constants
        D3D11_BUFFER_DESC bdsc {sizeof(FrameConstants), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
        _device->CreateBuffer(&bdsc, nullptr, _frameConstantsBuffer.GetAddressOf());

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

        _device->CreateSamplerState(&sdesc, &_defaultSamplerState);
        _context->PSSetSamplers(0, 1, _defaultSamplerState.GetAddressOf());
    }

    bool UWDirect3D11Render::_compileShader(const std::string &shader, const char *name, const char *target, ComPtr<ID3DBlob> &out) {
        ComPtr<ID3DBlob> errorBlob;

        if (D3DCompile(shader.c_str(), shader.length(), name, nullptr, nullptr, "main", target, D3DCOMPILE_DEBUG, 0, &out, &errorBlob) == S_OK) {
            return true;
        }
        else {
            std::ostrstream error;
            error << "Shader compilation errors\n\n";
            
            std::istrstream vsstream(shader.data(), shader.length());
            std::string line;
            unsigned counter = 0;

            while (std::getline(vsstream, line)) {
                error << std::setw(3) << ++counter << "  " << line << std::endl;
            }
            error << std::endl;
            std::string errmsg = (const char *)errorBlob->GetBufferPointer();
            errmsg.erase(0, errmsg.find(name));
            error << errmsg << std::endl << '\0';

            _platform->logError(error.str());
        }

        return false;
    }

    void UWDirect3D11Render::_drawQuad() {
        static RenderShader _shader = createShader({
            {"id", RenderShaderInput::Format::VERTEX_ID},
        },
        R"(
            inter {}
            vssrc {
                float2 vcoord = 1.8f * float2(input.id >> 1, input.id & 0x1) - 0.9f;
                output.position = float4(vcoord.x, vcoord.y, 1.0, 1.0);
            }
            fssrc {
                output.color = float4(1.0, 1.0, 1.0, 1.0);
            }        
        )", "screen quad");

        applyShader(_shader);

        _context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);        
        _context->Draw(4, 0);
    }
}
