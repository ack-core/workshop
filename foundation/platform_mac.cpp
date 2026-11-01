
// TODO:
// + gamepad
// + text input

#ifdef PLATFORM_MAC

#include <atomic>
#include <chrono>
#include <list>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <fstream>
#include <filesystem>
#include <cstdarg>
#include <cstdio>

#include "platform_mac.h"

#include <AppKit/AppKit.h>
#include <MetalKit/MetalKit.h>
#include <Carbon/Carbon.h>
#include <sys/sysctl.h>
#include <unistd.h>

@class AppView;

namespace {
#if defined(IS_TESTS)
    const unsigned APP_WIDTH = foundation::TESTING_RENDER_WIDTH;
    const unsigned APP_HEIGHT = foundation::TESTING_RENDER_HEIGHT;
#else
    const unsigned APP_WIDTH = 1280;
    const unsigned APP_HEIGHT = 720;
#endif
    const unsigned BUFFER_SIZE = 65536;

    const std::uint16_t KEY_CODES[std::size_t(foundation::Key::_count)] = { // index is foundation::Key value
        kVK_ANSI_A, kVK_ANSI_B, kVK_ANSI_C, kVK_ANSI_D, kVK_ANSI_E, kVK_ANSI_F, kVK_ANSI_G, kVK_ANSI_H, kVK_ANSI_I, kVK_ANSI_J, kVK_ANSI_K, kVK_ANSI_L, kVK_ANSI_M,
        kVK_ANSI_N, kVK_ANSI_O, kVK_ANSI_P, kVK_ANSI_Q, kVK_ANSI_R, kVK_ANSI_S, kVK_ANSI_T, kVK_ANSI_U, kVK_ANSI_V, kVK_ANSI_W, kVK_ANSI_X, kVK_ANSI_Y, kVK_ANSI_Z,
        kVK_ANSI_0, kVK_ANSI_1, kVK_ANSI_2, kVK_ANSI_3, kVK_ANSI_4, kVK_ANSI_5, kVK_ANSI_6, kVK_ANSI_7, kVK_ANSI_8, kVK_ANSI_9,
        kVK_LeftArrow, kVK_RightArrow, kVK_UpArrow, kVK_DownArrow,
        kVK_Space, kVK_Return, kVK_Tab,
        kVK_F1, kVK_F2, kVK_F3, kVK_F4, kVK_F5, kVK_F6, kVK_F7, kVK_F8, kVK_F9, kVK_F10,
    };

    unsigned g_width = APP_WIDTH;
    unsigned g_height = APP_HEIGHT;

    char g_buffer[BUFFER_SIZE];
    std::atomic<bool> g_killed {false};
    int g_returnCode = 0;

    NSWindow *g_window = nil;
    AppView *g_view = nil;

    std::weak_ptr<foundation::PlatformInterface> g_instance;
    util::callback<void(float)> g_updateAndDraw;

    foundation::EventHandlerToken g_tokenCounter = reinterpret_cast<foundation::EventHandlerToken>(0x100);

    std::list<std::pair<foundation::EventHandlerToken, util::callback<bool(const std::string &, const std::string &)>>> g_editorHandlers;
    std::list<std::pair<foundation::EventHandlerToken, util::callback<bool(const foundation::PlatformPointerEventArgs &)>>> g_pointerHandlers;
    std::list<std::pair<foundation::EventHandlerToken, util::callback<bool(const foundation::PlatformKeyboardEventArgs &)>>> g_keyboardHandlers;
    std::list<std::pair<foundation::EventHandlerToken, util::callback<void()>>> g_resizeHandlers;

    std::vector<std::unique_ptr<foundation::AsyncTask>> g_foregroundQueue;
    std::mutex g_foregroundMutex;
    std::size_t g_asyncWorkCount = 0;

    struct BackgroundThread {
        std::thread thread;
        std::mutex mutex;
        std::condition_variable notifier;
        std::list<std::unique_ptr<foundation::AsyncTask>> queue;
    };

    BackgroundThread g_io;
    BackgroundThread g_worker;

