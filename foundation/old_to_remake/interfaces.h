#pragma once

// TODO: graphic state control
// TODO: sound
// TODO: joystick
// TODO: tests

#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace platform {
    struct Base {
    protected:
        Base() = default;
        ~Base() = default;
    
    private:
        Base(Base &&) = delete;
        Base(const Base &) = delete;
        Base &operator =(Base &&) = delete;
        Base &operator =(const Base &) = delete;
    };
    
    enum class KeyboardKey {
        A = 0, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        NUM0, NUM1, NUM2, NUM3, NUM4, NUM5, NUM6, NUM7, NUM8, NUM9,
        LEFT, RIGHT, UP, DOWN,
        SPACE, ENTER, TAB,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10,
        _count
    };
    
    struct KeyboardEventArgs {
        KeyboardKey key;
        bool alt;
        bool shift;
        bool ctrl;
    };
    
    struct MouseEventArgs {
        mutable float coordinateX;
        mutable float coordinateY;
        bool  isLeftButtonPressed;
        bool  isRightButtonPressed;
    };
    
    struct TouchEventArgs {
        float coordinateX;
        float coordinateY;
        std::size_t touchID;
    };
    
    struct GamepadEventArgs {
    };
    
    using EventHandlersToken = unsigned char *;
    
    // Interface provides low-level core methods
    //
    class Platform : public Base {
    public:
        // Thread-safe logging
        void logInfo(const char *fmt, ...);
        void logWarning(const char *fmt, ...);
        void logError(const char *fmt, ...);
        
        // Forms std::vector of file paths in @dirPath
        // @dirPath  - target directory. Example: "data/map1"
        // @return   - vector of paths
        //
        std::vector<std::string> formFileList(const char *dirPath);
        
        // Loads file to memory
        // @filePath - file path. Example: "data/map1/test.png"
        // @return   - true if file successfully loaded. Items returned by formFileList should be successfully loaded.
        //
        bool loadFile(const char *filePath, std::unique_ptr<uint8_t[]> &data, std::size_t &size);
        
        // Returns native screen size in pixels
        //
        float getNativeScreenWidth() const;
        float getNativeScreenHeight() const;
        
        // Connecting render with native window. Used by RenderingDevice. Must be called before run()
        // @context  - platform-dependent handle (ID3D11Device * for windows, EAGLContext * for ios, etc)
        // @return   - platform-dependent result (IDXGISwapChain * for windows)
        //
        void *setNativeRenderingContext(void *context);
        
        // Show/hide PC mouse pointer
        void showCursor();
        void hideCursor();
        
        // Show/hide keyboard on mobile devices
        void showKeyboard();
        void hideKeyboard();
        
        // Set handlers for PC keyboard
        // @return nullptr if is not supported
        //
        EventHandlersToken addKeyboardEventHandlers(
            std::function<void(const KeyboardEventArgs &)> &&down,
            std::function<void(const KeyboardEventArgs &)> &&up
        );
        
        // Set handlers for User's input (physical or virtual keyboard)
        // @return nullptr if is not supported
        //
        EventHandlersToken addInputEventHandlers(
            std::function<void(const char (&utf8char)[4])> &&input,
            std::function<void()> &&backspace
        );
        
        // Set handlers for PC mouse
        // coordinateX/coordinateY of MouseEventArgs struct can be replaced with user's value (Platform will set new pointer coordinates)
        // @return nullptr if is not supported
        //
        EventHandlersToken addMouseEventHandlers(
            std::function<void(const MouseEventArgs &)> &&press,
            std::function<void(const MouseEventArgs &)> &&move,
            std::function<void(const MouseEventArgs &)> &&release
        );
        
        // Set handlers for touch
        // @return nullptr if is not supported
        //
        EventHandlersToken addTouchEventHandlers(
            std::function<void(const TouchEventArgs &)> &&start,
            std::function<void(const TouchEventArgs &)> &&move,
            std::function<void(const TouchEventArgs &)> &&finish
        );
        
        // Set handlers for gamepad
        // @return nullptr if is not supported
        //
        EventHandlersToken addGamepadEventHandlers(
            std::function<void(const GamepadEventArgs &)> &&buttonPress,
            std::function<void(const GamepadEventArgs &)> &&buttonRelease
        );
        
        // Start platform update cycle
        // This method blocks execution until application exit
        // Argument of @updateAndDraw is delta time in seconds
        //
        void run(std::function<void(float)> &&updateAndDraw);
        
        // Remove handlers of any type
        //
        void removeEventHandlers(EventHandlersToken token);
        
        // Breaks platform update cycle
        //
        void exit();
        
    protected:
        Platform() = default;
    };
    
    // Interface provides Audio control methods
    //
    class AudioDevice : public Base {
    public:
        
    protected:
        AudioDevice() = default;
    };

    // Topology of vertex data
    //
    enum class Topology {
        LINES = 0,
        LINESTRIP,
        TRIANGLES,
        TRIANGLESTRIP,
        _count
    };
    
    // Field description for Vertex Shader input struct
    //
    struct ShaderInput {
        enum class Format {
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
        
        const char *name;
        Format format;
    };
    
    class Shader : public Base {
    protected:
        Shader() = default;
    };
    
    class Texture2D : public Base {
    public:
        enum class Format {
            RGBA8UN = 0,   // rgba 1 byte per channel normalized to [0..1]
            RGB8UN = 1,    // rgb 1 byte per channel normalized to [0..1]
            R8UN = 2,      // 1 byte grayscale normalized to [0..1]. In shader .r component is used
            _count
        };
        
        std::uint32_t getWidth() const;
        std::uint32_t getHeight() const;
        std::uint32_t getMipCount() const;
        Texture2D::Format getFormat() const;
    
    protected:
        Texture2D() = default;
    };
    
    class StructuredData : public Base {
    public:
        std::uint32_t getCount() const;
        std::uint32_t getStride() const;
        
    protected:
        StructuredData() = default;
    };
    
    // Interface provides 3D-visualization methods
    //
    class RenderingDevice {
    public:
        void updateCameraTransform(const float (&camPos)[3], const float(&camDir)[3], const float(&camVP)[16]);
        
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
        //         out_position = _mul(_viewProjMatrix, float4(vertex_position, 1.0));
        //     }
        //     fssrc {                          - fragment shader also has float4 'out_color' variable
        //         out_color = input.varName4;
        //     }
        // s--------------------------------------
        // Types:
        //     matrix4, matrix3, float, float2, float3, float4, int, int2, int3, int4, uint, uint2, uint3, uint4
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
        //     _transform(v, m), _sign(s), _dot(v, v), _sin(v), _cos(v), _norm(v), _tex2d(index, v)
        //
        std::shared_ptr<Shader> createShader(
            const char *shadersrc,
            const std::initializer_list<ShaderInput> &vertex,
            const std::initializer_list<ShaderInput> &instance = {},
            const void *prmnt = nullptr
        );
        
        // Create texture from binary data
        // @w and @h    - width and height of the 0th mip layer
        // @imgMipsData - array of pointers. Each [i] pointer represents binary data for i'th mip and cannot be nullptr
        //
        std::shared_ptr<Texture2D> createTexture(
            Texture2D::Format format,
            std::uint32_t width,
            std::uint32_t height,
            const std::initializer_list<const std::uint8_t *> &mipsData = {}
        );
        
        // Create geometry
        // @data        - pointer to data (array of structures)
        // @count       - count of structures in array
        // @stride      - size of struture
        // @return      - handle
        //
        std::shared_ptr<StructuredData> createData(const void *data, std::uint32_t count, std::uint32_t stride);
        
        // TODO: render states
        // TODO: render targets
        
        // Apply shader
        // @shader      - shader object.
        // @constants   - pointer to data for 'const' block. Can be nullptr (constants will not be set)
        //
        void applyShader(const std::shared_ptr<Shader> &shader, const void *constants = nullptr);
        
        // Apply textures. textures[i] can be nullptr (texture will not be set)
        //
        void applyTextures(const std::initializer_list<const Texture2D *> &textures);
        
        // Draw vertexes without geometry
        //
        void drawGeometry(std::uint32_t vertexCount, Topology topology = Topology::TRIANGLES);

        // Draw vertexes from StructuredData
        // @vertexData and @instanceData has layout set by current shader. Both can be nullptr
        //
        void drawGeometry(
            const std::shared_ptr<StructuredData> &vertexData,
            const std::shared_ptr<StructuredData> &instanceData,
            std::uint32_t vertexCount,
            std::uint32_t instanceCount,
            Topology topology = Topology::TRIANGLES
        );

        // TODO: draw indexed geometry
        
        void prepareFrame();
        void presentFrame(float dtSec);
        
        // Get last rendered frame as a bitmap in memory
        // @imgFrame - array of size = PlatformInterface::getNativeScreenWidth * PlatformInterface::getNativeScreenHeight * 4
        //
        void getFrameBufferData(std::uint8_t *imgFrame);
        
    protected:
        RenderingDevice() = default;
    };
    
    std::shared_ptr<Platform> getPlatformInstance();
    std::shared_ptr<AudioDevice> getAudioDeviceInstance(const std::shared_ptr<Platform> &platform);
    std::shared_ptr<RenderingDevice> getRenderingDeviceInstance(const std::shared_ptr<Platform> &platform);
}

