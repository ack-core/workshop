
// TODO:
// + keyboard
// + gamepad
// + file operations

#include "platform.h"

#include <chrono>
#include <list>
#include <mutex>
#include <algorithm>
#include <fstream>
#include <filesystem>

#ifdef PLATFORM_WINDOWS

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <d3d11_1.h>

#include "platform_windows.h"

namespace {
    struct KeyboardCallbacksEntry {
        const void *handle;
        std::function<void(foundation::PlatformKeyboardEventArgs &)> keyboardDown;
        std::function<void(foundation::PlatformKeyboardEventArgs &)> keyboardUp;
    };

    struct MouseCallbacksEntry {
        const void *handle;
        std::function<void(foundation::PlatformMouseEventArgs &)> mousePress;
        std::function<void(foundation::PlatformMouseEventArgs &)> mouseMove;
        std::function<void(foundation::PlatformMouseEventArgs &)> mouseRelease;
    };

    static wchar_t *APP_CLASS_NAME = L"Application";

    const unsigned APP_WIDTH = 1280;
    const unsigned APP_HEIGHT = 720;
    const unsigned BUFFER_SIZE = 65536;

    bool g_killed = false;

    std::weak_ptr<foundation::PlatformInterface> g_instance;
    std::mutex g_logMutex;

    std::size_t g_callbacksIdSource = 0;
    std::list<KeyboardCallbacksEntry> g_keyboardCallbacks;
    std::list<MouseCallbacksEntry> g_mouseCallbacks;

    char g_buffer[BUFFER_SIZE];
}

namespace {
    LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        static bool mouseCaptured = false;

