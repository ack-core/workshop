
#pragma once
#include "platform.h"
#include "math.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

// TODO: 8 textures max

namespace foundation {
    enum class RenderTopology : std::uint8_t {
        POINTS = 0,
        LINES,
        LINESTRIP,
        TRIANGLES,
        TRIANGLESTRIP,
    };
    
    enum class ZBehaviorType : std::uint8_t {
        DISABLED = 0,
        TEST_ONLY,
        TEST_AND_WRITE,
    };

    enum class SamplerType : std::uint8_t {
        NEAREST = 0,
        LINEAR,
    };

    enum class BlendType : std::uint8_t {
        DISABLED = 0,           // Blending is disabled
        MIXING,                 // sourceRGB * sourceA + destRGB * (1 - sourceA)
        ADDITIVE,               // sourceRGB * sourceA + destRGB
    };
        
    enum class RenderTextureFormat : std::uint8_t {
        R8UN = 0,               // 1 byte grayscale normalized to [0..1]. In shader .r component is used
        R16F,                   // float16 grayscale In shader .r component is used
        R32F,                   // float grayscale In shader .r component is used
        RG8UN,                  // rg 1 byte per channel normalized to [0..1]
        RG16F,                  // 32-bit float per component
        RG32F,                  // 32-bit float per component
        RGBA8UN,                // rgba 1 byte per channel normalized to [0..1]
        RGBA16F,                // rgba 2 byte per channel normalized to [0..1]
        RGBA32F,                // 32-bit float per component
        UNKNOWN = 0xff
    };
    
    enum class InputAttributeFormat : std::uint8_t {
        HALF2, HALF4,
        FLOAT1, FLOAT2, FLOAT3, FLOAT4,
        SHORT2, SHORT4,
        USHORT2, USHORT4,
        SHORT2_NRM, SHORT4_NRM,
        USHORT2_NRM, USHORT4_NRM,
        BYTE4, BYTE4_NRM,
        INT1, INT2, INT3, INT4,
        UINT1, UINT2, UINT3, UINT4,
    };
    
    struct InputLayout {
        struct Attribute {
            const char *name;
            InputAttributeFormat format;
        };

        std::uint32_t repeat = 1;
        std::vector<Attribute> attributes;
    };
    
    class RenderShader {
    public:
        virtual ~RenderShader() = default;
        virtual auto getInputLayout() const -> const InputLayout & = 0;
    };
    
    class RenderTexture {
    public:
        virtual auto getWidth() const -> std::uint32_t = 0;
        virtual auto getHeight() const -> std::uint32_t = 0;
        virtual auto getMipCount() const -> std::uint32_t = 0;
        virtual auto getFormat() const -> RenderTextureFormat = 0;
        
    public:
        virtual ~RenderTexture() = default;
    };

    class RenderTarget {
    public:
        static const std::size_t MAX_TEXTURE_COUNT = 4;
        
        virtual bool hasDepthBuffer() const = 0;
        virtual auto getWidth() const -> std::uint32_t = 0;
        virtual auto getHeight() const -> std::uint32_t = 0;
        virtual auto getFormat() const -> RenderTextureFormat = 0;
        virtual auto getTextureCount() const -> std::uint32_t = 0;
        virtual auto getTexture(unsigned index) const -> const std::shared_ptr<RenderTexture> & = 0;
        
    public:
        virtual ~RenderTarget() = default;
    };

    class RenderData {
    public:
        virtual auto getCount() const -> std::uint32_t = 0;
        virtual auto getStride() const -> std::uint32_t = 0;
        
    public:
        virtual ~RenderData() = default;
    };
    
    using RenderShaderPtr = std::shared_ptr<RenderShader>;
    using RenderTexturePtr = std::shared_ptr<RenderTexture>;
    using RenderTargetPtr = std::shared_ptr<RenderTarget>;
    using RenderDataPtr = std::shared_ptr<RenderData>;
    
    // Render pass decription that is passed to 'applyState'
    //
    struct RenderPassConfig {
        RenderTargetPtr target = nullptr;
        
        bool doClearColor = false;
        bool doClearDepth = false;

