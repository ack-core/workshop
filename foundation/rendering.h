
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
    enum class RenderingTopology {
        LINES = 0,
        LINESTRIP,
        TRIANGLES,
        TRIANGLESTRIP,
        _count
    };

    enum class RenderingShaderInputFormat {
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

    enum class RenderingTextureFormat {
        RGBA8UN = 0,   // rgba 1 byte per channel normalized to [0..1]
        R8UN,          // 1 byte grayscale normalized to [0..1]. In shader .r component is used
        _count
    };
    
    class RenderingShader {
    public:
        // Field description for Vertex Shader input struct
        //
        struct Input {
            const char *name;
            RenderingShaderInputFormat format;
        };
        
    protected:
        virtual ~RenderingShader() = default;
    };

    class RenderingTexture {
    public:
        virtual std::uint32_t getWidth() const = 0;
        virtual std::uint32_t getHeight() const = 0;
        virtual std::uint32_t getMipCount() const = 0;
        virtual RenderingTextureFormat getFormat() const = 0;

    protected:
        virtual ~RenderingTexture() = default;
    };

    class RenderingData {
    public:
        virtual std::uint32_t getCount() const = 0;
        virtual std::uint32_t getStride() const = 0;

    protected:
        virtual ~RenderingData() = default;
    };
    
    using RenderingShaderInputDesc = std::initializer_list<RenderingShader::Input>;
    using RenderingShaderPtr = std::shared_ptr<RenderingShader>;
    using RenderingTexturePtr = std::shared_ptr<RenderingTexture>;
    using RenderingDataPtr = std::shared_ptr<RenderingData>;

    // Render targets decription that is passed to 'beginPass'
    //
    struct RenderingPassConfig {
        bool clearColor = false;
        bool clearDepth = false;

        RenderingTexturePtr target = nullptr;
        float clear[4] = {0.0f};
    
        RenderingPassConfig() {} // change nothing
        RenderingPassConfig(float r, float g, float b) : clearColor(true), clearDepth(true), clear{r, g, b, 1.0f} {} // set default RT and crear it
        RenderingPassConfig(float r, float g, float b, const RenderingTexturePtr &rt) : clearColor(true), clearDepth(true), clear{r, g, b, 1.0f}, target(rt) {} // set rt-texture and clear it
    };

    // Interface provides 3D-visualization methods
    //
    class RenderingInterface {
    public:
        static std::shared_ptr<RenderingInterface> instance(const std::shared_ptr<PlatformInterface> &platform);
    
    public:
        // Update per frame global constants
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
        //     frame_screenBounds       : float4  - screen size in pixels (.rg)
        //
        // Textures. There 8 texture slots. Example of getting color from the last slot: float4 color = _tex2d(7, float2(0, 0));
        //
        // Global functions:
        //     _transform(v, m), _sign(s), _dot(v, v), _sin(v), _cos(v), _norm(v), _lerp(k, v, v), _tex2d(index, v)
        //
        virtual RenderingShaderPtr createShader(const char *name, const char *src, const RenderingShaderInputDesc &vtx, const RenderingShaderInputDesc &itc = {}) = 0;

        // Create texture from binary data
        // @w and @h    - width and height of the 0th mip layer
        // @mips        - array of pointers. Each [i] pointer represents binary data for i'th mip and cannot be nullptr
        //
        virtual RenderingTexturePtr createTexture(RenderingTextureFormat format, std::uint32_t w, std::uint32_t h, const std::uint8_t * const *mips = nullptr, std::uint32_t mcount = 0) = 0;

        // Create geometry
        // @data        - pointer to data (array of structures)
        // @count       - count of structures in array
        // @stride      - size of struture
        // @return      - handle
        //
        virtual RenderingDataPtr createData(const void *data, std::uint32_t count, std::uint32_t stride) = 0;

        // TODO: render states
        // TODO: render targets

        // Pass is a rendering scope with shader, [TODO: render targets and states]
        // @name        - unique constant name to associate parameters
        // @shader      - shader object
        //
        virtual void beginPass(const char *name, const RenderingShaderPtr &shader, const RenderingPassConfig &cfg = {}) = 0;

        // Apply textures
        // @textures    - textures[i] can be nullptr (texture and sampler at i-th position will not be set)
        virtual void applyTextures(const RenderingTexture * const * textures, std::uint32_t tcount) = 0;

        // Update constant buffer of the current shader
        // @constants   - pointer to data for 'const' block. Must have size in bytes according to 'const' block from shader source. Cannot be null
        virtual void applyShaderConstants(const void *constants) = 0;

        // Draw vertexes from RenderingData
        // @vertexData has layout set by current shader. Can be nullptr
        //
        virtual void drawGeometry(const RenderingDataPtr &vertexData, std::uint32_t vcount, RenderingTopology topology) = 0;

        // Draw instanced vertexes from RenderingData
        // @vertexData and @instanceData has layout set by current shader. Both can be nullptr
        //
        virtual void drawGeometry(const RenderingDataPtr &vertexData, const RenderingDataPtr &instanceData, std::uint32_t vcount, std::uint32_t icount, RenderingTopology topology) = 0;

        virtual void endPass() = 0;
        virtual void presentFrame() = 0;

        // Get last rendered frame as a bitmap in memory
        // @imgFrame - array of size = PlatformInterface::getNativeScreenWidth * PlatformInterface::getNativeScreenHeight * 4
        //
        virtual void getFrameBufferData(std::uint8_t *imgFrame) = 0;

    protected:
        virtual ~RenderingInterface() = default;
    };
    
    using RenderingInterfacePtr = std::shared_ptr<RenderingInterface>;    
}
