
#include "rendering.h"
#include <unordered_set>

using WebGLId = std::uint32_t;

namespace foundation {
    class WASMShader : public RenderShader {
    public:
        WASMShader(WebGLId shader, InputLayout &&layout, std::uint32_t constBufferLength);
        ~WASMShader() override;
        
        auto getInputLayout() const -> const InputLayout & override;
        auto getConstBufferLength() const -> std::uint32_t;
        auto getWebGLShader() const -> WebGLId;
        
    private:
        const WebGLId _shader;
        const std::uint32_t _constBufferLength;
        const InputLayout _inputLayout;
    };
    
    class WASMData : public RenderData {
    public:
        WASMData(WebGLId data, std::uint32_t count, std::uint32_t stride);
        ~WASMData() override;
        
        auto getCount() const -> std::uint32_t override;
        auto getStride() const -> std::uint32_t override;
        auto getWebGLData() const -> WebGLId;
        
    private:
        const WebGLId _data;
        const std::uint32_t _stride;
        const std::uint32_t _count;
    };
    
    class WASMTexBase : public RenderTexture {
    public:
        ~WASMTexBase() override {}
        virtual WebGLId getWebGLTexture() const = 0;
    };

    class WASMTexture : public WASMTexBase {
    public:
        WASMTexture(WebGLId texture, RenderTextureFormat fmt, std::uint32_t w, std::uint32_t h, std::uint32_t mipCount);
        ~WASMTexture() override;
        
        auto getWidth() const -> std::uint32_t override;
        auto getHeight() const -> std::uint32_t override;
        auto getMipCount() const -> std::uint32_t override;
        auto getFormat() const -> RenderTextureFormat override;
        auto getWebGLTexture() const -> WebGLId override;
        
    private:
        const RenderTextureFormat _format;
        const WebGLId _texture;
        const std::uint32_t _width;
        const std::uint32_t _height;
        const std::uint32_t _mipCount;
    };
    
    class WASMTarget : public RenderTarget {
    public:
        class RTTexture : public WASMTexBase {
        public:
            RTTexture(const WASMTarget &target, WebGLId texture) : _target(target), _texture(texture) {}
            ~RTTexture() override {}
            
            auto getWidth() const -> std::uint32_t override { return _target._width; }
            auto getHeight() const -> std::uint32_t override { return _target._height; }
            auto getMipCount() const -> std::uint32_t override { return 1; }
            auto getFormat() const -> RenderTextureFormat override { return _target._format; }
            auto getWebGLTexture() const -> WebGLId override { return _texture; }
            
        private:
            const WASMTarget &_target;
            const WebGLId _texture;
        };

        WASMTarget(WebGLId target, const WebGLId *textures, unsigned count, RenderTextureFormat fmt, std::uint32_t w, std::uint32_t h, bool hasDepth);
        ~WASMTarget() override;
        
        auto getWidth() const -> std::uint32_t override;
        auto getHeight() const -> std::uint32_t override;
        auto getFormat() const -> RenderTextureFormat override;
        auto getTextureCount() const -> std::uint32_t override;
        auto getTexture(unsigned index) const -> const std::shared_ptr<RenderTexture> & override;
        auto getWebGLTarget() const -> WebGLId;
        bool hasDepthBuffer() const override;
        
    private:
        const RenderTextureFormat _format;
        const unsigned _count;
        const WebGLId _target;
        const std::uint32_t _width;
        const std::uint32_t _height;
        const bool _hasDepth;

        std::shared_ptr<RenderTexture> _textures[RenderTarget::MAX_TEXTURE_COUNT] = {nullptr};
    };
    
    class WASMRendering final : public RenderingInterface {
    public:
        WASMRendering(const std::shared_ptr<PlatformInterface> &platform);
        ~WASMRendering() override;
        
        void updateFrameConstants(const math::transform3f &view, const math::transform3f &proj, const math::vector3f &camPos, const math::vector3f &camDir) override;
        
        auto getScreenCoordinates(const math::vector3f &worldPosition) -> math::vector2f override;
        auto getWorldDirection(const math::vector2f &screenPosition) -> math::vector3f override;
        
        auto createShader(const char *name, const char *src, InputLayout &&layout) -> RenderShaderPtr override;
        auto createTexture(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, const std::initializer_list<const void *> &mipsData) -> RenderTexturePtr override;
        auto createRenderTarget(RenderTextureFormat format, unsigned textureCount, std::uint32_t w, std::uint32_t h, bool withZBuffer) -> RenderTargetPtr override;
        auto createData(const void *data, const std::vector<InputLayout::Attribute> &layout, std::uint32_t count) -> RenderDataPtr override;
        
        auto getBackBufferWidth() const -> float override;
        auto getBackBufferHeight() const -> float override;
        
        void applyState(const RenderShaderPtr &shader, const RenderPassConfig &cfg) override;
        void applyShaderConstants(const void *constants) override;
        void applyTextures(const std::initializer_list<std::pair<const RenderTexturePtr *, SamplerType>> &textures) override;
        
        void draw(std::uint32_t vertexCount, RenderTopology topology) override;
        void draw(const RenderDataPtr &vertexData, RenderTopology topology) override;
        void drawIndexed(const RenderDataPtr &vertexData, const RenderDataPtr &indexData, RenderTopology topology) override;
        void drawInstanced(const RenderDataPtr &vertexData, const RenderDataPtr &instanceData, RenderTopology topology) override;
        
        void presentFrame() override;
        
    private:
        auto _getUploadBuffer(std::size_t requiredLength) -> std::uint8_t *;
        
        struct FrameConstants {
            math::transform3f viewMatrix = math::transform3f::identity();
            math::transform3f projMatrix = math::transform3f::identity();
            math::vector4f cameraPosition = math::vector4f(0, 0, 0, 1);
            math::vector4f cameraDirection = math::vector4f(1, 0, 0, 0);
            math::vector4f rtBounds = {0, 0, 0, 0};
        }
        *_frameConstants;
        
        const std::shared_ptr<PlatformInterface> _platform;
        
        std::uint8_t *_uploadBufferData;
        std::size_t _uploadBufferLength;
        
        std::unordered_set<std::string> _shaderNames;
        std::shared_ptr<WASMShader> _currentShader;
    };
}