        float color[4] = {0.0f};
        float depth = 0.0f;
        
        ZBehaviorType zBehaviorType = ZBehaviorType::TEST_AND_WRITE;
        BlendType blendType = BlendType::DISABLED;
    };
    
    struct RenderPassCommonConfigs {
        static const RenderPassConfig DEFAULT() {
            return {};
        }
        static const RenderPassConfig DEFAULT(const RenderTargetPtr &rt) {
            return { .target = rt };
        }
        static const RenderPassConfig DEPTHCLEAR(const RenderTargetPtr &rt, float d = 0.0f) {
            return RenderPassConfig {
                rt, false, true,
                {0, 0, 0, 0}, d
            };
        }
        static const RenderPassConfig CLEAR(float r, float g, float b, float a = 1.0f, float d = 0.0f) {
            return RenderPassConfig {
                nullptr, true, true,
                {r, g, b, a}, d,
                ZBehaviorType::TEST_AND_WRITE, BlendType::DISABLED,
            };
        }
        static const RenderPassConfig CLEAR(const RenderTargetPtr &rt, float r, float g, float b, float a = 1.0f, float d = 0.0f) {
            return RenderPassConfig {
                rt, true, rt->hasDepthBuffer(),
                {r, g, b, a}, d,
                rt->hasDepthBuffer() ? ZBehaviorType::TEST_AND_WRITE : ZBehaviorType::DISABLED, BlendType::DISABLED,
            };
        }
        static const RenderPassConfig OVERLAY(BlendType blendType) {
            return RenderPassConfig {
                nullptr, false, false,
                {0, 0, 0, 0}, 0.0f,
                ZBehaviorType::DISABLED, blendType,
            };
        }
        static const RenderPassConfig OVERLAY(BlendType blendType, ZBehaviorType zBehaviorType) {
            return RenderPassConfig {
                nullptr, false, false,
                {0, 0, 0, 0}, 0.0f,
                zBehaviorType, blendType,
            };
        }
    };
    
    // Interface provides 3D-visualization methods
    //
    class RenderingInterface {
    public:
        static std::shared_ptr<RenderingInterface> instance(const std::shared_ptr<PlatformInterface> &platform);
        
    public:
        // Set global per-frame constants, available from shaders
        // @proj  - proj matrix
        // @view  - view matrix
        // Matrices are row-major
        //
        virtual void updateFrameConstants(const math::transform3f &view, const math::transform3f &proj, const math::vector3f &camPos, const math::vector3f &camDir) = 0;
        
        // Get screen coords from world position
        //
        virtual auto getScreenCoordinates(const math::vector3f &worldPosition) -> math::vector2f = 0;

        // Get ray direction from screen coords (start point is camera position)
        //
        virtual auto getWorldDirection(const math::vector2f &screenPosition) -> math::vector3f = 0;

