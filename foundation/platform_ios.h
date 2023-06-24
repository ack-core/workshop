
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
        void formFileList(const char *dirPath, std::function<void(const std::vector<PlatformFileEntry> &)> &&completion) override;
        void loadFile(const char *filePath, std::function<void(std::unique_ptr<uint8_t[]> &&data, std::size_t size)> &&completion) override;
        bool loadFile(const char *filePath, std::unique_ptr<uint8_t[]> &data, std::size_t &size) override;
        
        float getScreenWidth() const override;
        float getScreenHeight() const override;
        
        void *attachNativeRenderingContext(void *context) override;
        
        void showCursor() override;
        void hideCursor() override;
        
        void showKeyboard() override;
        void hideKeyboard() override;
        
        EventHandlerToken addKeyboardEventHandler(std::function<void(const PlatformKeyboardEventArgs &)> &&handler) override;
        EventHandlerToken addInputEventHandler(std::function<void(const char(&utf8char)[4])> &&input, std::function<void()> &&backspace) override;
        EventHandlerToken addMouseEventHandler(std::function<void(const PlatformMouseEventArgs &)> &&handler) override;
        EventHandlerToken addTouchEventHandler(std::function<void(const PlatformTouchEventArgs &)> &&handler) override;
        EventHandlerToken addGamepadEventHandler(std::function<void(const PlatformGamepadEventArgs &)> &&handler) override;
        
        void removeEventHandler(EventHandlerToken token) override;
        
        void run(std::function<void(float)> &&updateAndDraw) override;
        void exit() override;
        
        void logMsg(const char *fmt, ...) override;
        void logError(const char *fmt, ...) override;
        
    private:
        std::mutex _logMutex;
        std::string _executableDirectoryPath;

    private:
        std::thread _ioThread;
        std::condition_variable _ioNotifier;
        std::list<std::unique_ptr<AsyncTask>> _ioBackgroundQueue;
    };
}
