
// TODO:
// + gamepad
// + text input

#ifdef PLATFORM_WINDOWS

#include <atomic>
#include <chrono>
#include <list>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <cstdarg>
#include <cstdio>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <d3d11_1.h>

#include "platform_windows.h"

namespace {
    const wchar_t *APP_CLASS_NAME = L"Application";

#if defined(IS_TESTS)
    const unsigned APP_WIDTH = foundation::TESTING_RENDER_WIDTH;
    const unsigned APP_HEIGHT = foundation::TESTING_RENDER_HEIGHT;
#else
    const unsigned APP_WIDTH = 1280;
    const unsigned APP_HEIGHT = 720;
#endif
    const unsigned BUFFER_SIZE = 65536;

    unsigned g_width = APP_WIDTH;
    unsigned g_height = APP_HEIGHT;
    RECT g_minWindowRect { 0, 0, LONG(APP_WIDTH), LONG(APP_HEIGHT) };

    char g_buffer[BUFFER_SIZE];
    std::atomic<bool> g_killed {false};
    int g_returnCode = 0;

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

    bool translateKey(WPARAM vk, foundation::Key &key) {
        if (vk >= 'A' && vk <= 'Z') {
            key = foundation::Key(std::uint32_t(foundation::Key::A) + std::uint32_t(vk - 'A'));
            return true;
        }
        if (vk >= '0' && vk <= '9') {
            key = foundation::Key(std::uint32_t(foundation::Key::NUM0) + std::uint32_t(vk - '0'));
            return true;
        }
        if (vk >= VK_F1 && vk <= VK_F10) {
            key = foundation::Key(std::uint32_t(foundation::Key::F1) + std::uint32_t(vk - VK_F1));
            return true;
        }
        switch (vk) {
            case VK_LEFT: key = foundation::Key::LEFT; return true;
            case VK_RIGHT: key = foundation::Key::RIGHT; return true;
            case VK_UP: key = foundation::Key::UP; return true;
            case VK_DOWN: key = foundation::Key::DOWN; return true;
            case VK_SPACE: key = foundation::Key::SPACE; return true;
            case VK_RETURN: key = foundation::Key::ENTER; return true;
            case VK_TAB: key = foundation::Key::TAB; return true;
        }
        return false;
    }

    void keyboardEvent(foundation::PlatformKeyboardEventArgs::EventType type, WPARAM vk) {
        foundation::PlatformKeyboardEventArgs args;
        args.type = type;

        if (translateKey(vk, args.key)) {
            for (auto &index : g_keyboardHandlers) {
                if (index.second(args)) {
                    break;
                }
            }
        }
    }

    void pointerEvent(HWND hwnd, foundation::PlatformPointerEventArgs::EventType type, float x, float y, bool left, bool right, int wheel) {
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
            POINT cursorPosition { LONG(args.coordinateX), LONG(args.coordinateY) };
            ::ClientToScreen(hwnd, &cursorPosition);
            ::SetCursorPos(cursorPosition.x, cursorPosition.y);
        }
    }

    LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        using PointerEventType = foundation::PlatformPointerEventArgs::EventType;
        using KeyboardEventType = foundation::PlatformKeyboardEventArgs::EventType;

        const float x = float(short(LOWORD(lParam)));
        const float y = float(short(HIWORD(lParam)));

        switch (uMsg) {
            case WM_CLOSE:
            {
                g_killed = true;
                return 0;
            }
            case WM_GETMINMAXINFO:
            {
                MINMAXINFO *info = reinterpret_cast<MINMAXINFO *>(lParam);
                info->ptMinTrackSize.x = g_minWindowRect.right - g_minWindowRect.left;
                info->ptMinTrackSize.y = g_minWindowRect.bottom - g_minWindowRect.top;
                return 0;
            }
#if !defined(IS_TESTS)
            case WM_SIZE:
            {
                const unsigned width = LOWORD(lParam);
                const unsigned height = HIWORD(lParam);

                if (wParam != SIZE_MINIMIZED && (width != g_width || height != g_height)) {
                    g_width = width;
                    g_height = height;

                    for (auto &index : g_resizeHandlers) {
                        index.second();
                    }
                }
                break;
            }
#endif
            case WM_KEYDOWN:
            {
                if ((lParam & 0x40000000) == 0) {
                    keyboardEvent(KeyboardEventType::PRESS, wParam);
                }
                break;
            }
            case WM_KEYUP:
            {
                keyboardEvent(KeyboardEventType::RELEASE, wParam);
                break;
            }
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            {
                ::SetCapture(hwnd);
                pointerEvent(hwnd, PointerEventType::START, x, y, uMsg == WM_LBUTTONDOWN, uMsg == WM_RBUTTONDOWN, 0);
                break;
            }
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            {
                pointerEvent(hwnd, PointerEventType::FINISH, x, y, uMsg == WM_LBUTTONUP, uMsg == WM_RBUTTONUP, 0);

                if ((wParam & (MK_LBUTTON | MK_RBUTTON)) == 0) {
                    ::ReleaseCapture();
                }
                break;
            }
            case WM_MOUSEMOVE:
            {
                const bool left = (wParam & MK_LBUTTON) != 0;
                const bool right = (wParam & MK_RBUTTON) != 0;
                pointerEvent(hwnd, left || right ? PointerEventType::MOVE : PointerEventType::HOVER, x, y, left, right, 0);
                break;
            }
            case WM_MOUSEWHEEL:
            {
                POINT cursorPosition { LONG(x), LONG(y) }; // wheel coordinates are in screen space
                ::ScreenToClient(hwnd, &cursorPosition);

                const int wheel = GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? 1 : -1;
                pointerEvent(hwnd, PointerEventType::WHEEL, float(cursorPosition.x), float(cursorPosition.y), false, false, wheel);
                break;
            }
        }

        return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

