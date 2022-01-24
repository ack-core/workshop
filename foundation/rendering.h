
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "platform.h"

namespace foundation {
    // Topology of vertex data
    //
    enum class RenderTopology {
        POINTS = 0,
        LINES,
        LINESTRIP,
        TRIANGLES,
        TRIANGLESTRIP,
        _count
    };

    enum class RenderShaderInputFormat {
        ID = 0,
        HALF2, HALF4,
        FLOAT1, FLOAT2, FLOAT3, FLOAT4,
        SHORT2, SHORT4,
        SHORT2_NRM, SHORT4_NRM,
        BYTE4, BYTE4_NRM,
        INT1, INT2, INT3, INT4,
        UINT1, UINT2, UINT3, UINT4,
        _count
    };

    enum class RenderTextureFormat {
        R8UN = 0, // 1 byte grayscale normalized to [0..1]. In shader .r component is used
        R32F,     // float grayscale In shader .r component is used
        RGBA8UN,  // rgba 1 byte per channel normalized to [0..1]
        RGBA32F,  // 32-bit float per component
        _count
    };
    
    class RenderShader {
    public:
        // Field description for Vertex Shader input struct
        //
        struct Input {
            const char *name;
            RenderShaderInputFormat format;
        };
        
    protected:
        virtual ~RenderShader() = default;
    };

    class RenderTexture {
    public:
        virtual std::uint32_t getWidth() const = 0;
        virtual std::uint32_t getHeight() const = 0;
        virtual std::uint32_t getMipCount() const = 0;
        virtual RenderTextureFormat getFormat() const = 0;

    protected:
        virtual ~RenderTexture() = default;
    };

    class RenderData {
    public:
        virtual std::uint32_t getCount() const = 0;
        virtual std::uint32_t getStride() const = 0;

    protected:
        virtual ~RenderData() = default;
    };
    
    using RenderShaderInputDesc = std::initializer_list<RenderShader::Input>;
    using RenderShaderPtr = std::shared_ptr<RenderShader>;
    using RenderTexturePtr = std::shared_ptr<RenderTexture>;
    using RenderDataPtr = std::shared_ptr<RenderData>;

    // Render targets decription that is passed to 'beginPass'
    //
    struct RenderPassConfig {
        RenderTexturePtr target = nullptr;

        bool clearColor = false;
        bool clearDepth = false;

        float clear[4] = {0.0f};
        float depth = 0.0f;
    
        RenderPassConfig() {} // change nothing
        RenderPassConfig(float r, float g, float b, float depth = 0.0f) : clearColor(true), clearDepth(true), clear{r, g, b, 1.0f}, depth(depth) {}
        RenderPassConfig(const RenderTexturePtr &rt, float r, float g, float b, float depth = 0.0f) : target(rt), clearColor(true), clearDepth(true), clear{r, g, b, 1.0f}, depth(depth) {}
    };

    // Interface provides 3D-visualization methods
    //
    class RenderingInterface {
    public:
        static std::shared_ptr<RenderingInterface> instance(const std::shared_ptr<PlatformInterface> &platform);
    
    public:
        // Set global per-frame constants, available from shaders
        //
        virtual void updateFrameConstants(const float(&camPos)[3], const float(&camDir)[3], const float(&camVP)[16]) = 0;
        
