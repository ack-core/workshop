
namespace foundation {
    class WindowsPlatform : public PlatformInterface {
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
            std::function<void(const PlatformKeyboardEventArgs &)> &&down,
            std::function<void(const PlatformKeyboardEventArgs &)> &&up
        ) override;

        EventHandlersToken addInputEventHandlers(
            std::function<void(const char(&utf8char)[4])> &&input,
            std::function<void()> &&backspace
        ) override;

        EventHandlersToken addMouseEventHandlers(
            std::function<void(const PlatformMouseEventArgs &)> &&press,
            std::function<void(const PlatformMouseEventArgs &)> &&move,
            std::function<void(const PlatformMouseEventArgs &)> &&release
        ) override;

        EventHandlersToken addTouchEventHandlers(
            std::function<void(const PlatformTouchEventArgs &)> &&start,
            std::function<void(const PlatformTouchEventArgs &)> &&move,
            std::function<void(const PlatformTouchEventArgs &)> &&finish
        ) override;

        EventHandlersToken addGamepadEventHandlers(
            std::function<void(const PlatformGamepadEventArgs &)> &&buttonPress,
            std::function<void(const PlatformGamepadEventArgs &)> &&buttonRelease
        ) override;

        void removeEventHandlers(EventHandlersToken token) override;

        void run(std::function<void(float)> &&updateAndDraw) override;
        void exit() override;
        void logMsg(const char *fmt, ...) override;
    };
}