namespace foundation {
    namespace {
        class FileLoadTask : public AsyncTask {
        public:
            FileLoadTask(std::string &&filePath, util::callback<void(ByteDataPtr &&, std::size_t)> &&completion) : _path(std::move(filePath)), _size(0), _completion(std::move(completion)) {}
            ~FileLoadTask() override {}

        public:
            void executeInBackground() override {
                std::fstream fileStream(_path, std::ios::binary | std::ios::in | std::ios::ate);

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
    WindowsPlatform::WindowsPlatform() {
        ::SetProcessDPIAware();
        ::WNDCLASSW wc;

        _hinst = ::GetModuleHandle(0);
        wc.style = 0;
        wc.lpfnWndProc = (::WNDPROC)WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = _hinst;
        wc.hIcon = ::LoadIcon(0, NULL);
        wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (::HBRUSH)(COLOR_WINDOW + 0);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = APP_CLASS_NAME;

        ::AdjustWindowRect(&g_minWindowRect, WS_OVERLAPPEDWINDOW, FALSE);

        ::RegisterClassW(&wc);
        _window = ::CreateWindowW(APP_CLASS_NAME, APP_CLASS_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, g_minWindowRect.right - g_minWindowRect.left, g_minWindowRect.bottom - g_minWindowRect.top, HWND_DESKTOP, NULL, _hinst, NULL);

        ::ShowWindow(_window, SW_NORMAL);
        ::UpdateWindow(_window);

        ::GetModuleFileNameA(nullptr, g_buffer, BUFFER_SIZE);
        *(strrchr(g_buffer, '\\') + 1) = 0;
        std::replace(std::begin(g_buffer), std::end(g_buffer), '\\', '/');

        _dataPath = std::string(g_buffer) + "data/";
    }

    WindowsPlatform::~WindowsPlatform() {
        ::DestroyWindow(_window);
        ::UnregisterClassW(APP_CLASS_NAME, _hinst);
    }

    void WindowsPlatform::executeAsync(std::unique_ptr<AsyncTask> &&task) {
        g_asyncWorkCount++;
        backgroundThreadPush(g_worker, std::move(task));
    }

    bool WindowsPlatform::isAsyncWorkInProgress() const {
        return g_asyncWorkCount != 0;
    }

    void WindowsPlatform::loadFile(const char *filePath, util::callback<void(ByteDataPtr &&data, std::size_t size)> &&completion, bool suppressOutput) {
        g_asyncWorkCount++;
        backgroundThreadPush(g_io, std::make_unique<FileLoadTask>(_dataPath + filePath, std::move(completion)));
    }
    void WindowsPlatform::saveFile(const char *filePath, const std::uint8_t *data, std::size_t size, util::callback<void(bool)> &&completion, bool suppressOutput) {
        g_asyncWorkCount++;
        backgroundThreadPush(g_io, std::make_unique<FileSaveTask>(_dataPath + filePath, data, data ? size : 0, std::move(completion)));
    }

    float WindowsPlatform::getScreenWidth() const {
        return float(g_width);
    }
    float WindowsPlatform::getScreenHeight() const {
        return float(g_height);
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

            swapChainDesc.Width = g_width;
            swapChainDesc.Height = g_height;
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

    void WindowsPlatform::sendEditorMsg(const std::string &msg, const std::string &data) {

    }
    void WindowsPlatform::editorLoopbackMsg(const std::string &msg, const std::string &data) {
        for (auto &index : g_editorHandlers) {
            if (index.second(msg, data)) {
                break;
            }
        }
    }

    EventHandlerToken WindowsPlatform::addEditorEventHandler(util::callback<bool(const std::string &, const std::string &)> &&handler, bool setTop) {
        return addHandler(g_editorHandlers, std::move(handler), setTop);
    }
    EventHandlerToken WindowsPlatform::addKeyboardEventHandler(util::callback<bool(const PlatformKeyboardEventArgs &)> &&handler, bool setTop) {
        return addHandler(g_keyboardHandlers, std::move(handler), setTop);
    }
    EventHandlerToken WindowsPlatform::addInputEventHandler(util::callback<bool(const char(&utf8char)[4])> &&input, bool setTop) {
        return nullptr;
    }
    EventHandlerToken WindowsPlatform::addPointerEventHandler(util::callback<bool(const PlatformPointerEventArgs &)> &&handler, bool setTop) {
        return addHandler(g_pointerHandlers, std::move(handler), setTop);
    }
    EventHandlerToken WindowsPlatform::addGamepadEventHandler(util::callback<bool(const PlatformGamepadEventArgs &)> &&handler, bool setTop) {
        return nullptr;
    }
    EventHandlerToken WindowsPlatform::addResizeHandler(util::callback<void()> &&handler) {
        return addHandler(g_resizeHandlers, std::move(handler), false);
    }

    void WindowsPlatform::removeEventHandler(EventHandlerToken token) {
        removeHandler(g_editorHandlers, token);
        removeHandler(g_pointerHandlers, token);
        removeHandler(g_keyboardHandlers, token);
        removeHandler(g_resizeHandlers, token);
    }

    void WindowsPlatform::sendPointerEvent(const PlatformPointerEventArgs &args) {
        using EventType = PlatformPointerEventArgs::EventType;

        const bool right = args.flags.isRightButton != 0;
        const WPARAM buttons = (right ? MK_RBUTTON : 0) | (args.flags.isLeftButton != 0 || !right ? MK_LBUTTON : 0);
        const LPARAM position = MAKELPARAM(short(args.coordinateX), short(args.coordinateY));

        POINT screenPosition { LONG(args.coordinateX), LONG(args.coordinateY) }; // wheel coordinates are in screen space
        ::ClientToScreen(_window, &screenPosition);

        switch (args.type) {
            case EventType::START: ::SendMessage(_window, right ? WM_RBUTTONDOWN : WM_LBUTTONDOWN, buttons, position); return;
            case EventType::FINISH: ::SendMessage(_window, right ? WM_RBUTTONUP : WM_LBUTTONUP, 0, position); return;
            case EventType::MOVE: ::SendMessage(_window, WM_MOUSEMOVE, buttons, position); return;
            case EventType::HOVER: ::SendMessage(_window, WM_MOUSEMOVE, 0, position); return;
            case EventType::WHEEL: ::SendMessage(_window, WM_MOUSEWHEEL, MAKEWPARAM(0, args.flags.wheel * WHEEL_DELTA), MAKELPARAM(screenPosition.x, screenPosition.y)); return;
        }

        for (auto &index : g_pointerHandlers) { // no native message
            if (index.second(args)) {
                break;
            }
        }
    }

    void WindowsPlatform::sendKeyboardEvent(const PlatformKeyboardEventArgs &args) {
        for (WPARAM vk = 0; vk < 256; vk++) {
            Key key;

            if (translateKey(vk, key) && key == args.key) {
                ::SendMessage(_window, args.type == PlatformKeyboardEventArgs::EventType::PRESS ? WM_KEYDOWN : WM_KEYUP, vk, 0);
                return;
            }
        }
    }

    void WindowsPlatform::setLoop(util::callback<void(float)> &&updateAndDraw) {
        g_updateAndDraw = std::move(updateAndDraw);
    }

    void WindowsPlatform::exit(int returnCode) {
        g_killed = true;
        g_returnCode = returnCode;
    }

    void WindowsPlatform::logMsg(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(_logMutex);

        va_list args;
        va_start(args, fmt);
        vsnprintf(g_buffer, BUFFER_SIZE, fmt, args);
        va_end(args);

        OutputDebugStringA(g_buffer);
        OutputDebugStringA("\n");

        printf("%s\n", g_buffer);
    }

    void WindowsPlatform::logError(const char *fmt, ...) {
        std::lock_guard<std::mutex> guard(_logMutex);

        va_list args;
        va_start(args, fmt);
        vsnprintf(g_buffer, BUFFER_SIZE, fmt, args);
        va_end(args);

        OutputDebugStringA(g_buffer);
        OutputDebugStringA("\n");

        printf("%s\n", g_buffer);

        if (IsDebuggerPresent()) {
            DebugBreak();
        }
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

extern "C" void initialize();
extern "C" void deinitialize();

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    initialize();

    g_io.thread = std::thread(&backgroundThreadLoop, std::ref(g_io));
    g_worker.thread = std::thread(&backgroundThreadLoop, std::ref(g_worker));

    std::vector<std::unique_ptr<foundation::AsyncTask>> foregroundQueue;
    std::chrono::time_point<std::chrono::high_resolution_clock> prevFrameTime = std::chrono::high_resolution_clock::now();
    MSG message = {};

    while (!g_killed) {
        while (::PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&message);
            ::DispatchMessage(&message);
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

        auto curFrameTime = std::chrono::high_resolution_clock::now();
        float dt = float(std::chrono::duration_cast<std::chrono::milliseconds>(curFrameTime - prevFrameTime).count()) / 1000.0f;

        if (g_updateAndDraw) {
            g_updateAndDraw(dt);
        }

        prevFrameTime = curFrameTime;
    }

    backgroundThreadStop(g_io);
    backgroundThreadStop(g_worker);

    deinitialize();
    return g_returnCode;
}

#endif // PLATFORM_WINDOWS
