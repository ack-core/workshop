#pragma once

namespace native {
    class Win32Platform : public PlatformInterface {
    public:
        Win32Platform();
        ~Win32Platform();

        void logInfo(const char *fmt, ...) override;
        void logWarning(const char *fmt, ...) override;
        void logError(const char *fmt, ...) override;

        std::vector<std::string> formFileList(const char *dirPath) const override;
        bool loadFile(const char *filePath, std::unique_ptr<unsigned char []> &data, std::size_t &size) const override;

        float getNativeScreenWidth() const override;
        float getNativeScreenHeight() const override;
        void *setNativeRenderingContext(void *context) override;
        
        PlatformCallbackToken addKeyboardCallbacks(
            std::function<void(const PlatformKeyboardEventArgs &)> &&down, 
            std::function<void(const PlatformKeyboardEventArgs &)> &&up
        ) override;
        
        PlatformCallbackToken addMouseCallbacks(
            std::function<void(const PlatformMouseEventArgs &)> &&press,
            std::function<void(const PlatformMouseEventArgs &)> &&move,
            std::function<void(const PlatformMouseEventArgs &)> &&release
        ) override;

        void removeCallbacks(PlatformCallbackToken& token) override;

        void showCursor() override;
        void hideCursor() override;

        void run(std::function<void(float)> &&updateAndDraw) override;
    };
}
