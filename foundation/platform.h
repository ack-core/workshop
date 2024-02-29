
// TODO: screen safe zones

#pragma once
#include "util.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace foundation {
    using EventHandlerToken = unsigned char *;
    const EventHandlerToken INVALID_EVENT_TOKEN = nullptr;
    const std::size_t INVALID_POINTER_ID = std::size_t(-1);
    
    struct PlatformFileEntry {
        std::string name;
        bool isDirectory = false;
    };
    
    struct PlatformKeyboardEventArgs {
        enum class Key : std::uint32_t {
            A = 0, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
            NUM0, NUM1, NUM2, NUM3, NUM4, NUM5, NUM6, NUM7, NUM8, NUM9,
            LEFT, RIGHT, UP, DOWN,
            SPACE, ENTER, TAB,
            CTRL, SHIFT, ALT,
            F1, F2, F3, F4, F5, F6, F7, F8, F9, F10,
            _count
        };
        enum class EventType : std::uint32_t {
            UNKNOWN,
            PRESS,
            RELEASE
        };
        
        EventType type = EventType::UNKNOWN;
        Key key = Key(0xFF);
    };
    
    struct PlatformPointerEventArgs {
        enum class EventType : std::uint32_t {
            UNKNOWN = 0,
            START,
            MOVE,
            FINISH,
            CANCEL
        };
        
        EventType type = EventType::UNKNOWN;
        std::size_t pointerID = INVALID_POINTER_ID;
        mutable float coordinateX = 0.0f;
        mutable float coordinateY = 0.0f;
        
        struct {
            std::int32_t wheel : 8;
            std::int32_t isLeftButton : 1;
            std::int32_t isRightButton : 1;
        }
        flags;
    };
  
    // TODO:
    struct PlatformGamepadEventArgs {};
    
    // Classes to control asynchronous work
    //
    class AsyncTask {
    public:
        virtual void executeInBackground() = 0;
        virtual void executeInMainThread() = 0;
        
    public:
        virtual ~AsyncTask() = default;
    };
    
    template<typename Context> class CommonAsyncTask : public Context, public AsyncTask {
    public:
        CommonAsyncTask(util::callback<void(Context &context)> &&background, util::callback<void(Context &context)> &&main) : _background(std::move(background)), _main(std::move(main)) {}
        ~CommonAsyncTask() override {}
        
        void executeInBackground() override { _background(*this); }
        void executeInMainThread() override { _main(*this); }
        
    private:
        util::callback<void(Context &context)> _background;
        util::callback<void(Context &context)> _main;
    };
    
    // If some subsystem requires only error output
    //
    class LoggerInterface {
    public:
        virtual void logMsg(const char *fmt, ...) = 0;
        virtual void logError(const char *fmt, ...) = 0;
        
    public:
        virtual ~LoggerInterface() = default;
    };
    
    using LoggerInterfacePtr = std::shared_ptr<LoggerInterface>;
    
    // Interface provides low-level core methods
    //
    class PlatformInterface : public LoggerInterface {
    public:
        static std::shared_ptr<PlatformInterface> instance();
        
    public:
        // Execute task in IO Thread
        // @task     - task to be executed
        virtual void executeAsync(std::unique_ptr<AsyncTask> &&task) = 0;
        
        // Loads file to memory
        // @filePath - file path. Example: "data/map1/test.png"
        // @return   - data != nullptr and size != 0 if file opened successfully.
        // @completion called from the main thread
        //
        virtual void loadFile(const char *filePath, util::callback<void(std::unique_ptr<std::uint8_t[]> &&data, std::size_t size)> &&completion) = 0;
        
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
        virtual EventHandlerToken addKeyboardEventHandler(util::callback<void(const PlatformKeyboardEventArgs &)> &&handler) = 0;
        
        // Set handlers for User's input (virtual keyboard)
        // @return nullptr if not supported
        //
        virtual EventHandlerToken addInputEventHandler(util::callback<void(const char(&utf8char)[4])> &&input, util::callback<void()> &&backspace) = 0;
        
        // Set handlers for PC mouse or touch
        // coordinateX/coordinateY of PlatformPointerEventArgs struct can be replaced on PC with user's value (PlatformInterface will set new pointer coordinates)
        // @return nullptr if not supported
        //
        virtual EventHandlerToken addPointerEventHandler(util::callback<bool(const PlatformPointerEventArgs &)> &&handler) = 0;
        
        // Set handlers for gamepad
        // @return nullptr if not supported
        //
        virtual EventHandlerToken addGamepadEventHandler(util::callback<void(const PlatformGamepadEventArgs &)> &&handler) = 0;
        
        // Remove handlers of any type
        //
        virtual void removeEventHandler(EventHandlerToken token) = 0;
        
        // Start platform update cycle
        // This method blocks execution until application exited
        // Argument of @updateAndDraw is delta time in seconds
        //
        virtual void setLoop(util::callback<void(float)> &&updateAndDraw) = 0;
        
        // Set handler for window resize
        //
        virtual void setResizeHandler(util::callback<void()> &&handler) = 0;
        
        // Breaks platform update cycle
        //
        virtual void exit() = 0;
        
    public:
        virtual ~PlatformInterface() = default;
    };
    
    using PlatformInterfacePtr = std::shared_ptr<PlatformInterface>;
}
