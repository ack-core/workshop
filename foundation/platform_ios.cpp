
#ifdef PLATFORM_IOS

#include "platform_ios.h"

#include <chrono>
#include <mutex>
#include <fstream>
#include <filesystem>
#include <unordered_map>

#include <UIKit/UIKit.h>
#include <MetalKit/MetalKit.h>

namespace {
    const unsigned BUFFER_SIZE = 65536;
    char g_buffer[BUFFER_SIZE];
    
    std::mutex g_logMutex;
    std::weak_ptr<foundation::PlatformInterface> g_instance;
    std::function<void(float)> g_updateAndDraw;
    
    float g_nativeScreenWidth = 1.0f;
    float g_nativeScreenHeight = 1.0f;
    float g_nativeScreenScale = 1.0f;
    
    MTKView *g_mtkView = nil;
    
    foundation::EventHandlerToken g_tokenCounter = reinterpret_cast<foundation::EventHandlerToken>(0x100);
    std::unordered_map<foundation::EventHandlerToken, std::function<void(const foundation::PlatformTouchEventArgs &)>> g_touchHandlers;
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
        g_nativeScreenScale = UIScreen.mainScreen.nativeScale;
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
    //CGRect frame = UIScreen.mainScreen.bounds;
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
        foundation::PlatformTouchEventArgs args;
        args.type = foundation::PlatformTouchEventArgs::EventType::START;
        args.touchID = std::size_t(item);
        args.coordinateX = [item locationInView:nil].x * g_nativeScreenScale;
        args.coordinateY = [item locationInView:nil].y * g_nativeScreenScale;
        
        for (auto &index : g_touchHandlers) {
            index.second(args);
        }
    }
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event {
    for (UITouch *item in touches) {
        foundation::PlatformTouchEventArgs args;
        args.type = foundation::PlatformTouchEventArgs::EventType::MOVE;
        args.touchID = std::size_t(item);
        args.coordinateX = [item locationInView:nil].x * g_nativeScreenScale;
        args.coordinateY = [item locationInView:nil].y * g_nativeScreenScale;
        
        for (auto &index : g_touchHandlers) {
            index.second(args);
        }
    }
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event {
    for (UITouch *item in touches) {
        foundation::PlatformTouchEventArgs args;
        args.type = foundation::PlatformTouchEventArgs::EventType::FINISH;
        args.touchID = std::size_t(item);
        args.coordinateX = [item locationInView:nil].x * g_nativeScreenScale;
        args.coordinateY = [item locationInView:nil].y * g_nativeScreenScale;
        
        for (auto &index : g_touchHandlers) {
            index.second(args);
        }
    }
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event {
    for (UITouch *item in touches) {
        foundation::PlatformTouchEventArgs args;
        args.type = foundation::PlatformTouchEventArgs::EventType::CANCEL;
        args.touchID = std::size_t(item);
        args.coordinateX = [item locationInView:nil].x * g_nativeScreenScale;
        args.coordinateY = [item locationInView:nil].y * g_nativeScreenScale;
        
        for (auto &index : g_touchHandlers) {
            index.second(args);
        }
    }
}
@end

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
    
    }

    std::vector<PlatformFileEntry> IOSPlatform::formFileList(const char *dirPath) {
        std::string fullPath = _executableDirectoryPath + "/" + dirPath;
        std::vector<PlatformFileEntry> result;

        for (auto &entry : std::filesystem::directory_iterator(fullPath)) {
            result.emplace_back(PlatformFileEntry{ entry.path().generic_u8string(), entry.is_directory()});
        }

        return result;
    }
    bool IOSPlatform::loadFile(const char *filePath, std::unique_ptr<unsigned char[]> &data, std::size_t &size) {
        std::string fullPath = _executableDirectoryPath + "/" + filePath;
        std::fstream fileStream(fullPath, std::ios::binary | std::ios::in | std::ios::ate);

        if (fileStream.is_open() && fileStream.good()) {
            std::size_t fileSize = std::size_t(fileStream.tellg());
            data = std::make_unique<unsigned char[]>(fileSize);
            fileStream.seekg(0);
            fileStream.read((char *)data.get(), fileSize);
            size = fileSize;

            return true;
        }

        return false;
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

    EventHandlerToken IOSPlatform::addKeyboardEventHandler(std::function<void(const PlatformKeyboardEventArgs &)> &&handler) {
        return nullptr;
    }
    
    EventHandlerToken IOSPlatform::addInputEventHandler(std::function<void(const char(&utf8char)[4])> &&input, std::function<void()> &&backspace) {
        return nullptr;
    }
    
    EventHandlerToken IOSPlatform::addMouseEventHandler(std::function<void(const PlatformMouseEventArgs &)> &&handler) {
        return nullptr;
    }
    
    EventHandlerToken IOSPlatform::addTouchEventHandler(std::function<void(const PlatformTouchEventArgs &)> &&handler) {
        EventHandlerToken token = g_tokenCounter++;
        g_touchHandlers.emplace(token, std::move(handler));
        return nullptr;
    }
    
    EventHandlerToken IOSPlatform::addGamepadEventHandler(std::function<void(const PlatformGamepadEventArgs &)> &&handler) {
        return nullptr;
    }

    void IOSPlatform::removeEventHandler(EventHandlerToken token) {
        g_touchHandlers.erase(token);
    }

    void IOSPlatform::run(std::function<void(float)> &&updateAndDraw) {
        g_updateAndDraw = std::move(updateAndDraw);
        
        @autoreleasepool {
            char *argv[] = {0};
            UIApplicationMain(0, argv, nil, NSStringFromClass([AppDelegate class]));
        }
    }
    
    void IOSPlatform::exit() {
    
    }

    void IOSPlatform::logMsg(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(g_logMutex);

        va_list args;
        va_start(args, fmt);
        vsnprintf(g_buffer, BUFFER_SIZE, fmt, args);
        va_end(args);

        printf("%s\n", g_buffer);
    }
    
    void IOSPlatform::logError(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(g_logMutex);

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

#endif // PLATFORM_IOS

