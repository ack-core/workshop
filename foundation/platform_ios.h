
#include "platform.h"

#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <list>

namespace foundation {
    class IOSPlatform : public PlatformInterface {
    public:
        IOSPlatform();
        ~IOSPlatform() override;
        
        void executeAsync(std::unique_ptr<AsyncTask> &&task) override;
        void loadFile(const char *filePath, util::callback<void(std::unique_ptr<std::uint8_t[]> &&data, std::size_t size)> &&completion) override;
        void peekFile(util::callback<void(std::unique_ptr<std::uint8_t[]> &&data, std::size_t size)> &&completion) override;
        
        float getScreenWidth() const override;
        float getScreenHeight() const override;
        
        void *attachNativeRenderingContext(void *context) override;
        
        void showCursor() override;
        void hideCursor() override;
        
        void showKeyboard() override;
        void hideKeyboard() override;
        
        void sendEditorMsg(const std::string &msg) override;
        void editorLoopbackMsg(const std::string &msg) override;
        
        EventHandlerToken addEditorEventHandler(util::callback<bool(const std::string &)> &&handler, bool setTop) override;
        EventHandlerToken addKeyboardEventHandler(util::callback<bool(const PlatformKeyboardEventArgs &)> &&handler, bool setTop) override;
        EventHandlerToken addInputEventHandler(util::callback<bool(const char(&utf8char)[4])> &&input, bool setTop) override;
        EventHandlerToken addPointerEventHandler(util::callback<bool(const PlatformPointerEventArgs &)> &&handler, bool setTop) override;
        EventHandlerToken addGamepadEventHandler(util::callback<bool(const PlatformGamepadEventArgs &)> &&handler, bool setTop) override;
        
        void removeEventHandler(EventHandlerToken token) override;
        
        void setLoop(util::callback<void(float)> &&updateAndDraw) override;
        void setResizeHandler(util::callback<void()> &&handler) override;
        void exit() override;
        
        void logMsg(const char *fmt, ...) override;
        void logError(const char *fmt, ...) override;

    private:
        std::mutex _logMutex;
        std::string _executableDirectoryPath;
    };
}
