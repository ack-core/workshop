
#include "interfaces.h"
#include "uw_platform.h"

#define NOMINMAX

#include <d3d11_1.h>
#include <agile.h>
#include <codecvt>
#include <ppltasks.h>

#include <chrono>
#include <list>
#include <mutex>
#include <algorithm>

#include <cassert>

// TODO: thread safety
//       saving file
//       windows size and resizing

namespace native {
    namespace {
        struct KeyboardCallbacksEntry {
            const void *handle;
            std::function<void(PlatformKeyboardEventArgs &)> keyboardDown;
            std::function<void(PlatformKeyboardEventArgs &)> keyboardUp;
        };

        struct MouseCallbacksEntry {
            const void *handle;
            std::function<void(PlatformMouseEventArgs &)> mousePress;
            std::function<void(PlatformMouseEventArgs &)> mouseMove;
            std::function<void(PlatformMouseEventArgs &)> mouseRelease;
        };

        std::size_t callbacksIdSource = 0;
        std::list<KeyboardCallbacksEntry> g_keyboardCallbacks;
        std::list<MouseCallbacksEntry> g_mouseCallbacks;

        std::function<void(float)> g_updateAndDraw = nullptr;
        bool g_platformCreated = false;

        Platform::Agile<Windows::UI::Core::CoreWindow ^> g_window;
        Platform::Agile<Windows::UI::Core::CoreCursor ^> g_cursor;

        char g_logMessageBuffer[65536];
        std::mutex g_logMutex;

        _CrtMemState g_startMemState;
        
        bool convertKey(Windows::System::VirtualKey key, std::uint32_t &target) {
            if (key >= Windows::System::VirtualKey::A && key <= Windows::System::VirtualKey::Z) {
                target = std::uint32_t(key) - std::uint32_t(Windows::System::VirtualKey::A);
                return true;
            }

            return false;
        }
    }

    namespace uwp {
        using namespace Platform;
        using namespace Windows::Foundation;
        using namespace Windows::UI::Core;
        using namespace Windows::Storage;
        using namespace Windows::ApplicationModel;
        using namespace Windows::ApplicationModel::Core;
        using namespace Windows::ApplicationModel::Activation;
        using namespace Windows::UI::Popups;
        using namespace Windows::Graphics::Display;
        
        float nativeCoord(DisplayInformation ^curDisplayInfo, float logicalCoord) {
            return std::round(logicalCoord * curDisplayInfo->LogicalDpi / 96.0f + 0.5f);
        }

