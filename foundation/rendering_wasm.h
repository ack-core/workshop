
#include "rendering.h"
#include <unordered_set>

namespace foundation {
    class WASMShader : public RenderShader {
    public:
        WASMShader(const void *webglShader, const InputLayout &layout, std::uint32_t constBufferLength);
        ~WASMShader() override;
        
        auto getInputLayout() const -> const InputLayout & override;
        auto getConstBufferLength() const -> std::uint32_t;
        auto getWebGLShader() const -> const void *;
        
    private:
        const void *_shader;
        const std::uint32_t _constBufferLength;
        const InputLayout _inputLayout;
    };
    
    class WASMData : public RenderData {
    public:
        WASMData(const void *webglData, std::uint32_t count, std::uint32_t stride);
        ~WASMData() override;
        
        auto getCount() const -> std::uint32_t override;
        auto getStride() const -> std::uint32_t override;
        auto getWebGLData() const -> const void *;
        
    private:
        const void *_data;
        std::uint32_t _count;
        std::uint32_t _stride;
    };
    
    class WASMRendering final : public RenderingInterface {
    public:
        WASMRendering(const std::shared_ptr<PlatformInterface> &platform);
        ~WASMRendering() override;
        
        void updateFrameConstants(const math::transform3f &view, const math::transform3f &proj, const math::vector3f &camPos, const math::vector3f &camDir) override;
        
        auto getScreenCoordinates(const math::vector3f &worldPosition) -> math::vector2f override;
        auto getWorldDirection(const math::vector2f &screenPosition) -> math::vector3f override;
        
        auto createShader(const char *name, const char *src, const InputLayout &layout) -> RenderShaderPtr override;
        auto createTexture(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, const std::initializer_list<const void *> &mipsData) -> RenderTexturePtr override;
        auto createRenderTarget(RenderTextureFormat format, unsigned textureCount, std::uint32_t w, std::uint32_t h, bool withZBuffer) -> RenderTargetPtr override;
        auto createData(const void *data, const std::vector<InputLayout::Attribute> &layout, std::uint32_t count) -> RenderDataPtr override;
        
        auto getBackBufferWidth() const -> float override;
        auto getBackBufferHeight() const -> float override;
        
        void applyState(const RenderShaderPtr &shader, const RenderPassConfig &cfg) override;
        void applyShaderConstants(const void *constants) override;
        void applyTextures(const RenderTexturePtr *textures, std::uint32_t count) override;
        
        void draw(std::uint32_t vertexCount, RenderTopology topology) override;
        void draw(const RenderDataPtr &vertexData, RenderTopology topology) override;
        void drawIndexed(const RenderDataPtr &vertexData, const RenderDataPtr &indexData, RenderTopology topology) override;
        void drawInstanced(const RenderDataPtr &vertexData, const RenderDataPtr &instanceData, RenderTopology topology) override;
        
        void presentFrame() override;
        
    private:
        struct FrameConstants {
            math::transform3f viewMatrix = math::transform3f::identity();
            math::transform3f projMatrix = math::transform3f::identity();
            math::vector4f cameraPosition = math::vector4f(0, 0, 0, 1);
            math::vector4f cameraDirection = math::vector4f(1, 0, 0, 0);
            math::vector4f rtBounds = {0, 0, 0, 0};
        }
        *_frameConstants;
        
        const std::shared_ptr<PlatformInterface> _platform;
        
        std::uint8_t *_drawConstantBufferData;
        std::size_t _drawConstantBufferLength;
        
        std::unordered_set<std::string> _shaderNames;
        std::shared_ptr<WASMShader> _currentShader;
    };
}
