
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "../platform/interfaces.h"

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
        VERTEX_ID = 0,
        HALF2, HALF4,
        FLOAT1, FLOAT2, FLOAT3, FLOAT4,
        SHORT2, SHORT4,
        SHORT2_NRM, SHORT4_NRM,
        BYTE4,
        BYTE4_NRM,
        INT1, INT2, INT3, INT4,
        _count
    };

    enum class RenderingTextureFormat {
        RGBA8UN = 0,   // rgba 1 byte per channel normalized to [0..1]
        RGB8UN,        // rgb 1 byte per channel normalized to [0..1]
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

    class RenderingTexture2D {
    public:
        virtual std::uint32_t getWidth() const = 0;
        virtual std::uint32_t getHeight() const = 0;
        virtual std::uint32_t getMipCount() const = 0;
        virtual RenderingTextureFormat getFormat() const = 0;

    protected:
        virtual ~RenderingTexture2D() = default;
    };

    class RenderingStructuredData {
    public:
        virtual std::uint32_t getCount() const = 0;
        virtual std::uint32_t getStride() const = 0;

    protected:
        virtual ~RenderingStructuredData() = default;
    };

    // Interface provides 3D-visualization methods
    //
    class RenderingInterface {
    public:
        static std::shared_ptr<RenderingInterface> instance(std::shared_ptr<PlatformInterface> &platform);
    
    public:
        // Update per frame global constants
        // 
        virtual void updateCameraTransform(const float(&camPos)[3], const float(&camDir)[3], const float(&camVP)[16]) = 0;

        // Create shader from source text
        // @vertex    - input layout for vertex shader (all such variables have 'vertex_' prefix)
        // @instance  - input layout for vertex shader (all such variables have 'instance_' prefix)
        // @prmnt     - pointer to data for the block of permanent constants (Can be nullptr in case of )
        // @shadersrc - generic shader source text. Example:
        // s--------------------------------------
        //     prmnt {                          - block of permanent constants. Can be omitted if unused.
        //         constName0 : float4          -
        //     }
        //     const {                          - block of per-apply constants. Can be omitted if unused.
        //         constName1 : float4          -
        //         constNames[16] : float4      - spaces in/before array braces are not permitted
        //     }
        //     inter {                          - vertex output/fragment input. Can be omitted if unused.
        //         varName4 : float4            - vertex shader also has float4 'out_position' variable
        //     }
        //     vssrc {                          - assume that input = {{"position", ShaderInput::Format::FLOAT4}, {"color", ShaderInput::Format::BYTE4_NRM}};
        //         inter.varName4 = vertex_color;
        //         out_position = _transform(float4(vertex_position, 1.0), _viewProjMatrix);
        //     }
        //     fssrc {                          - fragment shader has float4 'out_color' variable
        //         out_color = input.varName4;
        //     }
        // s--------------------------------------
        // Types:
        //     matrix4, matrix3, float1, float2, float3, float4, int1, int2, int3, int4, uint1, uint2, uint3, uint4
        //
        // Per frame global constants:
        //     _renderTargetBounds : float2 - render target size in pixels
        //     _viewProjMatrix     : matrix - view * projection matrix
        //     _cameraPosition     : float4 - camera position (w = 1)
        //     _cameraDirection    : float4 - normalized camera direction (w = 0)
        //
        // Textures. There 8 texture slots. Example of getting color from the last slot: float4 color = _tex2d(7, float2(0, 0));
        //
        // Global functions:
        //     _transform(v, m), _sign(s), _dot(v, v), _sin(v), _cos(v), _norm(v), _lerp(v, v, v), _tex2d(index, v)
        //
        virtual std::shared_ptr<RenderingShader> createShader(
            const char *shadersrc,
            const std::initializer_list<RenderingShader::Input> &vertex,
            const std::initializer_list<RenderingShader::Input> &instance = {},
            const void *prmnt = nullptr
        ) = 0;

        // Create texture from binary data
        // @w and @h    - width and height of the 0th mip layer
        // @imgMipsData - array of pointers. Each [i] pointer represents binary data for i'th mip and cannot be nullptr
        //
        virtual std::shared_ptr<RenderingTexture2D> createTexture(
            RenderingTextureFormat format,
            std::uint32_t width,
            std::uint32_t height,
            const std::initializer_list<const std::uint8_t *> &mipsData = {}
        ) = 0;

        // Create geometry
        // @data        - pointer to data (array of structures)
        // @count       - count of structures in array
        // @stride      - size of struture
        // @return      - handle
        //
        virtual std::shared_ptr<RenderingStructuredData> createData(const void *data, std::uint32_t count, std::uint32_t stride) = 0;

        // TODO: render states
        // TODO: render targets

        // Apply shader
        // @shader      - shader object.
        // @constants   - pointer to data for 'const' block. Can be nullptr (constants will not be set)
        //
        virtual void applyShader(const std::shared_ptr<RenderingShader> &shader, const void *constants = nullptr) = 0;

        // Apply textures. textures[i] can be nullptr (texture will not be set)
        //
        virtual void applyTextures(const std::initializer_list<const RenderingTexture2D *> &textures) = 0;

        // Draw vertexes without geometry
        //
        virtual void drawGeometry(std::uint32_t vertexCount, RenderingTopology topology = RenderingTopology::TRIANGLES) = 0;

        // Draw vertexes from StructuredData
        // @vertexData and @instanceData has layout set by current shader. Both can be nullptr
        //
        virtual void drawGeometry(
            const std::shared_ptr<RenderingStructuredData> &vertexData,
            const std::shared_ptr<RenderingStructuredData> &instanceData,
            std::uint32_t vertexCount,
            std::uint32_t instanceCount,
            RenderingTopology topology = RenderingTopology::TRIANGLES
        ) = 0;

        // TODO: draw indexed geometry

        virtual void prepareFrame() = 0;
        virtual void presentFrame(float dtSec) = 0;

        // Get last rendered frame as a bitmap in memory
        // @imgFrame - array of size = PlatformInterface::getNativeScreenWidth * PlatformInterface::getNativeScreenHeight * 4
        //
        virtual void getFrameBufferData(std::uint8_t *imgFrame) = 0;

    protected:
        virtual ~RenderingInterface() = default;
    };
}