        // Create shader from source text
        // @name      - name that is used in error messages
        // @layout    - input layout for vertex shader. Vertex attributes have 'vertex_' prefix
        // @src       - generic shader source text. Example:
        //
        // Assume that @layout = {1, {"position", foundation::InputAttributeFormat::FLOAT3}}
        // So vertex shader has vertex_position and vertex_color input values.
        // s--------------------------------------
        //     fixed {                                           - block of permanent constants. Can be omitted if unused.
        //         constName0 : float4 = [1.0, -3.0, 0.0, 1.0]   - in the code these constants have 'fixed_' prefix
        //         constName1 : int2 = [1, -2]
        //         constNameArray0[2] : float4 = [1.0, 0.3, 0.6, 0.9][0.0, 0.1, 0.2, 0.3]
        //     }
        //     const {                                           - block of per-pass constants. Can be omitted if unused.
        //         constName1 : float4                           - these constants have 'const_' prefix in code
        //         constNamesArray1[16] : float4                 - spaces in/before array braces are not permitted
        //     }
        //     inout {                                           - vertex output and fragment input. Can be omitted if unused.
        //         varName4 : float4                             - these variables have 'output_' prefix in vssrc and 'input_' in fssrc
        //     }
        //     fndef getcolor(float3 rgb) -> float4 {            - example of function defined in shader
        //         return float4(rgb, 1.0);
        //     }
        //     vssrc {
        //         output_varName4 = _lerp(vertex_color, const_constName1, fixed_constName1);
        //         output_position = _transform(float4(vertex_position, 1.0), frame_viewProjMatrix);
        //
        //     }                                                 - vertex shader has float4 'output_position' variable
        //     fssrc {                                           - fragment shader has float4 'output_color[4]' variable. Max four render targets
        //         output_color[0] = input_varName4;             - fragment shader has float2 'fragment_coord' variable with range 0.0 <= x, y <= 1.0
        //     }
        // s--------------------------------------
        // Types:
        //     matrix4, matrix3, float1, float2, float3, float4, int1, int2, int3, int4, uint1, uint2, uint3, uint4
        //
        // Per frame global constants:
        //     frame_viewProjMatrix     : matrix4 - view * projection matrix
        //     frame_cameraPosition     : float4  - camera position (w = 1)
        //     frame_cameraDirection    : float4  - normalized camera direction (w = 0)
        //     frame_rtBounds           : float4  - render target size in pixels (.rg)
        //
        // Textures. There 4 texture slots. Example of getting color from the last slot: float4 color = _tex2linear(3, float2(0, 0));
        //
        // Global functions:
        //     _transform(v, m), _sign(s), _dot(v, v), _sin(v), _cos(v), _norm(v), _lerp(v, v, k), _tex2nearest/_tex2linear/_tex2raw(index, v)
        //
        virtual auto createShader(const char *name, const char *src, InputLayout &&layout) -> RenderShaderPtr = 0;
        
        // Create texture from binary data or empty
        // @w and @h    - width and height of the 0th mip layer
        // @mips        - array of pointers. Each [i] pointer represents binary data for i'th mip and cannot be nullptr. Array can be empty if there is only one mip-level
        //
        virtual auto createTexture(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, const std::initializer_list<const void *> &mipsData) -> RenderTexturePtr = 0;
        
        // Create render target texture
        // @count       - color targets count
        // @w and @h    - width and height
        //
        virtual auto createRenderTarget(RenderTextureFormat format, unsigned textureCount, std::uint32_t w, std::uint32_t h, bool withZBuffer) -> RenderTargetPtr = 0;
        
        // Create data buffer
        // @data        - pointer to data (array of structures)
        // @layout      - vertex description. Should match the description in shader
        // @count       - count of structures in array
        // @return      - handle
        //
        virtual auto createData(const void *data, const InputLayout &layout, std::uint32_t count) -> RenderDataPtr = 0;
        
        // Return actual rendering area size
        //
        virtual auto getBackBufferWidth() const -> float = 0;
        virtual auto getBackBufferHeight() const -> float = 0;
        
        // State is a rendering scope with shader
        // @shader      - shader object
        // @cfg         - render pass configuration
        //
        virtual void applyState(const RenderShaderPtr &shader, RenderTopology topology, const RenderPassConfig &cfg = RenderPassCommonConfigs::DEFAULT()) = 0;
        
        // Apply textures and their sampling type
        // @textures    - texture can be nullptr (texture at i-th position will not be set)
        // @count       - number of textures
        //
        virtual void applyTextures(const std::initializer_list<std::pair<const RenderTexturePtr, SamplerType>> &textures) = 0;
        
        // Update constant buffer of the current shader
        // @constants   - pointer to data for 'const' block. Must have size in bytes according to 'const' block from shader source. Cannot be null
        //
        virtual void applyShaderConstants(const void *constants) = 0;

        // Draw vertexes from RenderData
        // @inputData layout has to match current shader's layout
        //
        virtual void draw(const RenderDataPtr &inputData = nullptr, std::uint32_t instanceCount = 1) = 0;
        
        // Frame finalization
        //
        virtual void presentFrame() = 0;
        
    public:
        virtual ~RenderingInterface() = default;
    };
    
    using RenderingInterfacePtr = std::shared_ptr<RenderingInterface>;
}
