
// TODO:
// + keyboard
// + gamepad
// + file operations

#include "interfaces.h"
#include "platform_windows.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <d3d11_1.h>
#include <codecvt>

#include <chrono>
#include <list>
#include <mutex>
#include <algorithm>
#include <fstream>

namespace {
    struct KeyboardCallbacksEntry {
        const void *handle;
        std::function<void(engine::KeyboardEventArgs &)> keyboardDown;
        std::function<void(engine::KeyboardEventArgs &)> keyboardUp;
    };

    struct MouseCallbacksEntry {
        const void *handle;
        std::function<void(engine::MouseEventArgs &)> mousePress;
        std::function<void(engine::MouseEventArgs &)> mouseMove;
        std::function<void(engine::MouseEventArgs &)> mouseRelease;
    };

    const unsigned APP_WIDTH = 1280;
    const unsigned APP_HEIGHT = 720;

    std::weak_ptr<engine::Platform> g_instance;

    HINSTANCE  g_hinst;
    HWND       g_window;
    MSG        g_message;
    bool       g_killed = false;

    std::size_t callbacksIdSource = 0;
    std::list<KeyboardCallbacksEntry> g_keyboardCallbacks;
    std::list<MouseCallbacksEntry> g_mouseCallbacks;

    std::mutex g_logMutex;
    char g_logMessageBuffer[65536];
}

namespace {
    LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        static bool mouseCaptured = false;

        if (uMsg == WM_CLOSE) {
            g_killed = true;
        }
        if (uMsg == WM_LBUTTONDOWN) {
            mouseCaptured = true;
            ::SetCapture(hwnd);

            engine::MouseEventArgs mouseEvent;

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

            engine::MouseEventArgs mouseEvent;

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
                engine::MouseEventArgs mouseEvent;

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

namespace engine {
    WindowsPlatform::WindowsPlatform() {}
    WindowsPlatform::~WindowsPlatform() {}

    std::vector<std::string> WindowsPlatform::formFileList(const char *dirPath) {
        return {};
    }
    bool WindowsPlatform::loadFile(const char *filePath, std::unique_ptr<unsigned char[]> &data, std::size_t &size) {
        return false;
    }

    float WindowsPlatform::getNativeScreenWidth() const {
        return float(APP_WIDTH);
    }
    float WindowsPlatform::getNativeScreenHeight() const {
        return float(APP_HEIGHT);
    }

    void *WindowsPlatform::attachNativeRenderingContext(void *context) {
        IDXGISwapChain1 *swapChain = nullptr;

        if (g_window != nullptr) {
            ID3D11Device1 *device = reinterpret_cast<ID3D11Device1 *>(context);
            IDXGIAdapter *adapter = nullptr;
            IDXGIDevice1 *dxgiDevice = nullptr;
            IDXGIFactory2 *dxgiFactory = nullptr;
            device->QueryInterface(__uuidof(IDXGIDevice1), (void **)&dxgiDevice);

            dxgiDevice->SetMaximumFrameLatency(1);
            dxgiDevice->GetAdapter(&adapter);
            adapter->GetParent(__uuidof(IDXGIFactory2), (void **)&dxgiFactory);
            adapter->Release();

            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };

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

    void WindowsPlatform::showCursor() {
        if (g_window != nullptr) {
            ShowCursor(true);
        }
    }
    void WindowsPlatform::hideCursor() {
        if (g_window != nullptr) {
            ShowCursor(false);
        }
    }

    void WindowsPlatform::showKeyboard() {}
    void WindowsPlatform::hideKeyboard() {}

    EventHandlersToken WindowsPlatform::addKeyboardEventHandlers(
        std::function<void(const KeyboardEventArgs &)> &&down,
        std::function<void(const KeyboardEventArgs &)> &&up
    ) {
        return nullptr;
    }

    EventHandlersToken WindowsPlatform::addInputEventHandlers(
        std::function<void(const char(&utf8char)[4])> &&input,
        std::function<void()> &&backspace
    ) {
        return nullptr;
    }

    EventHandlersToken WindowsPlatform::addMouseEventHandlers(
        std::function<void(const MouseEventArgs &)> &&press,
        std::function<void(const MouseEventArgs &)> &&move,
        std::function<void(const MouseEventArgs &)> &&release
    ) {
        EventHandlersToken result{ reinterpret_cast<EventHandlersToken>(callbacksIdSource++) };
        g_mouseCallbacks.emplace_front(MouseCallbacksEntry{ result, std::move(press), std::move(move), std::move(release) });
        return result;
    }

    EventHandlersToken WindowsPlatform::addTouchEventHandlers(
        std::function<void(const TouchEventArgs &)> &&start,
        std::function<void(const TouchEventArgs &)> &&move,
        std::function<void(const TouchEventArgs &)> &&finish
    ) {
        return nullptr;
    }

    EventHandlersToken WindowsPlatform::addGamepadEventHandlers(
        std::function<void(const GamepadEventArgs &)> &&buttonPress,
        std::function<void(const GamepadEventArgs &)> &&buttonRelease
    ) {
        return nullptr;
    }

    void WindowsPlatform::removeEventHandlers(EventHandlersToken token) {
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

    void WindowsPlatform::run(std::function<void(float)> &&updateAndDraw) {
        ::SetProcessDPIAware();
        ::WNDCLASSW wc;

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

        ::RegisterClassW(&wc);
        g_window = ::CreateWindowW(szAppName, szAppName, tmpstyle, CW_USEDEFAULT, CW_USEDEFAULT, APP_WIDTH + wndborderx * 2, APP_HEIGHT + wndcaption + wndbordery * 2, HWND_DESKTOP, NULL, g_hinst, NULL);

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
            updateAndDraw(dt);
            prevFrameTime = curFrameTime;
        }

        ::UnregisterClassW(szAppName, g_hinst);
        ::DestroyWindow(g_window);
    }

    void WindowsPlatform::exit() {
        g_killed = true;
    }

    void WindowsPlatform::logMsg(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(g_logMutex);

        va_list args;
        va_start(args, fmt);
        vsprintf_s(g_logMessageBuffer, fmt, args);
        va_end(args);

        OutputDebugStringA(g_logMessageBuffer);
        OutputDebugStringA("\n");

        printf("%s\n", g_logMessageBuffer);
    }

    std::shared_ptr<Platform> Platform::instance() {
        std::shared_ptr<Platform> result;

        if (g_instance.use_count() == 0) {
            g_instance = result = std::make_shared<WindowsPlatform>();
        }

        return result;
    }

}