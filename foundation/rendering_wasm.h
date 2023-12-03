
#include "rendering.h"
#include <unordered_set>

namespace foundation {
    class WASMShader : public RenderShader {
    public:
        WASMShader(const std::string &name, std::uint32_t constBufferLength);
        ~WASMShader() override;
        
        std::uint32_t getConstBufferLength() const;
        const std::string &getName() const;
        
    private:
        std::string _name;
        std::uint32_t _constBufferLength;
    };
    
    class WASMRendering final : public RenderingInterface {
    public:
        WASMRendering(const std::shared_ptr<PlatformInterface> &platform);
        ~WASMRendering() override;
        
        void updateFrameConstants(const math::transform3f &view, const math::transform3f &proj, const math::vector3f &camPos, const math::vector3f &camDir) override;
        
        auto getScreenCoordinates(const math::vector3f &worldPosition) -> math::vector2f override;
        auto getWorldDirection(const math::vector2f &screenPosition) -> math::vector3f override;
        
        RenderShaderPtr createShader(const char *name, const char *src, const RenderShaderInputDesc &vtx, const RenderShaderInputDesc &itc) override;
        RenderTexturePtr createTexture(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, const std::initializer_list<const void *> &mipsData) override;
        RenderTargetPtr createRenderTarget(RenderTextureFormat format, unsigned textureCount, std::uint32_t w, std::uint32_t h, bool withZBuffer) override;
        RenderDataPtr createData(const void *data, std::uint32_t count, std::uint32_t stride) override;
        
        float getBackBufferWidth() const override;
        float getBackBufferHeight() const override;
        
        void applyState(const RenderShaderPtr &shader, const RenderPassConfig &cfg) override;
        void applyShaderConstants(const void *constants) override;
        void applyTextures(const RenderTexturePtr *textures, std::uint32_t count) override;
        
        void drawGeometry(const RenderDataPtr &vertexData, std::uint32_t vcount, RenderTopology topology) override;
        void drawGeometry(const RenderDataPtr &vertexData, const RenderDataPtr &indexData, std::uint32_t indexCount, RenderTopology topology) override;
        void drawGeometry(const RenderDataPtr &vertexData, const RenderDataPtr &instanceData, std::uint32_t vcount, std::uint32_t icount, RenderTopology topology) override;

        void presentFrame() override;
        
    private:
        struct FrameConstants {
            math::transform3f viewMatrix = math::transform3f::identity();
            math::transform3f projMatrix = math::transform3f::identity();
            math::vector4f cameraPosition = math::vector4f(0, 0, 0, 1);
            math::vector4f cameraDirection = math::vector4f(1, 0, 0, 0);
            math::vector4f rtBounds = {0, 0, 0, 0};
        }
        _frameConstants;
        
        const std::shared_ptr<PlatformInterface> _platform;
        std::unordered_set<std::string> _shaderNames;
    };
}