        ref class RenderAppView sealed : public IFrameworkView {
            friend ref class RenderAppViewSource;
            
        public:
            virtual void Initialize(CoreApplicationView ^applicationView) {
                applicationView->Activated += ref new TypedEventHandler<CoreApplicationView ^, IActivatedEventArgs ^>(this, &RenderAppView::onActivated);
                CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs ^>(this, &RenderAppView::onSuspending);
                CoreApplication::Resuming += ref new EventHandler<Object ^>(this, &RenderAppView::onResuming);
            }
            virtual void SetWindow(CoreWindow ^window) {
                g_window = window;
                g_cursor = window->PointerCursor;
                
                window->SizeChanged += ref new TypedEventHandler <CoreWindow ^, WindowSizeChangedEventArgs ^>(this, &RenderAppView::onSizeChanged);
                window->VisibilityChanged += ref new TypedEventHandler <CoreWindow ^, VisibilityChangedEventArgs ^>(this, &RenderAppView::onVisibilityChanged);
                window->Closed += ref new TypedEventHandler <CoreWindow ^, CoreWindowEventArgs ^>(this, &RenderAppView::onClosed);

                window->PointerPressed += ref new TypedEventHandler <CoreWindow^, PointerEventArgs^>(this, &RenderAppView::OnPointerPressed);
                window->PointerMoved += ref new TypedEventHandler <CoreWindow^, PointerEventArgs^>(this, &RenderAppView::OnPointerMoved);
                window->PointerReleased += ref new TypedEventHandler <CoreWindow^, PointerEventArgs^>(this, &RenderAppView::OnPointerReleased);
                window->PointerWheelChanged += ref new TypedEventHandler<CoreWindow ^, PointerEventArgs ^>(this, &RenderAppView::OnPointerWheelChanged);
                window->KeyDown += ref new TypedEventHandler<CoreWindow ^, KeyEventArgs ^>(this, &RenderAppView::OnKeyDown);
                window->KeyUp += ref new TypedEventHandler<CoreWindow ^, KeyEventArgs ^>(this, &RenderAppView::OnKeyUp);

                DisplayInformation ^curDisplayInfo = DisplayInformation::GetForCurrentView();
            }
            virtual void Load(Platform::String ^entryPoint) {

            }
            virtual void Run() {
                static std::chrono::time_point<std::chrono::high_resolution_clock> prevFrameTime = std::chrono::high_resolution_clock::now();
                
                _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
                _CrtMemCheckpoint(&g_startMemState);

                while (!_closed) {
                    if (_visible) {
                        CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
                        if (g_updateAndDraw) {
                            auto curFrameTime = std::chrono::high_resolution_clock::now();
                            float dt = float(std::chrono::duration_cast<std::chrono::microseconds>(curFrameTime - prevFrameTime).count()) / 1000.0f;
                            g_updateAndDraw(dt); // ms
                            prevFrameTime = curFrameTime;
                        }
                    }
                    else {
                        CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
                    }
                }

                _CrtMemDumpAllObjectsSince(&g_startMemState);
            }
            virtual void Uninitialize() {

            }

        private:
            bool _closed = false;
            bool _visible = true;

            RenderAppView() {}

            void onActivated(Windows::ApplicationModel::Core::CoreApplicationView ^applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs ^args) {
                Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->PreferredLaunchWindowingMode = Windows::UI::ViewManagement::ApplicationViewWindowingMode::PreferredLaunchViewSize;
                Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->PreferredLaunchViewSize = Size {1280, 720};
                CoreWindow::GetForCurrentThread()->Activate();                
            }
            void onSuspending(Object ^sender, Windows::ApplicationModel::SuspendingEventArgs ^args) {
                
            }
            void onResuming(Object ^sender, Object ^args) {

            }

            void onSizeChanged(Windows::UI::Core::CoreWindow ^window, Windows::UI::Core::WindowSizeChangedEventArgs ^args) {

            }
            void onVisibilityChanged(Windows::UI::Core::CoreWindow ^window, Windows::UI::Core::VisibilityChangedEventArgs ^args) {
                _visible = args->Visible;
            }
            void onClosed(Windows::UI::Core::CoreWindow ^window, Windows::UI::Core::CoreWindowEventArgs ^args) {
                _closed = true;
            }

            void OnPointerPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args) {
                PlatformMouseEventArgs mouseEvent;

                sender->SetPointerCapture();
                mouseEvent.coordinateX = sender->PointerPosition.X - sender->Bounds.X;
                mouseEvent.coordinateY = sender->PointerPosition.Y - sender->Bounds.Y;
                mouseEvent.isLeftButtonPressed = args->CurrentPoint->Properties->IsLeftButtonPressed;
                mouseEvent.isRightButtonPressed = args->CurrentPoint->Properties->IsRightButtonPressed;

                for (const auto &entry : g_mouseCallbacks) {
                    if (entry.mousePress) {
                        entry.mousePress(mouseEvent);
                    }
                }
            }
            void OnPointerMoved(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args) {
                PlatformMouseEventArgs mouseEvent;

                mouseEvent.coordinateX = sender->PointerPosition.X - sender->Bounds.X;
                mouseEvent.coordinateY = sender->PointerPosition.Y - sender->Bounds.Y;
                mouseEvent.isLeftButtonPressed = args->CurrentPoint->Properties->IsLeftButtonPressed;
                mouseEvent.isRightButtonPressed = args->CurrentPoint->Properties->IsRightButtonPressed;

                for (const auto &entry : g_mouseCallbacks) {
                    if (entry.mouseMove) {
                        entry.mouseMove(mouseEvent);
                    }
                }

                sender->PointerPosition = Windows::Foundation::Point {mouseEvent.coordinateX + sender->Bounds.X, mouseEvent.coordinateY + sender->Bounds.Y};
            }
            void OnPointerReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args) {
                PlatformMouseEventArgs mouseEvent;

                sender->ReleasePointerCapture();
                mouseEvent.coordinateX = sender->PointerPosition.X - sender->Bounds.X;
                mouseEvent.coordinateY = sender->PointerPosition.Y - sender->Bounds.Y;
                mouseEvent.isLeftButtonPressed = args->CurrentPoint->Properties->IsLeftButtonPressed;
                mouseEvent.isRightButtonPressed = args->CurrentPoint->Properties->IsRightButtonPressed;

                for (const auto &entry : g_mouseCallbacks) {
                    if (entry.mouseRelease) {
                        entry.mouseRelease(mouseEvent);
                    }
                }
            }
            void OnPointerWheelChanged(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::PointerEventArgs ^args) {

            }
            void OnKeyDown(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::KeyEventArgs ^args) {
                std::uint32_t key;

                if (convertKey(args->VirtualKey, key)) {
                    PlatformKeyboardEventArgs keyboardEvent {key};

                    for (const auto &entry : g_keyboardCallbacks) {
                        if (entry.keyboardDown) {
                            entry.keyboardDown(keyboardEvent);
                        }
                    }
                }
            }
            void OnKeyUp(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::KeyEventArgs ^args) {
                std::uint32_t key;

                if (convertKey(args->VirtualKey, key)) {
                    PlatformKeyboardEventArgs keyboardEvent {key};

                    for (const auto &entry : g_keyboardCallbacks) {
                        if (entry.keyboardUp) {
                            entry.keyboardUp(keyboardEvent);
                        }
                    }
                }
            }
        };

        ref class RenderAppViewSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource {
        public:
            RenderAppViewSource() {}
            virtual Windows::ApplicationModel::Core::IFrameworkView ^CreateView() {
                return ref new RenderAppView();
            }
        };
    }

    std::shared_ptr<PlatformInterface> PlatformInterface::makeInstance() {
        assert(g_platformCreated == false);
        return std::make_shared<UWPlatform>();
    }

    UWPlatform::UWPlatform() {
        g_platformCreated = true;
    }

    UWPlatform::~UWPlatform() {

    }

    void UWPlatform::logInfo(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(g_logMutex);

        va_list args;
        va_start(args, fmt);
        vsprintf_s(g_logMessageBuffer, fmt, args);
        va_end(args);

        OutputDebugStringA("[Inf] ");
        OutputDebugStringA(g_logMessageBuffer);
        OutputDebugStringA("\n");        
    }

    void UWPlatform::logWarning(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(g_logMutex);

        va_list args;
        va_start(args, fmt);
        vsprintf_s(g_logMessageBuffer, fmt, args);
        va_end(args);

        OutputDebugStringA("[Wrn] ");
        OutputDebugStringA(g_logMessageBuffer);
        OutputDebugStringA("\n");
    }

    void UWPlatform::logError(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(g_logMutex);

        va_list args;
        va_start(args, fmt);
        vsprintf_s(g_logMessageBuffer, fmt, args);
        va_end(args);
        
        OutputDebugStringA("[Err] ");
        OutputDebugStringA(g_logMessageBuffer);
        OutputDebugStringA("\n");        
    }

    std::vector<std::string> UWPlatform::formFileList(const char *dirPath) const {
        std::vector<std::string> result;
        std::wstring dirPathW = std::wstring_convert<std::codecvt_utf8_utf16 <wchar_t>>().from_bytes(dirPath);

        try {
            Windows::Storage::StorageFolder ^fldr = concurrency::create_task(Windows::ApplicationModel::Package::Current->InstalledLocation->GetFolderAsync(ref new Platform::String(dirPathW.c_str()))).get();
            Windows::Foundation::Collections::IVectorView<Windows::Storage::StorageFile ^> ^files = concurrency::create_task(fldr->GetFilesAsync()).get();

            for (unsigned int i = 0; i < files->Size; i++) {
                Windows::Storage::StorageFile ^curFile = files->GetAt(i);

                std::string curFilePath = std::string(dirPath) + "/" + std::wstring_convert<std::codecvt_utf8 <wchar_t>>().to_bytes(curFile->Name->Data());
                result.emplace_back(std::move(curFilePath));
            }
        }
        catch (Platform::Exception^) {}
        return result;
    }

    bool UWPlatform::loadFile(const char *filePath, std::unique_ptr<unsigned char[]> &data, std::size_t &size) const {
        std::wstring filePathW = std::wstring_convert<std::codecvt_utf8_utf16 <wchar_t>>().from_bytes(filePath);
        std::replace(filePathW.begin(), filePathW.end(), L'/', L'\\');

        try {
            auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;
            filePathW = std::wstring(folder->Path->Data()) + std::wstring(L"\\") + filePathW;

            CREATEFILE2_EXTENDED_PARAMETERS params = {0};
            params.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
            params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
            params.dwSecurityQosFlags = SECURITY_ANONYMOUS;
            HANDLE file = CreateFile2(filePathW.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING, &params);
            
            LARGE_INTEGER tmpsize;
            GetFileSizeEx(file, &tmpsize);
            
            auto ptr = new unsigned char[tmpsize.LowPart];
            ReadFile(file, ptr, tmpsize.LowPart, nullptr, nullptr);
            CloseHandle(file);

            data.reset(ptr);
            size = tmpsize.LowPart;
            return true;
        }
        catch (Platform::Exception ^) {}
        return false;
    }

    float UWPlatform::getNativeScreenWidth() const {
        return 1280;
    }

    float UWPlatform::getNativeScreenHeight() const {
        return 720;
    }

    void *UWPlatform::setNativeRenderingContext(void *context) {
        IDXGISwapChain1 *swapChain = nullptr;
        
        if (g_window != nullptr) {
            ID3D11Device1 *device = reinterpret_cast<ID3D11Device1 *>(context);
            IDXGIAdapter  *adapter = nullptr;
            IDXGIDevice1  *dxgiDevice = nullptr;
            IDXGIFactory2 *dxgiFactory = nullptr;
            device->QueryInterface(__uuidof(IDXGIDevice1), (void **)&dxgiDevice);

            dxgiDevice->SetMaximumFrameLatency(1);
            dxgiDevice->GetAdapter(&adapter);
            adapter->GetParent(__uuidof(IDXGIFactory2), (void **)&dxgiFactory);
            adapter->Release();

            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};

            swapChainDesc.Width = 1280;
            swapChainDesc.Height = 720;
            swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            swapChainDesc.Stereo = false;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = 2;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            swapChainDesc.Flags = 0;
            swapChainDesc.Scaling = DXGI_SCALING_NONE;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

            dxgiFactory->CreateSwapChainForCoreWindow(device, reinterpret_cast<IUnknown *>(g_window.Get()), &swapChainDesc, nullptr, &swapChain);
            dxgiFactory->Release();
            dxgiDevice->Release();
        }

        return swapChain;
    }

    PlatformCallbackToken UWPlatform::addKeyboardCallbacks(
        std::function<void(const PlatformKeyboardEventArgs &)> &&down,
        std::function<void(const PlatformKeyboardEventArgs &)> &&up
    ) {
        PlatformCallbackToken result {reinterpret_cast<PlatformCallbackToken>(callbacksIdSource++)};
        g_keyboardCallbacks.emplace_front(KeyboardCallbacksEntry {result, std::move(down), std::move(up)});
        return result;
    }

    PlatformCallbackToken UWPlatform::addMouseCallbacks(
        std::function<void(const PlatformMouseEventArgs &)> &&press,
        std::function<void(const PlatformMouseEventArgs &)> &&move,
        std::function<void(const PlatformMouseEventArgs &)> &&release
    ) {
        PlatformCallbackToken result {reinterpret_cast<PlatformCallbackToken>(callbacksIdSource++)};
        g_mouseCallbacks.emplace_front(MouseCallbacksEntry {result, std::move(press), std::move(move), std::move(release)});
        return result;
    }

    void UWPlatform::removeCallbacks(PlatformCallbackToken& token) {
        for (auto index = std::begin(g_keyboardCallbacks); index != std::end(g_keyboardCallbacks); ++index) {
            if (index->handle == token) {
                g_keyboardCallbacks.erase(index);
                return;
            }
        }

        for (auto index = std::begin(g_mouseCallbacks); index != std::end(g_mouseCallbacks); ++index) {
            if (index->handle == token) {
                g_mouseCallbacks.erase(index);
                return;
            }
        }
    }

    void UWPlatform::showCursor() {
        if (g_window != nullptr) {
            g_window->PointerCursor = g_cursor.Get();
        }
    }

    void UWPlatform::hideCursor() {
        if (g_window != nullptr) {
            g_window->PointerCursor = nullptr;
        }
    }
    
    void UWPlatform::run(std::function<void(float)> &&updateAndDraw) {
        g_updateAndDraw = std::move(updateAndDraw);

        auto source = ref new uwp::RenderAppViewSource();
        Windows::ApplicationModel::Core::CoreApplication::Run(source);
    }
}