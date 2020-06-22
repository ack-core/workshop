
namespace engine {
    class WindowsPlatform : public Platform {
    public:
        WindowsPlatform();
        ~WindowsPlatform() override;

        std::vector<std::string> formFileList(const char *dirPath) override;
        bool loadFile(const char *filePath, std::unique_ptr<unsigned char[]> &data, std::size_t &size) override;

        float getNativeScreenWidth() const override;
        float getNativeScreenHeight() const override;

        void *attachNativeRenderingContext(void *context) override;

        void showCursor() override;
        void hideCursor() override;

        void showKeyboard() override;
        void hideKeyboard() override;

        EventHandlersToken addKeyboardEventHandlers(
            std::function<void(const KeyboardEventArgs &)> &&down,
            std::function<void(const KeyboardEventArgs &)> &&up
        ) override;

        EventHandlersToken addInputEventHandlers(
            std::function<void(const char(&utf8char)[4])> &&input,
            std::function<void()> &&backspace
        ) override;

        EventHandlersToken addMouseEventHandlers(
            std::function<void(const MouseEventArgs &)> &&press,
            std::function<void(const MouseEventArgs &)> &&move,
            std::function<void(const MouseEventArgs &)> &&release
        ) override;

        EventHandlersToken addTouchEventHandlers(
            std::function<void(const TouchEventArgs &)> &&start,
            std::function<void(const TouchEventArgs &)> &&move,
            std::function<void(const TouchEventArgs &)> &&finish
        ) override;

        EventHandlersToken addGamepadEventHandlers(
            std::function<void(const GamepadEventArgs &)> &&buttonPress,
            std::function<void(const GamepadEventArgs &)> &&buttonRelease
        ) override;

        void removeEventHandlers(EventHandlersToken token) override;

        void run(std::function<void(float)> &&updateAndDraw) override;
        void exit() override;
        void logMsg(const char *fmt, ...) override;
    };
}