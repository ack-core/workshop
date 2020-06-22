
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace engine {
    enum class KeyboardKey {
        A = 0, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        NUM0, NUM1, NUM2, NUM3, NUM4, NUM5, NUM6, NUM7, NUM8, NUM9,
        LEFT, RIGHT, UP, DOWN,
        SPACE, ENTER, TAB,
        CTRL, SHIFT, ALT,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10,
        _count
    };

    struct KeyboardEventArgs {
        KeyboardKey key;
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

    using EventHandlersToken = void *;

    // Interface provides low-level core methods
    //
    class Platform {
    public:
        static std::shared_ptr<Platform> instance();

    public:
        // Thread-safe logging
        virtual void logMsg(const char *fmt, ...) = 0;

        // Forms std::vector of file paths in @dirPath
        // @dirPath  - target directory. Example: "data/map1"
        // @return   - vector of paths
        //
        virtual std::vector<std::string> formFileList(const char *dirPath) = 0;

        // Loads file to memory
        // @filePath - file path. Example: "data/map1/test.png"
        // @return   - true if file successfully loaded. Items returned by formFileList should be successfully loaded.
        //
        virtual bool loadFile(const char *filePath, std::unique_ptr<uint8_t[]> &data, std::size_t &size) = 0;

        // Returns native screen size in pixels
        //
        virtual float getNativeScreenWidth() const = 0;
        virtual float getNativeScreenHeight() const = 0;

        // Connecting render with native window. Used by RenderingDevice. Must be called before run()
        // @context  - platform-dependent handle (ID3D11Device * for windows, EAGLContext * for ios, etc)
        // @return   - platform-dependent result (IDXGISwapChain * for windows)
        //
        virtual void *attachNativeRenderingContext(void *context) = 0;

        // Show/hide mouse pointer if supported
        virtual void showCursor() = 0;
        virtual void hideCursor() = 0;

        // Show/hide keyboard if supported
        virtual void showKeyboard() = 0;
        virtual void hideKeyboard() = 0;

        // Set handlers for keyboard
        // @return nullptr if not supported
        //
        virtual EventHandlersToken addKeyboardEventHandlers(
            std::function<void(const KeyboardEventArgs &)> &&down,
            std::function<void(const KeyboardEventArgs &)> &&up
        ) = 0;

        // Set handlers for User's input (physical or virtual keyboard)
        // @return nullptr if not supported
        //
        virtual EventHandlersToken addInputEventHandlers(
            std::function<void(const char(&utf8char)[4])> &&input,
            std::function<void()> &&backspace
        ) = 0;

        // Set handlers for PC mouse
        // coordinateX/coordinateY of MouseEventArgs struct can be replaced with user's value (Platform will set new pointer coordinates)
        // @return nullptr if not supported
        //
        virtual EventHandlersToken addMouseEventHandlers(
            std::function<void(const MouseEventArgs &)> &&press,
            std::function<void(const MouseEventArgs &)> &&move,
            std::function<void(const MouseEventArgs &)> &&release
        ) = 0;

        // Set handlers for touch
        // @return nullptr if not supported
        //
        virtual EventHandlersToken addTouchEventHandlers(
            std::function<void(const TouchEventArgs &)> &&start,
            std::function<void(const TouchEventArgs &)> &&move,
            std::function<void(const TouchEventArgs &)> &&finish
        ) = 0;

        // Set handlers for gamepad
        // @return nullptr if not supported
        //
        virtual EventHandlersToken addGamepadEventHandlers(
            std::function<void(const GamepadEventArgs &)> &&buttonPress,
            std::function<void(const GamepadEventArgs &)> &&buttonRelease
        ) = 0;

        // Remove handlers of any type
        //
        virtual void removeEventHandlers(EventHandlersToken token) = 0;

        // Start platform update cycle
        // This method blocks execution until application exited
        // Argument of @updateAndDraw is delta time in seconds
        //
        virtual void run(std::function<void(float)> &&updateAndDraw) = 0;

        // Breaks platform update cycle
        //
        virtual void exit() = 0;

    protected:
        virtual ~Platform() = default;
    };
}
