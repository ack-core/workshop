
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace foundation {
    using EventHandlerToken = unsigned char *;

    struct FileEntry {
        std::string name;
        bool isDirectory = false;
    };

    struct PlatformKeyboardEventArgs {
        enum class Key {
            A = 0, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
            NUM0, NUM1, NUM2, NUM3, NUM4, NUM5, NUM6, NUM7, NUM8, NUM9,
            LEFT, RIGHT, UP, DOWN,
            SPACE, ENTER, TAB,
            CTRL, SHIFT, ALT,
            F1, F2, F3, F4, F5, F6, F7, F8, F9, F10,
            _count
        };
        enum class EventType {
            UNKNOWN,
            PRESS,
            RELEASE
        };

        EventType type = EventType::UNKNOWN;
        Key key = Key(0xFF);
    };

    struct PlatformMouseEventArgs {
        enum class EventType {
            UNKNOWN,
            PRESS,
            MOVE,
            RELEASE
        };
    
        EventType type = EventType::UNKNOWN;
        mutable float coordinateX = 0.0f;
        mutable float coordinateY = 0.0f;
        bool captured = false;
        bool isLeftButton = false;
        bool isRightButton = false;
        int wheel = 0;
    };

    struct PlatformTouchEventArgs {
        enum class EventType {
            UNKNOWN,
            START,
            MOVE,
            FINISH,
            CANCEL
        };
    
        EventType type = EventType::UNKNOWN;
        float coordinateX = 0.0f;
        float coordinateY = 0.0f;
        std::size_t touchID = std::size_t(-1);
    };

    // TODO:
    struct PlatformGamepadEventArgs {};

    // Interface provides low-level core methods
    //
    class PlatformInterface {
    public:
        static std::shared_ptr<PlatformInterface> instance();

    public:
        // Logging
        virtual void logMsg(const char *fmt, ...) = 0;
        virtual void logError(const char *fmt, ...) = 0;

        // Forms std::vector of file paths in @dirPath
        // @dirPath  - target directory. Example: "data/map1"
        // @return   - vector of entries
        //
        virtual std::vector<FileEntry> formFileList(const char *dirPath) = 0;

        // Loads file to memory
        // @filePath - file path. Example: "data/map1/test.png"
        // @return   - true if file successfully loaded. Items returned by formFileList should be successfully loaded.
        //
        virtual bool loadFile(const char *filePath, std::unique_ptr<uint8_t[]> &data, std::size_t &size) = 0;

        // Returns native screen size in pixels
        //
        virtual float getScreenWidth() const = 0;
        virtual float getScreenHeight() const = 0;

        // Connecting render with native window. Used by RenderingInterface implementation. Must be called before run()
        // @context  - platform-dependent handle (ID3D11Device * for windows, EAGLContext * for gles, etc)
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
        virtual EventHandlerToken addKeyboardEventHandler(std::function<void(const PlatformKeyboardEventArgs &)> &&handler) = 0;

        // Set handlers for User's input (virtual keyboard)
        // @return nullptr if not supported
        //
        virtual EventHandlerToken addInputEventHandler(std::function<void(const char(&utf8char)[4])> &&input, std::function<void()> &&backspace) = 0;

        // Set handlers for PC mouse
        // coordinateX/coordinateY of PlatformMouseEventArgs struct can be replaced with user's value (PlatformInterface will set new pointer coordinates)
        // @return nullptr if not supported
        //
        virtual EventHandlerToken addMouseEventHandler(std::function<void(const PlatformMouseEventArgs &)> &&handler) = 0;

        // Set handlers for touch
        // @return nullptr if not supported
        //
        virtual EventHandlerToken addTouchEventHandler(std::function<void(const PlatformTouchEventArgs &)> &&handler) = 0;

        // Set handlers for gamepad
        // @return nullptr if not supported
        //
        virtual EventHandlerToken addGamepadEventHandler(std::function<void(const PlatformGamepadEventArgs &)> &&handler) = 0;

        // Remove handlers of any type
        //
        virtual void removeEventHandler(EventHandlerToken token) = 0;

        // Start platform update cycle
        // This method blocks execution until application exited
        // Argument of @updateAndDraw is delta time in seconds
        //
        virtual void run(std::function<void(float)> &&updateAndDraw) = 0;

        // Breaks platform update cycle
        //
        virtual void exit() = 0;

    protected:
        virtual ~PlatformInterface() = default;
    };
    
    using PlatformInterfacePtr = std::shared_ptr<PlatformInterface>;    
}
