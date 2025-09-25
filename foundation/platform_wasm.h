
#include "platform.h"

namespace foundation {
    class WASMPlatform : public PlatformInterface {
    public:
        WASMPlatform();
        ~WASMPlatform() override;
        
        void executeAsync(std::unique_ptr<AsyncTask> &&task) override;
        void loadFile(const char *filePath, util::callback<void(std::unique_ptr<std::uint8_t[]> &&data, std::size_t size)> &&completion) override;
        void saveFile(const char *filePath, const std::uint8_t *data, std::size_t size, util::callback<void(bool)> &&completion) override;
        
        float getScreenWidth() const override;
        float getScreenHeight() const override;
        
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
        
        void removeEventHandler(EventHandlerToken token) override;

        void setLoop(util::callback<void(float)> &&updateAndDraw) override;
        void setResizeHandler(util::callback<void()> &&handler) override;
        void exit() override;
        
        void logMsg(const char *fmt, ...) override;
        void logError(const char *fmt, ...) override;

    private:
        void _output(bool error, const char *fmt, va_list arglist);

        std::string _dataPath;
    };
}