        // Create shader from source text
        // @name      - name that is used in error messages
        // @vtx       - input layout for vertex shader. All such variables have 'vertex_' prefix.
        // @itc       - input layout for vertex shader. All such variables have 'instance_' prefix.
        // @src       - generic shader source text. Example:
        //
        // Assume that @vtx = {{"position", ShaderInput::Format::FLOAT3}, {"color", ShaderInput::Format::BYTE4_NRM}}
        // So vertex shader has vertex_position and vertex_color input values.
        // s--------------------------------------
        //     fixed {                                           - block of permanent constants. Can be omitted if unused.
        //         constName0 : float4 = [1.0, -3.0, 0.0, 1.0]   - in the code these constants have 'fixed_' prefix
        //         constNames0[2] : float4 = [1.0, 0.3, 0.6, 0.9][0.0, 0.1, 0.2, 0.3] 
        //         constNameI : int2 = [1, -2] 
        //     }
        //     const {                                           - block of per-pass constants. Can be omitted if unused.
        //         constName1 : float4                           - these constants have 'const_' prefix in code
        //         constNames1[16] : float4                      - spaces in/before array braces are not permitted
        //     }
        //     inout {                                           - vertex output and fragment input. Can be omitted if unused.
        //         varName4 : float4                             - these variables have 'input_' prefix in vssrc and 'output_' in fssrc
        //     }                                                 - vertex shader also has float4 'output_position' variable
        //     vssrc {
        //         output_varName4 = _lerp(vertex_color, const_constName0, const_constName1);
        //         output_position = _transform(float4(vertex_position, 1.0), frame_viewProjMatrix);
        //     }
        //     fssrc {                                           - fragment shader has float4 'output_color' variable
        //         output_color = input_varName4;
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
        // Textures. There 8 texture slots. Example of getting color from the last slot: float4 color = _tex2d(7, float2(0, 0));
        //
        // Global functions:
        //     _transform(v, m), _sign(s), _dot(v, v), _sin(v), _cos(v), _norm(v), _lerp(k, v, v), _tex2d(index, v)
        //
        virtual RenderShaderPtr createShader(const char *name, const char *src, const RenderShaderInputDesc &vtx, const RenderShaderInputDesc &itc = {}) = 0;

        // Create texture from binary data
        // @w and @h    - width and height of the 0th mip layer
        // @mips        - array of pointers. Each [i] pointer represents binary data for i'th mip and cannot be nullptr
        //
        virtual RenderTexturePtr createTexture(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, const std::initializer_list<const void *> &mipsData) = 0;

        // Create render target texture
        // @w and @h    - width and height
        //
        virtual RenderTexturePtr createRenderTarget(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, bool withZBuffer) = 0;

        // Create geometry
        // @data        - pointer to data (array of structures)
        // @count       - count of structures in array
        // @stride      - size of struture
        // @return      - handle
        //
        virtual RenderDataPtr createData(const void *data, std::uint32_t count, std::uint32_t stride) = 0;

        // TODO: render states
        // Pass is a rendering scope with shader, [TODO: blending+rasterizer states]
        // @name        - unique constant name to associate parameters
        // @shader      - shader object
        //
        virtual void beginPass(const char *name, const RenderShaderPtr &shader, const RenderPassConfig &cfg = {}) = 0;

        // Apply textures
        // @textures    - textures[i] can be nullptr (texture and sampler at i-th position will not be set)
        //
        virtual void applyTextures(const std::initializer_list<const RenderTexturePtr> &textures) = 0;

        // Update constant buffer of the current shader
        // @constants   - pointer to data for 'const' block. Must have size in bytes according to 'const' block from shader source. Cannot be null
        //
        virtual void applyShaderConstants(const void *constants) = 0;

        // Draw vertexes from RenderData
        // @vertexData has layout set by current shader. Can be nullptr
        //
        virtual void drawGeometry(const RenderDataPtr &vertexData, std::uint32_t vcount, RenderTopology topology) = 0;

        // Draw instanced vertexes from RenderData
        // @vertexData and @instanceData has layout set by current shader. Both can be nullptr
        //
        virtual void drawGeometry(const RenderDataPtr &vertexData, const RenderDataPtr &instanceData, std::uint32_t vcount, std::uint32_t icount, RenderTopology topology) = 0;

        virtual void endPass() = 0;
        virtual void presentFrame() = 0;

    protected:
        virtual ~RenderingInterface() = default;
    };
    
    using RenderingInterfacePtr = std::shared_ptr<RenderingInterface>;
}
