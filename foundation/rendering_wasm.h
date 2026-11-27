
#include "rendering.h"
#include <unordered_set>

using WebGLId = std::uint32_t;

namespace foundation {
    class WASMShader : public RenderShader {
    public:
        WASMShader(WebGLId shader, const InputLayout &layout, std::uint32_t constBufferLength);
        ~WASMShader() override;
        
        auto getInputLayout() const -> const InputLayout & override;
        auto getConstBufferLength() const -> std::uint32_t override;
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
        WASMTexBase();
        ~WASMTexBase() override;
        virtual WebGLId getWebGLTexture() const = 0;
        virtual bool isCube() const { return false; }
    };

    class WASMTexture : public WASMTexBase {
    public:
        WASMTexture(WebGLId texture, RenderTextureFormat fmt, std::uint32_t w, std::uint32_t h, std::uint32_t mipCount, bool cube = false);
        ~WASMTexture() override;
        
        auto getWidth() const -> std::uint32_t override;
        auto getHeight() const -> std::uint32_t override;
        auto getMipCount() const -> std::uint32_t override;
        auto getFormat() const -> RenderTextureFormat override;
        auto getUVStart() const -> math::vector2f override;
        auto getUVSize() const -> math::vector2f override;
        auto getWebGLTexture() const -> WebGLId override;
        auto isCube() const -> bool override;

    private:
        const RenderTextureFormat _format;
        const WebGLId _texture;
        const std::uint32_t _width;
        const std::uint32_t _height;
        const std::uint32_t _mipCount;
        const bool _cube;
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
            auto getUVStart() const -> math::vector2f override { return math::vector2f(0.0f, 0.0f); }
            auto getUVSize() const -> math::vector2f override { return math::vector2f(1.0f, 1.0f); }
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
        
        auto createShader(const char *src, const InputLayout &layout, bool suppressErrors) -> RenderShaderPtr override;
        auto createTexture(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, const std::initializer_list<const void *> &mipsData) -> RenderTexturePtr override;
        auto createCubeTexture(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, const void *(&faceData)[6]) -> RenderTexturePtr override;
        auto createRenderTarget(RenderTextureFormat format, std::uint32_t textureCount, std::uint32_t w, std::uint32_t h, bool withZBuffer) -> RenderTargetPtr override;
        auto createData(const InputLayout &layout, const void *data, std::uint32_t vcnt, const std::uint32_t *indexes, std::uint32_t icnt) -> RenderDataPtr override;
        
        auto getBackBufferSize() const -> math::vector2f override;
        auto getStdVPMatrix() const -> math::transform3f override;
        
        void forTarget(const RenderTargetPtr &target, const RenderTexturePtr &depth, const std::optional<math::color> &rgba, util::callback<void(RenderingInterface &)> &&pass) override;
        void applyShader(const RenderShaderPtr &shader, foundation::RenderTopology topology, BlendType blendType, DepthBehavior depthBehavior) override;
        void setWireframeEnabled(bool enabled) override;
        void applyShaderConstants(const void *constants) override;
        void applyTextures(const std::initializer_list<std::pair<RenderTexturePtr, SamplerType>> &textures) override;
        void applyTextures(const std::vector<std::pair<RenderTexturePtr, SamplerType>> &textures) override;
        
        void draw(std::uint32_t vertexCount) override;
        void draw(const RenderDataPtr &inputData, std::uint32_t instanceCount, std::uint32_t start, std::uint32_t count) override;
        void draw(const void *data, std::uint32_t vcnt, const std::uint32_t *indexes = nullptr, std::uint32_t icnt = 0) override;
        auto presentFrame() -> std::unique_ptr<std::uint8_t[]> override;
        auto getStatistics() const -> const StatInfo & override;

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
        std::size_t _uploadBufferLength;
        ByteDataPtr _uploadBufferData;

        std::unordered_map<std::string, std::weak_ptr<RenderShader>> _shaders;
        std::shared_ptr<WASMShader> _currentShader;
        
        RenderTargetPtr _currentTarget;
        RenderTopology _topology;
        
        bool _isForTarget = false;
    };
}
