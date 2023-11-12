
#ifdef PLATFORM_IOS

#include "platform_ios.h"

#include <chrono>
#include <fstream>
#include <filesystem>
#include <unordered_map>

#include <UIKit/UIKit.h>
#include <MetalKit/MetalKit.h>

namespace {
    const unsigned BUFFER_SIZE = 65536;
    char g_buffer[BUFFER_SIZE];
    
    std::weak_ptr<foundation::PlatformInterface> g_instance;
    util::callback<void(float)> g_updateAndDraw;
    
    float g_nativeScreenWidth = 1.0f;
    float g_nativeScreenHeight = 1.0f;
    float g_nativeScreenScale = 1.0f;
    
    MTKView *g_mtkView = nil;
    
    foundation::EventHandlerToken g_tokenCounter = reinterpret_cast<foundation::EventHandlerToken>(0x100);
    
    struct Handler {
        foundation::EventHandlerToken token;
        util::callback<bool(const foundation::PlatformPointerEventArgs &)> handler;
    };
    
    std::list<Handler> g_pointerHandlers;
    std::vector<std::unique_ptr<foundation::AsyncTask>> g_foregroundQueue;
    std::mutex g_foregroundMutex;
    
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
        
        while (g_instance.use_count() != 0) {
            std::unique_ptr<foundation::AsyncTask> task = nullptr;
            
            {
                std::unique_lock<std::mutex> guard(threadData.mutex);
                
                while (threadData.queue.empty()) {
                    threadData.notifier.wait(guard);
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
    
}

//--------------------------------------------------------------------------------------------------------------------------------
// Creepy IOS Stuff

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@end

@interface ViewDelegate : NSObject <MTKViewDelegate>
@end

@interface RootViewController : UIViewController<UIKeyInput>
@end

@implementation AppDelegate
{
    UIWindow *_window;
    RootViewController *_controller;
}
- (instancetype)init {
    if (self = [super init]) {
    
    }
    return self;
}
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    if (application.applicationState != UIApplicationStateBackground) {
        _controller = [[RootViewController alloc] init];
        _window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
        _window.rootViewController = _controller;
        
        CGRect frame = UIScreen.mainScreen.bounds;

        g_nativeScreenScale = UIScreen.mainScreen.scale;
        g_nativeScreenWidth = frame.size.width * g_nativeScreenScale;
        g_nativeScreenHeight = frame.size.height * g_nativeScreenScale;
        
        [_window makeKeyAndVisible];
    }
    return YES;
}
@end

@implementation ViewDelegate
-(nonnull instancetype)init {
	self = [super init];
	if (self) {
        
	}
	
	return self;
}

- (void)drawInMTKView:(nonnull MTKView *)view {
    static std::chrono::time_point<std::chrono::high_resolution_clock> prevFrameTime = std::chrono::high_resolution_clock::now();
    auto curFrameTime = std::chrono::high_resolution_clock::now();
    float dt = float(std::chrono::duration_cast<std::chrono::milliseconds>(curFrameTime - prevFrameTime).count()) / 1000.0f;
    
    if (g_updateAndDraw) {
        static std::vector<std::unique_ptr<foundation::AsyncTask>> foregroundQueue;
        
        {
            std::unique_lock<std::mutex> guard(g_foregroundMutex);
            for (auto &task : g_foregroundQueue) {
                foregroundQueue.emplace_back(std::move(task));
            }
            g_foregroundQueue.clear();
        }
        
        for (auto &task : foregroundQueue) {
            task->executeInMainThread();
        }
        foregroundQueue.clear();
        
        @autoreleasepool {
            g_updateAndDraw(dt);
        }
    }
    
    prevFrameTime = curFrameTime;
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {

}
@end

@implementation RootViewController
{
    ViewDelegate *_delegate;
}
- (void)loadView
{
    g_mtkView = [[MTKView alloc] init];
    self.view = g_mtkView;
}
- (void)viewDidLoad {
    [super viewDidLoad];
    _delegate = [[ViewDelegate alloc] init];
    
	g_mtkView.multipleTouchEnabled = YES;
	g_mtkView.preferredFramesPerSecond = 30;
    g_mtkView.autoResizeDrawable = NO;
	g_mtkView.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    g_mtkView.depthStencilAttachmentTextureUsage = MTLTextureUsageRenderTarget;
	g_mtkView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    g_mtkView.clearDepth = 0.0f;
    g_mtkView.sampleCount = 1;
	g_mtkView.delegate = _delegate;
}
- (BOOL)hasText { return NO; }
- (BOOL)canBecomeFirstResponder { return NO; }
- (void)insertText:(NSString *)text {
    
}
- (void)deleteBackward {
    
}
- (BOOL)shouldAutorotate { return NO; }
- (BOOL)prefersStatusBarHidden { return YES; }
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event {
    for (UITouch *item in touches) {
        foundation::PlatformPointerEventArgs args;
        args.type = foundation::PlatformPointerEventArgs::EventType::START;
        args.pointerID = std::size_t(item);
        args.coordinateX = [item locationInView:nil].x * g_nativeScreenScale;
        args.coordinateY = [item locationInView:nil].y * g_nativeScreenScale;
        
        for (auto &index : g_pointerHandlers) {
            if (index.handler(args)) {
                break;
            }
        }
    }
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event {
    for (UITouch *item in touches) {
        foundation::PlatformPointerEventArgs args;
        args.type = foundation::PlatformPointerEventArgs::EventType::MOVE;
        args.pointerID = std::size_t(item);
        args.coordinateX = [item locationInView:nil].x * g_nativeScreenScale;
        args.coordinateY = [item locationInView:nil].y * g_nativeScreenScale;
        
        for (auto &index : g_pointerHandlers) {
            if (index.handler(args)) {
                break;
            }
        }
    }
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event {
    for (UITouch *item in touches) {
        foundation::PlatformPointerEventArgs args;
        args.type = foundation::PlatformPointerEventArgs::EventType::FINISH;
        args.pointerID = std::size_t(item);
        args.coordinateX = [item locationInView:nil].x * g_nativeScreenScale;
        args.coordinateY = [item locationInView:nil].y * g_nativeScreenScale;
        
        for (auto &index : g_pointerHandlers) {
            if (index.handler(args)) {
                break;
            }
        }
    }
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event {
    for (UITouch *item in touches) {
        foundation::PlatformPointerEventArgs args;
        args.type = foundation::PlatformPointerEventArgs::EventType::CANCEL;
        args.pointerID = std::size_t(item);
        args.coordinateX = [item locationInView:nil].x * g_nativeScreenScale;
        args.coordinateY = [item locationInView:nil].y * g_nativeScreenScale;
        
        for (auto &index : g_pointerHandlers) {
            if (index.handler(args)) {
                break;
            }
        }
    }
}
@end

//--------------------------------------------------------------------------------------------------------------------------------

namespace foundation {
    namespace {
        class FileListTask : public AsyncTask {
        public:
            FileListTask(std::string &&dirPath, util::callback<void(const std::vector<PlatformFileEntry> &)> &&completion) : _path(std::move(dirPath)), _completion(std::move(completion)) {}
            ~FileListTask() {}
            
        public:
            void executeInBackground() override {
                for (auto &entry : std::filesystem::directory_iterator(_path)) {
                    _entries.emplace_back(PlatformFileEntry{ entry.path().generic_string(), entry.is_directory()});
                }
            }
            void executeInMainThread() override {
                _completion(_entries);
            }
            
        private:
            std::string _path;
            std::vector<PlatformFileEntry> _entries;
            util::callback<void(const std::vector<PlatformFileEntry> &)> _completion;
        };
        
        class FileLoadTask : public AsyncTask {
        public:
            FileLoadTask(const char *filePath, util::callback<void(std::unique_ptr<std::uint8_t[]> &&, std::size_t)> &&completion) : _path(filePath), _size(0), _completion(std::move(completion)) {}
            ~FileLoadTask() {}
            
        public:
            void executeInBackground() override {
                std::fstream fileStream(_path, std::ios::binary | std::ios::in | std::ios::ate);
                
                if (fileStream.is_open() && fileStream.good()) {
                    std::size_t fileSize = std::size_t(fileStream.tellg());
                    _data = std::make_unique<unsigned char[]>(fileSize);
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
            std::unique_ptr<std::uint8_t[]> _data;
            std::size_t _size;
            util::callback<void(std::unique_ptr<std::uint8_t[]> &&, std::size_t)> _completion;
        };
    }
}

//--------------------------------------------------------------------------------------------------------------------------------

namespace foundation {
    IOSPlatform::IOSPlatform() {
    	@autoreleasepool {
            CGRect frame = UIScreen.mainScreen.bounds;
            g_nativeScreenScale = UIScreen.mainScreen.nativeScale;
            g_nativeScreenWidth = frame.size.height * g_nativeScreenScale; // at this time screen orientation isn't applied
            g_nativeScreenHeight = frame.size.width * g_nativeScreenScale;
            
            _executableDirectoryPath = [[[NSBundle mainBundle] bundlePath] cStringUsingEncoding:NSUTF8StringEncoding];
        }
    }
    
    IOSPlatform::~IOSPlatform() {
        g_io.notifier.notify_one();
        g_io.thread.join();
        g_worker.notifier.notify_one();
        g_worker.thread.join();
    }
    
    void IOSPlatform::executeAsync(std::unique_ptr<AsyncTask> &&task) {
        {
            std::lock_guard<std::mutex> guard(g_worker.mutex);
            g_worker.queue.emplace_back(std::move(task));
        }
        g_worker.notifier.notify_one();
    }
    
//    void IOSPlatform::formFileList(const char *dirPath, util::callback<void(const std::vector<PlatformFileEntry> &)> &&completion) {
//        {
//            std::lock_guard<std::mutex> guard(g_io.mutex);
//            g_io.queue.emplace_back(std::make_unique<FileListTask>((_executableDirectoryPath + "/" + dirPath).data(), std::move(completion)));
//        }
//        g_io.notifier.notify_one();
//    }
    
    void IOSPlatform::loadFile(const char *filePath, util::callback<void(std::unique_ptr<std::uint8_t[]> &&data, std::size_t size)> &&completion) {
        {
            std::lock_guard<std::mutex> guard(g_io.mutex);
            g_io.queue.emplace_back(std::make_unique<FileLoadTask>((_executableDirectoryPath + "/" + filePath).data(), std::move(completion)));
        }
        g_io.notifier.notify_one();
    }
        
    float IOSPlatform::getScreenWidth() const {
        return g_nativeScreenWidth;
    }
    float IOSPlatform::getScreenHeight() const {
        return g_nativeScreenHeight;
    }
    
    void *IOSPlatform::attachNativeRenderingContext(void *context) {
        g_mtkView.device = (__bridge id<MTLDevice>)context;
        return (__bridge void *)g_mtkView;
    }
    
    void IOSPlatform::showCursor() {}
    void IOSPlatform::hideCursor() {}
    void IOSPlatform::showKeyboard() {}
    void IOSPlatform::hideKeyboard() {}
    
    EventHandlerToken IOSPlatform::addKeyboardEventHandler(util::callback<void(const PlatformKeyboardEventArgs &)> &&handler) {
        return nullptr;
    }
    
    EventHandlerToken IOSPlatform::addInputEventHandler(util::callback<void(const char(&utf8char)[4])> &&input, util::callback<void()> &&backspace) {
        return nullptr;
    }
    
    EventHandlerToken IOSPlatform::addPointerEventHandler(util::callback<bool(const PlatformPointerEventArgs &)> &&handler) {
        EventHandlerToken token = g_tokenCounter++;
        g_pointerHandlers.emplace_back(Handler{token, std::move(handler)});
        return token;
    }
    
    EventHandlerToken IOSPlatform::addGamepadEventHandler(util::callback<void(const PlatformGamepadEventArgs &)> &&handler) {
        return nullptr;
    }
    
    void IOSPlatform::removeEventHandler(EventHandlerToken token) {
        for (auto index = g_pointerHandlers.begin(); index != g_pointerHandlers.end(); ++index) {
            if (index->token == token) {
                g_pointerHandlers.erase(index);
                return;
            }
        }
    }
    
    void IOSPlatform::setLoop(util::callback<void(float)> &&updateAndDraw) {
        g_updateAndDraw = std::move(updateAndDraw);
    }
    
    void IOSPlatform::exit() {
    
    }
    
    void IOSPlatform::logMsg(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(_logMutex);
        
        va_list args;
        va_start(args, fmt);
        vsnprintf(g_buffer, BUFFER_SIZE, fmt, args);
        va_end(args);
        
        printf("%s\n", g_buffer);
    }
    
    void IOSPlatform::logError(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(_logMutex);
        
        va_list args;
        va_start(args, fmt);
        vsnprintf(g_buffer, BUFFER_SIZE, fmt, args);
        va_end(args);
        
        printf("%s\n", g_buffer);
        __builtin_debugtrap();
    }
}

namespace foundation {
    std::shared_ptr<PlatformInterface> PlatformInterface::instance() {
        std::shared_ptr<PlatformInterface> result;
        
        if (g_instance.use_count() == 0) {
            g_instance = result = std::make_shared<IOSPlatform>();
        }
        else {
            result = g_instance.lock();
        }
        
        return result;
    }
}

extern "C" void initialize();
extern "C" void deinitialize();

int main(int argc, const char * argv[]) {
    initialize();
    
    g_io.thread = std::thread(&backgroundThreadLoop, std::ref(g_io));
    g_worker.thread = std::thread(&backgroundThreadLoop, std::ref(g_worker));
    
    @autoreleasepool {
        char *argv[] = {0};
        UIApplicationMain(0, argv, nil, NSStringFromClass([AppDelegate class]));
    }
    
    deinitialize();
}

#endif // PLATFORM_IOS