        if (uMsg == WM_CLOSE) {
            g_killed = true;
        }
        if (uMsg == WM_KEYDOWN) {
            if ((lParam & 0x40000000) == 0) {
                if (wParam >= 'A' && wParam <= 'Z') {
                    foundation::PlatformKeyboardEventArgs keyboardEvent;
                    keyboardEvent.key = foundation::PlatformKeyboardEventArgs::Key(wParam - 'A');
                    
                    for (const auto &entry : g_keyboardCallbacks) {
                        if (entry.keyboardDown) {
                            entry.keyboardDown(keyboardEvent);
                        }
                    }
                }
            }
        }
        if (uMsg == WM_KEYUP) {
            if (wParam >= 'A' && wParam <= 'Z') {
                foundation::PlatformKeyboardEventArgs keyboardEvent;
                keyboardEvent.key = foundation::PlatformKeyboardEventArgs::Key(wParam - 'A');

                for (const auto &entry : g_keyboardCallbacks) {
                    if (entry.keyboardUp) {
                        entry.keyboardUp(keyboardEvent);
                    }
                }
            }
        }
        if (uMsg == WM_LBUTTONDOWN) {
            mouseCaptured = true;
            ::SetCapture(hwnd);

            foundation::PlatformMouseEventArgs mouseEvent;

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

            foundation::PlatformMouseEventArgs mouseEvent;

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
            foundation::PlatformMouseEventArgs mouseEvent;

            mouseEvent.captured = mouseCaptured;
            mouseEvent.coordinateX = float(short(LOWORD(lParam)));
            mouseEvent.coordinateY = float(short(HIWORD(lParam)));

            for (const auto &entry : g_mouseCallbacks) {
                if (entry.mouseMove) {
                    entry.mouseMove(mouseEvent);
                }
            }
        }
        if (uMsg == 0x020A) {
            foundation::PlatformMouseEventArgs mouseEvent;

            POINT cursorPosition;
            GetCursorPos(&cursorPosition);
            ScreenToClient(hwnd, &cursorPosition);

            mouseEvent.coordinateX = float(cursorPosition.x);
            mouseEvent.coordinateY = float(cursorPosition.y);
            mouseEvent.wheel = GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? 1 : -1;

            for (const auto &entry : g_mouseCallbacks) {
                if (entry.mouseMove) {
                    entry.mouseMove(mouseEvent);
                }
            }
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

namespace foundation {
    WindowsPlatform::WindowsPlatform() {
        ::SetProcessDPIAware();
        ::WNDCLASSW wc;

        _message = {};

        _hinst = ::GetModuleHandle(0);
        wc.style = NULL;
        wc.lpfnWndProc = (::WNDPROC)WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = _hinst;
        wc.hIcon = ::LoadIcon(0, NULL);
        wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (::HBRUSH)(COLOR_WINDOW + 0);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = APP_CLASS_NAME;

        int tmpstyle = (WS_SYSMENU | WS_MINIMIZEBOX);
        int wndborderx = ::GetSystemMetrics(SM_CXSIZEFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER);
        int wndbordery = ::GetSystemMetrics(SM_CXSIZEFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER);
        int wndcaption = ::GetSystemMetrics(SM_CYCAPTION);

        ::RegisterClassW(&wc);
        _window = ::CreateWindowW(APP_CLASS_NAME, APP_CLASS_NAME, tmpstyle, CW_USEDEFAULT, CW_USEDEFAULT, APP_WIDTH + wndborderx * 2, APP_HEIGHT + wndcaption + wndbordery * 2, HWND_DESKTOP, NULL, _hinst, NULL);

        ::ShowWindow(_window, SW_NORMAL);
        ::UpdateWindow(_window);

        ::GetModuleFileNameA(nullptr, g_buffer, BUFFER_SIZE);
        *(strrchr(g_buffer, '\\') + 1) = 0;
        std::replace(std::begin(g_buffer), std::end(g_buffer), '\\', '/');

        _executableDirectoryPath = g_buffer;
    }

    WindowsPlatform::~WindowsPlatform() {
        ::UnregisterClassW(APP_CLASS_NAME, _hinst);
        ::DestroyWindow(_window);
    }

    std::vector<FileEntry> WindowsPlatform::formFileList(const char *dirPath) {
        std::string fullPath = _executableDirectoryPath + dirPath;
        std::vector<FileEntry> result;

        for (auto &entry : std::filesystem::directory_iterator(fullPath)) {
            result.emplace_back(FileEntry{ entry.path().generic_u8string(), entry.is_directory()});
        }

        return result;
    }
    bool WindowsPlatform::loadFile(const char *filePath, std::unique_ptr<unsigned char[]> &data, std::size_t &size) {
        std::string fullPath = _executableDirectoryPath + filePath;
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

    float WindowsPlatform::getNativeScreenWidth() const {
        return float(APP_WIDTH);
    }
    float WindowsPlatform::getNativeScreenHeight() const {
        return float(APP_HEIGHT);
    }

    void *WindowsPlatform::attachNativeRenderingContext(void *context) {
        IDXGISwapChain1 *swapChain = nullptr;

        if (_window != nullptr) {
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
            swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            swapChainDesc.Stereo = false;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = 2;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            swapChainDesc.Flags = 0;
            swapChainDesc.Scaling = DXGI_SCALING_NONE;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

            dxgiFactory->CreateSwapChainForHwnd(device, _window, &swapChainDesc, nullptr, nullptr, &swapChain);
            dxgiFactory->Release();
            dxgiDevice->Release();
        }

        return swapChain;
    }

    void WindowsPlatform::showCursor() {
        if (_window != nullptr) {
            ShowCursor(true);
        }
    }
    void WindowsPlatform::hideCursor() {
        if (_window != nullptr) {
            ShowCursor(false);
        }
    }

    void WindowsPlatform::showKeyboard() {}
    void WindowsPlatform::hideKeyboard() {}

    EventHandlersToken WindowsPlatform::addKeyboardEventHandlers(
        std::function<void(const PlatformKeyboardEventArgs &)> &&down,
        std::function<void(const PlatformKeyboardEventArgs &)> &&up
    ) {
        EventHandlersToken result{ reinterpret_cast<EventHandlersToken>(g_callbacksIdSource++) };
        g_keyboardCallbacks.emplace_front(KeyboardCallbacksEntry{ result, std::move(down), std::move(up) });
        return result;
    }

    EventHandlersToken WindowsPlatform::addInputEventHandlers(
        std::function<void(const char(&utf8char)[4])> &&input,
        std::function<void()> &&backspace
    ) {
        return nullptr;
    }

    EventHandlersToken WindowsPlatform::addMouseEventHandlers(
        std::function<void(const PlatformMouseEventArgs &)> &&press,
        std::function<void(const PlatformMouseEventArgs &)> &&move,
        std::function<void(const PlatformMouseEventArgs &)> &&release
    ) {
        EventHandlersToken result{ reinterpret_cast<EventHandlersToken>(g_callbacksIdSource++) };
        g_mouseCallbacks.emplace_front(MouseCallbacksEntry{ result, std::move(press), std::move(move), std::move(release) });
        return result;
    }

    EventHandlersToken WindowsPlatform::addTouchEventHandlers(
        std::function<void(const PlatformTouchEventArgs &)> &&start,
        std::function<void(const PlatformTouchEventArgs &)> &&move,
        std::function<void(const PlatformTouchEventArgs &)> &&finish
    ) {
        return nullptr;
    }

    EventHandlersToken WindowsPlatform::addGamepadEventHandlers(
        std::function<void(const PlatformGamepadEventArgs &)> &&buttonPress,
        std::function<void(const PlatformGamepadEventArgs &)> &&buttonRelease
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
        static std::chrono::time_point<std::chrono::high_resolution_clock> prevFrameTime = std::chrono::high_resolution_clock::now();

        while (!g_killed) {
            while (::PeekMessage(&_message, _window, 0, 0, PM_REMOVE)) {
                ::TranslateMessage(&_message);
                ::DispatchMessage(&_message);
            }

            auto curFrameTime = std::chrono::high_resolution_clock::now();
            float dt = float(std::chrono::duration_cast<std::chrono::milliseconds>(curFrameTime - prevFrameTime).count()) / 1000.0f;
            updateAndDraw(dt);
            prevFrameTime = curFrameTime;
        }
    }

    void WindowsPlatform::exit() {
        g_killed = true;
    }

    void WindowsPlatform::logMsg(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(g_logMutex);

        va_list args;
        va_start(args, fmt);
        vsprintf_s(g_buffer, fmt, args);
        va_end(args);

        OutputDebugStringA(g_buffer);
        OutputDebugStringA("\n");

        printf("%s\n", g_buffer);
    }

    void WindowsPlatform::logError(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(g_logMutex);

        va_list args;
        va_start(args, fmt);
        vsprintf_s(g_buffer, fmt, args);
        va_end(args);

        OutputDebugStringA(g_buffer);
        OutputDebugStringA("\n");

        printf("%s\n", g_buffer);

        DebugBreak();
    }
}

namespace foundation {
    std::shared_ptr<PlatformInterface> PlatformInterface::instance() {
        std::shared_ptr<PlatformInterface> result;

        if (g_instance.use_count() == 0) {
            g_instance = result = std::make_shared<WindowsPlatform>();
        }
        else {
            result = g_instance.lock();
        }

        return result;
    }
}

#endif // PLATFORM_WINDOWS