    void backgroundThreadLoop(BackgroundThread &threadData) {
        printf("[PLATFORM] Background Thread started\n");

        while (true) {
            std::unique_ptr<foundation::AsyncTask> task = nullptr;

            {
                std::unique_lock<std::mutex> guard(threadData.mutex);

                while (threadData.queue.empty() && g_killed == false) {
                    threadData.notifier.wait(guard);
                }
                if (g_killed) {
                    return;
                }

                task = std::move(threadData.queue.front());
                threadData.queue.pop_front();
            }

            if (task) {
                task->executeInBackground();
                std::unique_lock<std::mutex> guard(g_foregroundMutex);
                g_foregroundQueue.emplace_back(std::move(task));
            }
        }
    }
    void backgroundThreadPush(BackgroundThread &threadData, std::unique_ptr<foundation::AsyncTask> &&task) {
        g_asyncWorkCount++;
        {
            std::lock_guard<std::mutex> guard(threadData.mutex);
            threadData.queue.emplace_back(std::move(task));
        }
        threadData.notifier.notify_one();
    }
    void backgroundThreadStop(BackgroundThread &threadData) {
        {
            std::lock_guard<std::mutex> guard(threadData.mutex);
        }
        threadData.notifier.notify_one();
        threadData.thread.join();
    }

    template <typename L, typename H> foundation::EventHandlerToken addHandler(L &list, H &&handler, bool setTop) {
        foundation::EventHandlerToken token = g_tokenCounter++;
        if (setTop) {
            list.emplace_front(std::make_pair(token, std::move(handler)));
        }
        else {
            list.emplace_back(std::make_pair(token, std::move(handler)));
        }
        return token;
    }
    template <typename L> void removeHandler(L &list, foundation::EventHandlerToken token) {
        for (auto index = list.begin(); index != list.end(); ++index) {
            if (index->first == token) {
                list.erase(index);
                return;
            }
        }
    }

    bool translateKey(std::uint16_t code, foundation::Key &key) {
        for (std::uint32_t i = 0; i < std::uint32_t(foundation::Key::_count); i++) {
            if (KEY_CODES[i] == code) {
                key = foundation::Key(i);
                return true;
            }
        }
        return false;
    }

    void keyboardEvent(foundation::PlatformKeyboardEventArgs::EventType type, std::uint16_t code) {
        foundation::PlatformKeyboardEventArgs args;
        args.type = type;

        if (translateKey(code, args.key)) {
            for (auto &index : g_keyboardHandlers) {
                if (index.second(args)) {
                    break;
                }
            }
        }
    }

    bool isDebuggerPresent() {
        kinfo_proc info = {};
        std::size_t size = sizeof(info);
        int request[] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, ::getpid()};
        return ::sysctl(request, 4, &info, &size, nullptr, 0) == 0 && (info.kp_proc.p_flag & P_TRACED) != 0;
    }

    void pointerEvent(foundation::PlatformPointerEventArgs::EventType type, float x, float y, bool left, bool right, int wheel);
}

//--------------------------------------------------------------------------------------------------------------------------------
// Mac Stuff

@interface AppView : MTKView <MTKViewDelegate, NSWindowDelegate>
@end

@implementation AppView
- (void)pointerEvent:(foundation::PlatformPointerEventArgs::EventType)type event:(NSEvent *)event left:(bool)left right:(bool)right wheel:(int)wheel {
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    const CGFloat scale = self.window.backingScaleFactor;
    pointerEvent(type, float(point.x * scale), float((self.bounds.size.height - point.y) * scale), left, right, wheel);
}

- (void)drawInMTKView:(nonnull MTKView *)view {
    static std::chrono::time_point<std::chrono::high_resolution_clock> prevFrameTime = std::chrono::high_resolution_clock::now();
    auto curFrameTime = std::chrono::high_resolution_clock::now();
    float dt = float(std::chrono::duration_cast<std::chrono::milliseconds>(curFrameTime - prevFrameTime).count()) / 1000.0f;

    if (g_updateAndDraw) {
        g_updateAndDraw(dt);
    }

    prevFrameTime = curFrameTime;
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
#if !defined(IS_TESTS)
    const unsigned width = unsigned(size.width);
    const unsigned height = unsigned(size.height);

    if (width != g_width || height != g_height) {
        g_width = width;
        g_height = height;

        for (auto &index : g_resizeHandlers) {
            index.second();
        }
    }
#endif
}

- (BOOL)windowShouldClose:(NSWindow *)sender {
    g_killed = true;
    return NO;
}

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent *)event { return YES; }

- (void)keyDown:(NSEvent *)event {
    if (event.isARepeat == NO) {
        keyboardEvent(foundation::PlatformKeyboardEventArgs::EventType::PRESS, event.keyCode);
    }
}
- (void)keyUp:(NSEvent *)event {
    keyboardEvent(foundation::PlatformKeyboardEventArgs::EventType::RELEASE, event.keyCode);
}

