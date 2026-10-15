
#include "rendering.h"

#ifdef PLATFORM_WINDOWS

#include "rendering_dx11.h"

namespace {
    std::weak_ptr<foundation::RenderingInterface> g_instance;
}

namespace foundation {
    Direct3D11Rendering::Direct3D11Rendering(const std::shared_ptr<PlatformInterface> &platform) : _platform(platform) {}
    Direct3D11Rendering::~Direct3D11Rendering() {}

    void Direct3D11Rendering::updateFrameConstants(const math::transform3f &vp, const math::transform3f &svp, const math::transform3f &ivp, const math::vector3f &camPos, const math::vector3f &camDir) {
        _stdVPMatrix = svp;
    }

    auto Direct3D11Rendering::createShader(const char *src, const InputLayout &layout) -> RenderShaderPtr {
        return nullptr;
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

    void Direct3D11Rendering::presentFrame() {}
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
