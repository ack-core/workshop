
#include <cctype>
#include <string>
#include <vector>
#include <strstream>
#include <iomanip>
#include <algorithm>

#include "interfaces.h"

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

}

namespace foundation {
    Direct3D11Shader::~Direct3D11Shader() {}

    Direct3D11Texture2D::~Direct3D11Texture2D() {}

    std::uint32_t Direct3D11Texture2D::getWidth() const {}
    std::uint32_t Direct3D11Texture2D::getHeight() const {}
    std::uint32_t Direct3D11Texture2D::getMipCount() const {}
    RenderingTextureFormat Direct3D11Texture2D::getFormat() const {}

    Direct3D11StructuredData::~Direct3D11StructuredData() {}

    std::uint32_t Direct3D11StructuredData::getCount() const {}
    std::uint32_t Direct3D11StructuredData::getStride() const {}

    Direct3D11Rendering::Direct3D11Rendering(const std::shared_ptr<PlatformInterface> &platform) {}
    Direct3D11Rendering::~Direct3D11Rendering() {}

    void Direct3D11Rendering::updateCameraTransform(const float(&camPos)[3], const float(&camDir)[3], const float(&camVP)[16]) {}

    std::shared_ptr<RenderingShader> Direct3D11Rendering::createShader(const char *shadersrc, const std::initializer_list<RenderingShader::Input> &vertex, const std::initializer_list<RenderingShader::Input> &instance, const void *prmnt) {
    
    }

    std::shared_ptr<RenderingTexture2D> Direct3D11Rendering::createTexture(RenderingTextureFormat format, std::uint32_t width, std::uint32_t height, const std::initializer_list<const std::uint8_t *> &mipsData) {
    
    }

    std::shared_ptr<RenderingStructuredData> Direct3D11Rendering::createData(const void *data, std::uint32_t count, std::uint32_t stride) {
    
    }

    void Direct3D11Rendering::applyShader(const std::shared_ptr<RenderingShader> &shader, const void *constants) {}
    void Direct3D11Rendering::applyTextures(const std::initializer_list<const RenderingTexture2D *> &textures) {}

    void Direct3D11Rendering::drawGeometry(std::uint32_t vertexCount, RenderingTopology topology) {}
    void Direct3D11Rendering::drawGeometry(const std::shared_ptr<RenderingStructuredData> &vertexData, const std::shared_ptr<RenderingStructuredData> &instanceData, std::uint32_t vertexCount, std::uint32_t instanceCount, RenderingTopology topology) {}

    void Direct3D11Rendering::prepareFrame() {}
    void Direct3D11Rendering::presentFrame(float dt) {}

    void Direct3D11Rendering::getFrameBufferData(std::uint8_t *imgFrame) {}
}

namespace foundation {
    std::shared_ptr<RenderingInterface> instance(const std::shared_ptr<PlatformInterface> &platform) {
        return std::make_shared<Direct3D11Rendering>(platform);
    }
}

#endif // PLATFORM_WINDOWS
