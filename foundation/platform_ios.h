
#include "platform.h"

#include <memory>
#include <atomic>
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
        bool isAsyncWorkInProgress() const override;
        void loadFile(const char *filePath, util::callback<void(ByteDataPtr &&data, std::size_t size)> &&completion) override;
        void saveFile(const char *filePath, const std::uint8_t *data, std::size_t size, util::callback<void(bool)> &&completion) override;
        
        float getScreenWidth() const override;
        float getScreenHeight() const override;
        float getScreenScale() const override;
        
        void *attachNativeRenderingContext(void *context) override;
        
        void showCursor() override;
        void hideCursor() override;
        
        void showKeyboard() override;
        void hideKeyboard() override;
        
        void sendEditorMsg(const std::string &msg, const std::string &data) override;
        void editorLoopbackMsg(const std::string &msg, const std::string &data) override;
        
        EventHandlerToken addEditorEventHandler(util::callback<bool(const std::string &, const std::string &)> &&handler, bool setTop) override;
        EventHandlerToken addKeyboardEventHandler(util::callback<bool(const PlatformKeyboardEventArgs &)> &&handler, bool setTop) override;
        EventHandlerToken addInputEventHandler(util::callback<bool(const char(&utf8char)[4])> &&input, bool setTop) override;
        EventHandlerToken addPointerEventHandler(util::callback<bool(const PlatformPointerEventArgs &)> &&handler, bool setTop) override;
        EventHandlerToken addGamepadEventHandler(util::callback<bool(const PlatformGamepadEventArgs &)> &&handler, bool setTop) override;
        EventHandlerToken addResizeHandler(util::callback<void()> &&handler) override;
        
        void removeEventHandler(EventHandlerToken token) override;

        void sendPointerEvent(const PlatformPointerEventArgs &args) override;
        void sendKeyboardEvent(const PlatformKeyboardEventArgs &args) override;

        void setLoop(util::callback<void(float)> &&updateAndDraw) override;
        void exit(int returnCode) override;
        
        void setSuppressOutput(bool info, bool errors) override;
        void logMsg(const char *fmt, ...) override;
        void logError(const char *fmt, ...) override;

        const StatInfo& getStatistics() const override;

    private:
        std::mutex _logMutex;
        std::atomic<bool> _suppressInfo{false};
        std::atomic<bool> _suppressErrors{false};
        std::string _executableDirectoryPath;
        std::string _documentsPath;
        float _screenScale;
    };
}