- (void)mouseDown:(NSEvent *)event {
    [self pointerEvent:foundation::PlatformPointerEventArgs::EventType::START event:event left:true right:false wheel:0];
}
- (void)rightMouseDown:(NSEvent *)event {
    [self pointerEvent:foundation::PlatformPointerEventArgs::EventType::START event:event left:false right:true wheel:0];
}
- (void)mouseUp:(NSEvent *)event {
    [self pointerEvent:foundation::PlatformPointerEventArgs::EventType::FINISH event:event left:true right:false wheel:0];
}
- (void)rightMouseUp:(NSEvent *)event {
    [self pointerEvent:foundation::PlatformPointerEventArgs::EventType::FINISH event:event left:false right:true wheel:0];
}
- (void)mouseDragged:(NSEvent *)event {
    [self pointerEvent:foundation::PlatformPointerEventArgs::EventType::MOVE event:event left:true right:false wheel:0];
}
- (void)rightMouseDragged:(NSEvent *)event {
    [self pointerEvent:foundation::PlatformPointerEventArgs::EventType::MOVE event:event left:false right:true wheel:0];
}
- (void)mouseMoved:(NSEvent *)event {
    [self pointerEvent:foundation::PlatformPointerEventArgs::EventType::HOVER event:event left:false right:false wheel:0];
}
- (void)scrollWheel:(NSEvent *)event {
    if (event.deltaY != 0) {
        [self pointerEvent:foundation::PlatformPointerEventArgs::EventType::WHEEL event:event left:false right:false wheel:event.deltaY > 0 ? 1 : -1];
    }
}
@end

//--------------------------------------------------------------------------------------------------------------------------------

namespace {
    void pointerEvent(foundation::PlatformPointerEventArgs::EventType type, float x, float y, bool left, bool right, int wheel) {
        foundation::PlatformPointerEventArgs args = {};
        args.type = type;
        args.pointerID = 0;
        args.coordinateX = x;
        args.coordinateY = y;
        args.flags.wheel = wheel;
        args.flags.isLeftButton = left;
        args.flags.isRightButton = right;

        for (auto &index : g_pointerHandlers) {
            if (index.second(args)) {
                break;
            }
        }

        if (args.coordinateX != x || args.coordinateY != y) { // handler replaced pointer coordinates
            const CGFloat scale = g_window.backingScaleFactor;
            const NSPoint viewPosition = NSMakePoint(args.coordinateX / scale, g_view.bounds.size.height - args.coordinateY / scale);
            const NSPoint screenPosition = [g_window convertPointToScreen:[g_view convertPoint:viewPosition toView:nil]];
            ::CGWarpMouseCursorPosition(CGPointMake(screenPosition.x, NSScreen.screens.firstObject.frame.size.height - screenPosition.y));
        }
    }
}

namespace foundation {
    namespace {
        class FileLoadTask : public AsyncTask {
        public:
            FileLoadTask(std::string &&filePath, std::string &&fallbackPath, util::callback<void(ByteDataPtr &&, std::size_t)> &&completion) : _path(std::move(filePath)), _fallbackPath(std::move(fallbackPath)), _size(0), _completion(std::move(completion)) {}
            ~FileLoadTask() override {}

        public:
            void executeInBackground() override {
                std::fstream fileStream(std::filesystem::exists(_path) ? _path : _fallbackPath, std::ios::binary | std::ios::in | std::ios::ate);

                if (fileStream.is_open() && fileStream.good()) {
                    std::size_t fileSize = std::size_t(fileStream.tellg());
                    _data = std::make_unique<std::uint8_t[]>(fileSize);
                    fileStream.seekg(0);
                    fileStream.read((char *)_data.get(), fileSize);
                    _size = fileSize;
                }
            }
            void executeInMainThread() override {
                _completion(std::move(_data), _size);
            }

        private:
            std::string _path;
            std::string _fallbackPath;
            ByteDataPtr _data;
            std::size_t _size;
            util::callback<void(ByteDataPtr &&, std::size_t)> _completion;
        };

        class FileSaveTask : public AsyncTask {
        public:
            FileSaveTask(std::string &&filePath, const std::uint8_t *data, std::size_t size, util::callback<void(bool)> &&completion) : _path(std::move(filePath)), _data(data, data + size), _result(false), _completion(std::move(completion)) {}
            ~FileSaveTask() override {}

        public:
            void executeInBackground() override {
                std::error_code error;

                if (_data.empty()) {
                    _result = std::filesystem::remove(_path, error);
                }
                else {
                    std::filesystem::create_directories(std::filesystem::path(_path).parent_path(), error);
                    std::fstream fileStream(_path, std::ios::binary | std::ios::out | std::ios::trunc);

                    if (fileStream.is_open() && fileStream.good()) {
                        fileStream.write((const char *)_data.data(), _data.size());
                        _result = fileStream.good();
                    }
                }
            }
            void executeInMainThread() override {
                _completion(_result);
            }

