
#include "platform.h"

namespace foundation {
    class WASMPlatform : public PlatformInterface {
    public:
        WASMPlatform();
        ~WASMPlatform() override;
        
        void executeAsync(std::unique_ptr<AsyncTask> &&task) override;
        void loadFile(const char *filePath, util::callback<void(std::unique_ptr<std::uint8_t[]> &&data, std::size_t size)> &&completion) override;
        
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

        void setLoop(util::callback<void(float)> &&updateAndDraw) override;
        void setResizeHandler(util::callback<void()> &&handler) override;
        void exit() override;
        
        void logMsg(const char *fmt, ...) override;
        void logError(const char *fmt, ...) override;

    private:
        std::string _dataPath;
    };
}
