
#include "rendering.h"
#include <unordered_set>

using WebGLId = std::uint32_t;

namespace foundation {
    class WASMShader : public RenderShader {
    public:
        WASMShader(WebGLId shader, const InputLayout &layout, std::uint32_t constBufferLength);
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
        WASMData(WebGLId data, std::uint32_t vcount, std::uint32_t icount, std::uint32_t stride);
        ~WASMData() override;
        
        auto getVertexCount() const -> std::uint32_t override;
        auto getIndexCount() const -> std::uint32_t override;
        auto getStride() const -> std::uint32_t override;
        auto getWebGLData() const -> WebGLId;
        
    private:
        const WebGLId _data;
        const std::uint32_t _stride;
        const std::uint32_t _vcount;
        const std::uint32_t _icount;
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
            RTTexture(const WASMTarget &target, RenderTextureFormat format, WebGLId texture) : _target(target), _format(format), _texture(texture) {}
            ~RTTexture() override;
            
            auto getWidth() const -> std::uint32_t override { return _target._width; }
            auto getHeight() const -> std::uint32_t override { return _target._height; }
            auto getMipCount() const -> std::uint32_t override { return 1; }
            auto getFormat() const -> RenderTextureFormat override { return _format; }
            auto getWebGLTexture() const -> WebGLId override { return _texture; }
            
        private:
            const WASMTarget &_target;
            const RenderTextureFormat _format;
            const WebGLId _texture;
        };

        WASMTarget(WebGLId target, const WebGLId *textures, std::uint32_t count, WebGLId depth, RenderTextureFormat fmt, std::uint32_t w, std::uint32_t h);
        ~WASMTarget() override;
        
        auto getWidth() const -> std::uint32_t override;
        auto getHeight() const -> std::uint32_t override;
        auto getFormat() const -> RenderTextureFormat override;
        auto getTextureCount() const -> std::uint32_t override;
        auto getTexture(unsigned index) const -> const std::shared_ptr<RenderTexture> & override;
        auto getDepth() const -> const std::shared_ptr<RenderTexture> & override;
        auto getWebGLTarget() const -> WebGLId;
        
    private:
        const WebGLId _target;
        const std::uint32_t _count;
        const std::uint32_t _width;
        const std::uint32_t _height;

        std::shared_ptr<RenderTexture> _textures[RenderTarget::MAX_TEXTURE_COUNT] = {nullptr};
        std::shared_ptr<RenderTexture> _depth = nullptr;
    };
    
    class WASMRendering final : public RenderingInterface {
    public:
        WASMRendering(const std::shared_ptr<PlatformInterface> &platform);
        ~WASMRendering() override;
        
        void updateFrameConstants(const math::transform3f &vp, const math::transform3f &svp, const math::transform3f &ivp, const math::vector3f &camPos, const math::vector3f &camDir) override;
        
        auto createShader(const char *name, const char *src, const InputLayout &layout) -> RenderShaderPtr override;
        auto createTexture(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, const std::initializer_list<const void *> &mipsData) -> RenderTexturePtr override;
        auto createRenderTarget(RenderTextureFormat format, std::uint32_t textureCount, std::uint32_t w, std::uint32_t h, bool withZBuffer) -> RenderTargetPtr override;
        auto createData(const void *data, const InputLayout &layout, std::uint32_t vcnt, const std::uint32_t *indexes, std::uint32_t icnt) -> RenderDataPtr override;
        
        auto getBackBufferWidth() const -> float override;
        auto getBackBufferHeight() const -> float override;
        
        void forTarget(const RenderTargetPtr &target, const RenderTexturePtr &depth, const std::optional<math::color> &rgba, util::callback<void(RenderingInterface &)> &&pass) override;
        void applyShader(const RenderShaderPtr &shader, foundation::RenderTopology topology, BlendType blendType, DepthBehavior depthBehavior) override;
        void applyShaderConstants(const void *constants) override;
        void applyTextures(const std::initializer_list<std::pair<const RenderTexturePtr, SamplerType>> &textures) override;
        
        void draw(std::uint32_t vertexCount) override;
        void draw(const RenderDataPtr &inputData, std::uint32_t instanceCount) override;
        void presentFrame() override;
        
    private:
        auto _getUploadBuffer(std::size_t requiredLength) -> std::uint8_t *;
        
        struct FrameConstants {
            math::transform3f plmVPMatrix = math::transform3f::identity();
            math::transform3f stdVPMatrix = math::transform3f::identity();
            math::transform3f invVPMatrix = math::transform3f::identity();
            math::vector4f cameraPosition = math::vector4f(0, 0, 0, 1);
            math::vector4f cameraDirection = math::vector4f(1, 0, 0, 0);
            math::vector4f rtBounds = {0, 0, 0, 0};
        };
        
        const std::shared_ptr<PlatformInterface> _platform;
        
        std::unique_ptr<FrameConstants> _frameConstants;
        std::unique_ptr<std::uint8_t[]> _uploadBufferData;
        std::size_t _uploadBufferLength;
        
        std::unordered_set<std::string> _shaderNames;
        std::shared_ptr<WASMShader> _currentShader;
        
        RenderTopology _topology;
    };
}