        private:
            std::string _path;
            std::vector<std::uint8_t> _data;
            bool _result;
            util::callback<void(bool)> _completion;
        };
    }
}

namespace foundation {
    MacPlatform::MacPlatform() {
        @autoreleasepool {
            const CGFloat scale = NSScreen.mainScreen.backingScaleFactor;
            const NSRect frame = NSMakeRect(0, 0, APP_WIDTH / scale, APP_HEIGHT / scale);

            g_view = [[AppView alloc] initWithFrame:frame device:MTLCreateSystemDefaultDevice()];
            g_view.paused = YES;
            g_view.enableSetNeedsDisplay = NO;
#if defined(IS_TESTS)
            g_view.autoResizeDrawable = NO;
#endif
            g_view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
            g_view.depthStencilAttachmentTextureUsage = MTLTextureUsageRenderTarget;
            g_view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
            g_view.clearDepth = 0.0f;
            g_view.sampleCount = 1;
            g_view.delegate = g_view;

            g_window = [[NSWindow alloc] initWithContentRect:frame styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];
            g_window.releasedWhenClosed = NO;
            g_window.title = @"Application";
            g_window.contentView = g_view;
            g_window.delegate = g_view;
            g_window.contentMinSize = frame.size;
            g_window.acceptsMouseMovedEvents = YES;

            [g_window center];
            [g_window makeFirstResponder:g_view];
            [g_window makeKeyAndOrderFront:nil];
            [NSApp activateIgnoringOtherApps:YES];

            NSBundle *bundle = [NSBundle mainBundle];
            _dataPath = std::string(bundle.resourcePath.UTF8String) + "/data/";
#if defined(IS_TESTS)
            _writablePath = std::string(bundle.bundlePath.stringByDeletingLastPathComponent.UTF8String) + "/data/";
#else
            NSString *supportPath = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES).firstObject;
            _writablePath = std::string(supportPath.UTF8String) + "/" + bundle.bundleIdentifier.UTF8String + "/";
#endif
        }
    }

    MacPlatform::~MacPlatform() {
        [g_window close];
    }

    void MacPlatform::executeAsync(std::unique_ptr<AsyncTask> &&task) {
        backgroundThreadPush(g_worker, std::move(task));
    }

    bool MacPlatform::isAsyncWorkInProgress() const {
        return g_asyncWorkCount != 0;
    }

    void MacPlatform::loadFile(const char *filePath, util::callback<void(ByteDataPtr &&data, std::size_t size)> &&completion, bool suppressOutput) {
        backgroundThreadPush(g_io, std::make_unique<FileLoadTask>(_dataPath + filePath, _writablePath + filePath, std::move(completion)));
    }
    void MacPlatform::saveFile(const char *filePath, const std::uint8_t *data, std::size_t size, util::callback<void(bool)> &&completion, bool suppressOutput) {
        backgroundThreadPush(g_io, std::make_unique<FileSaveTask>(_writablePath + filePath, data, data ? size : 0, std::move(completion)));
    }

    float MacPlatform::getScreenWidth() const {
        return float(g_width);
    }
    float MacPlatform::getScreenHeight() const {
        return float(g_height);
    }

    void *MacPlatform::attachNativeRenderingContext(void *context) {
        g_view.device = (__bridge id<MTLDevice>)context;
#if defined(IS_TESTS)
        g_view.framebufferOnly = NO;
        g_view.drawableSize = CGSizeMake(TESTING_RENDER_WIDTH, TESTING_RENDER_HEIGHT);
#endif
        return (__bridge void *)g_view;
    }

    void MacPlatform::showCursor() {
        [NSCursor unhide];
    }
    void MacPlatform::hideCursor() {
        [NSCursor hide];
    }

    void MacPlatform::showKeyboard() {}
    void MacPlatform::hideKeyboard() {}

    void MacPlatform::sendEditorMsg(const std::string &msg, const std::string &data) {

    }
    void MacPlatform::editorLoopbackMsg(const std::string &msg, const std::string &data) {
        for (auto &index : g_editorHandlers) {
            if (index.second(msg, data)) {
                break;
            }
        }
    }

    EventHandlerToken MacPlatform::addEditorEventHandler(util::callback<bool(const std::string &, const std::string &)> &&handler, bool setTop) {
        return addHandler(g_editorHandlers, std::move(handler), setTop);
    }
    EventHandlerToken MacPlatform::addKeyboardEventHandler(util::callback<bool(const PlatformKeyboardEventArgs &)> &&handler, bool setTop) {
        return addHandler(g_keyboardHandlers, std::move(handler), setTop);
    }
    EventHandlerToken MacPlatform::addInputEventHandler(util::callback<bool(const char(&utf8char)[4])> &&input, bool setTop) {
        return nullptr;
    }
    EventHandlerToken MacPlatform::addPointerEventHandler(util::callback<bool(const PlatformPointerEventArgs &)> &&handler, bool setTop) {
        return addHandler(g_pointerHandlers, std::move(handler), setTop);
    }
    EventHandlerToken MacPlatform::addGamepadEventHandler(util::callback<bool(const PlatformGamepadEventArgs &)> &&handler, bool setTop) {
        return nullptr;
    }
    EventHandlerToken MacPlatform::addResizeHandler(util::callback<void()> &&handler) {
        return addHandler(g_resizeHandlers, std::move(handler), false);
    }

    void MacPlatform::removeEventHandler(EventHandlerToken token) {
        removeHandler(g_editorHandlers, token);
        removeHandler(g_pointerHandlers, token);
        removeHandler(g_keyboardHandlers, token);
        removeHandler(g_resizeHandlers, token);
    }

    void MacPlatform::sendPointerEvent(const PlatformPointerEventArgs &args) {
        using EventType = PlatformPointerEventArgs::EventType;

        const bool hasButton = args.type == EventType::START || args.type == EventType::MOVE || args.type == EventType::FINISH;
        const bool right = hasButton && args.flags.isRightButton != 0;
        const bool left = hasButton && (args.flags.isLeftButton != 0 || !right);

        pointerEvent(args.type, args.coordinateX, args.coordinateY, left, right, args.flags.wheel);
    }

    void MacPlatform::sendKeyboardEvent(const PlatformKeyboardEventArgs &args) {
        for (auto &index : g_keyboardHandlers) {
            if (index.second(args)) {
                break;
            }
        }
    }

    void MacPlatform::setLoop(util::callback<void(float)> &&updateAndDraw) {
        g_updateAndDraw = std::move(updateAndDraw);
    }

    void MacPlatform::exit(int returnCode) {
        g_killed = true;
        g_returnCode = returnCode;
    }

    void MacPlatform::logMsg(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(_logMutex);

        va_list args;
        va_start(args, fmt);
        vsnprintf(g_buffer, BUFFER_SIZE, fmt, args);
        va_end(args);

        printf("%s\n", g_buffer);
    }

    void MacPlatform::logError(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(_logMutex);

        va_list args;
        va_start(args, fmt);
        vsnprintf(g_buffer, BUFFER_SIZE, fmt, args);
        va_end(args);

        printf("%s\n", g_buffer);

        if (isDebuggerPresent()) {
            __builtin_debugtrap();
        }
    }
}

