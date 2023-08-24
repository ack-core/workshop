
#include "interfaces.h"
#include "w32_platform.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <d3d11_1.h>
#include <codecvt>

#include <chrono>
#include <list>
#include <mutex>
#include <algorithm>
#include <fstream>

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

        HINSTANCE  g_hinst;
        HWND       g_window;
        MSG        g_message;
        bool       g_killed = false;
        bool       g_platformCreated = false;
        
        constexpr unsigned APP_WIDTH = 1280;
        constexpr unsigned APP_HEIGHT = 720;

        char g_logMessageBuffer[65536];
        std::mutex g_logMutex;

        _CrtMemState g_startMemState;

        //bool convertKey(Windows::System::VirtualKey key, std::uint32_t &target) {
        //    if (key >= Windows::System::VirtualKey::A && key <= Windows::System::VirtualKey::Z) {
        //        target = std::uint32_t(key) - std::uint32_t(Windows::System::VirtualKey::A);
        //        return true;
        //    }
        //}

        LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
            static bool mouseCaptured = false;

            if (uMsg == WM_CLOSE) {
                g_killed = true;
            }
            if (uMsg == WM_LBUTTONDOWN) {
                mouseCaptured = true;
                ::SetCapture(hwnd);
                
                PlatformMouseEventArgs mouseEvent;

                mouseEvent.coordinateX = float(short(LOWORD(lParam)));
                mouseEvent.coordinateY = float(short(HIWORD(lParam)));

                for (const auto &entry : g_mouseCallbacks) {
                    if (entry.mousePress) {
                        entry.mousePress(mouseEvent);
                    }
                }
            }
            if (uMsg == WM_LBUTTONUP) {
                mouseCaptured = false;

                PlatformMouseEventArgs mouseEvent;

                mouseEvent.coordinateX = float(short(LOWORD(lParam)));
                mouseEvent.coordinateY = float(short(HIWORD(lParam)));

                for (const auto &entry : g_mouseCallbacks) {
                    if (entry.mouseRelease) {
                        entry.mouseRelease(mouseEvent);
                    }
                }

                ::ReleaseCapture();
            }
            if (uMsg == WM_MOUSEMOVE) {
                if (mouseCaptured) {
                    PlatformMouseEventArgs mouseEvent;

                    mouseEvent.coordinateX = float(short(LOWORD(lParam)));
                    mouseEvent.coordinateY = float(short(HIWORD(lParam)));

                    for (const auto &entry : g_mouseCallbacks) {
                        if (entry.mouseMove) {
                            entry.mouseMove(mouseEvent);
                        }
                    }
                }
            }
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    std::shared_ptr<PlatformInterface> PlatformInterface::makeInstance() {
        assert(g_platformCreated == false);
        return std::make_shared<Win32Platform>();
    }

    Win32Platform::Win32Platform() {

    }

    Win32Platform::~Win32Platform() {

    }

    void Win32Platform::logInfo(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(g_logMutex);

        va_list args;
        va_start(args, fmt);
        vsprintf_s(g_logMessageBuffer, fmt, args);
        va_end(args);

        OutputDebugStringA("[Inf] ");
        OutputDebugStringA(g_logMessageBuffer);
        OutputDebugStringA("\n");        
    }

    void Win32Platform::logWarning(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(g_logMutex);

        va_list args;
        va_start(args, fmt);
        vsprintf_s(g_logMessageBuffer, fmt, args);
        va_end(args);

        OutputDebugStringA("[Wrn] ");
        OutputDebugStringA(g_logMessageBuffer);
        OutputDebugStringA("\n");
    }

    void Win32Platform::logError(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(g_logMutex);

        va_list args;
        va_start(args, fmt);
        vsprintf_s(g_logMessageBuffer, fmt, args);
        va_end(args);
        
        OutputDebugStringA("[Err] ");
        OutputDebugStringA(g_logMessageBuffer);
        OutputDebugStringA("\n");        
    }

    std::vector<std::string> Win32Platform::formFileList(const char *dirPath) const {
        std::vector<std::string> result;
        std::wstring dirPathW = std::wstring_convert<std::codecvt_utf8_utf16 <wchar_t>>().from_bytes(dirPath);


        return result;
    }

    bool Win32Platform::loadFile(const char *filePath, std::unique_ptr<unsigned char[]> &data, std::size_t &size) const {
        std::wstring filePathW = std::wstring_convert<std::codecvt_utf8_utf16 <wchar_t>>().from_bytes(filePath);
        std::fstream fileStream(filePathW, std::ios::binary | std::ios::in | std::ios::ate);
        
        if (fileStream.is_open() && fileStream.good()) {
            std::size_t fsize = std::size_t(fileStream.tellg());
            data = std::make_unique<unsigned char[]>(fsize);
            fileStream.seekg(0);
            fileStream.read((char *)data.get(), fsize);
            size = fsize;

            return true;
        }

        return false;
    }

    float Win32Platform::getNativeScreenWidth() const {
        return float(APP_WIDTH);
    }

    float Win32Platform::getNativeScreenHeight() const {
        return float(APP_HEIGHT);
    }

    void *Win32Platform::setNativeRenderingContext(void *context) {
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

            swapChainDesc.Width = APP_WIDTH;
            swapChainDesc.Height = APP_HEIGHT;
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

            dxgiFactory->CreateSwapChainForHwnd(device, g_window, &swapChainDesc, nullptr, nullptr, &swapChain);
            dxgiFactory->Release();
            dxgiDevice->Release();
        }

        return swapChain;
    }

    PlatformCallbackToken Win32Platform::addKeyboardCallbacks(
        std::function<void(const PlatformKeyboardEventArgs &)> &&down,
        std::function<void(const PlatformKeyboardEventArgs &)> &&up
    ) {
        PlatformCallbackToken result {reinterpret_cast<PlatformCallbackToken>(callbacksIdSource++)};
        g_keyboardCallbacks.emplace_front(KeyboardCallbacksEntry {result, std::move(down), std::move(up)});
        return result;
    }

    PlatformCallbackToken Win32Platform::addMouseCallbacks(
        std::function<void(const PlatformMouseEventArgs &)> &&press,
        std::function<void(const PlatformMouseEventArgs &)> &&move,
        std::function<void(const PlatformMouseEventArgs &)> &&release
    ) {
        PlatformCallbackToken result {reinterpret_cast<PlatformCallbackToken>(callbacksIdSource++)};
        g_mouseCallbacks.emplace_front(MouseCallbacksEntry {result, std::move(press), std::move(move), std::move(release)});
        return result;
    }

    void Win32Platform::removeCallbacks(PlatformCallbackToken& token) {
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

    void Win32Platform::showCursor() {
        if (g_window != nullptr) {
            ShowCursor(true);
        }
    }

    void Win32Platform::hideCursor() {
        if (g_window != nullptr) {
            ShowCursor(false);
        }
    }
    
    void Win32Platform::run(std::function<void(float)> &&updateAndDraw) {
        g_updateAndDraw = std::move(updateAndDraw);
        
        ::SetProcessDPIAware();
        ::WNDCLASS wc;
        static wchar_t *szAppName = L"App";

        g_hinst = ::GetModuleHandle(0);
        wc.style = NULL;
        wc.lpfnWndProc = (::WNDPROC)WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = g_hinst;
        wc.hIcon = ::LoadIcon(0, NULL);
        wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (::HBRUSH)(COLOR_WINDOW + 0);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = szAppName;

        int tmpstyle = (WS_SYSMENU | WS_MINIMIZEBOX);
        int wndborderx = ::GetSystemMetrics(SM_CXSIZEFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER);
        int wndbordery = ::GetSystemMetrics(SM_CXSIZEFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER);
        int wndcaption = ::GetSystemMetrics(SM_CYCAPTION);

        ::RegisterClass(&wc);
        g_window = ::CreateWindow(szAppName, szAppName, tmpstyle, CW_USEDEFAULT, CW_USEDEFAULT, APP_WIDTH + wndborderx * 2, APP_HEIGHT + wndcaption + wndbordery * 2, HWND_DESKTOP, NULL, g_hinst, NULL);

        ::ShowWindow(g_window, SW_NORMAL);
        ::UpdateWindow(g_window);

        static std::chrono::time_point<std::chrono::high_resolution_clock> prevFrameTime = std::chrono::high_resolution_clock::now();

        while (!g_killed) {
            while (::PeekMessage(&g_message, g_window, 0, 0, PM_REMOVE)) {
                ::TranslateMessage(&g_message);
                ::DispatchMessage(&g_message);
            }

            auto curFrameTime = std::chrono::high_resolution_clock::now();
            float dt = float(std::chrono::duration_cast<std::chrono::microseconds>(curFrameTime - prevFrameTime).count()) / 1000.0f;
            g_updateAndDraw(dt);
            prevFrameTime = curFrameTime;
        }

        ::UnregisterClass(szAppName, g_hinst);
        ::DestroyWindow(g_window);
    }
}