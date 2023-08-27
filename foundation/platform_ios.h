
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
        void formFileList(const char *dirPath, util::callback<void(const std::vector<PlatformFileEntry> &)> &&completion) override;
        void loadFile(const char *filePath, util::callback<void(std::unique_ptr<uint8_t[]> &&data, std::size_t size)> &&completion) override;
        bool loadFile(const char *filePath, std::unique_ptr<uint8_t[]> &data, std::size_t &size) override;
        
        float getScreenWidth() const override;
        float getScreenHeight() const override;
        
        void *attachNativeRenderingContext(void *context) override;
        
        void showCursor() override;
        void hideCursor() override;
        
        void showKeyboard() override;
        void hideKeyboard() override;
        
        EventHandlerToken addKeyboardEventHandler(util::callback<void(const PlatformKeyboardEventArgs &)> &&handler) override;
        EventHandlerToken addInputEventHandler(util::callback<void(const char(&utf8char)[4])> &&input, util::callback<void()> &&backspace) override;
        EventHandlerToken addPointerEventHandler(util::callback<bool(const PlatformPointerEventArgs &)> &&handler) override;
        EventHandlerToken addGamepadEventHandler(util::callback<void(const PlatformGamepadEventArgs &)> &&handler) override;
        
        void removeEventHandler(EventHandlerToken token) override;
        
        void run(util::callback<void(float)> &&updateAndDraw) override;
        void exit() override;
        
        void logMsg(const char *fmt, ...) override;
        void logError(const char *fmt, ...) override;

    private:
        std::mutex _logMutex;
        std::string _executableDirectoryPath;

    private:
        struct BackgroundThread {
            std::thread thread;
            std::mutex mutex;
            std::condition_variable notifier;
            std::list<std::unique_ptr<AsyncTask>> queue;
        };
        
        BackgroundThread _io;
        BackgroundThread _worker;

    private:
        void _backgroundThreadLoop(BackgroundThread &data);
    };
}