namespace foundation {
    std::shared_ptr<PlatformInterface> PlatformInterface::instance() {
        std::shared_ptr<PlatformInterface> result;

        if (g_instance.use_count() == 0) {
            g_instance = result = std::make_shared<MacPlatform>();
        }
        else {
            result = g_instance.lock();
        }

        return result;
    }
}

extern "C" void initialize();
extern "C" void deinitialize();

int main(int argc, const char *argv[]) {
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        NSMenu *mainMenu = [NSMenu new];
        NSMenuItem *appItem = [mainMenu addItemWithTitle:@"" action:nil keyEquivalent:@""];
        appItem.submenu = [NSMenu new];
        [appItem.submenu addItemWithTitle:@"Quit" action:@selector(performClose:) keyEquivalent:@"q"];
        NSApp.mainMenu = mainMenu;

        [NSApp finishLaunching];

        initialize();

        g_io.thread = std::thread(&backgroundThreadLoop, std::ref(g_io));
        g_worker.thread = std::thread(&backgroundThreadLoop, std::ref(g_worker));

        std::vector<std::unique_ptr<foundation::AsyncTask>> foregroundQueue;

        while (!g_killed) {
            @autoreleasepool {
                while (NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES]) {
                    [NSApp sendEvent:event];
                }

                {
                    std::unique_lock<std::mutex> guard(g_foregroundMutex);
                    for (auto &task : g_foregroundQueue) {
                        foregroundQueue.emplace_back(std::move(task));
                    }
                    g_foregroundQueue.clear();
                }

                for (auto &task : foregroundQueue) {
                    task->executeInMainThread();
                    g_asyncWorkCount--;
                }
                foregroundQueue.clear();

                [g_view draw];
            }
        }

        backgroundThreadStop(g_io);
        backgroundThreadStop(g_worker);

        deinitialize();
    }
    return g_returnCode;
}

#endif // PLATFORM_MAC
