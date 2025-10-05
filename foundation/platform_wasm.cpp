
#ifdef PLATFORM_WASM
#include "platform_wasm.h"

#include <stdarg.h>
#include <cstdio>
#include <list>

#include "thirdparty/tlsf/tlsf.h"

namespace {
    const std::size_t PAGESIZE = 65536;

    std::atomic<std::uint32_t> g_memoryLock {0};
    std::size_t g_currentMemoryPages;
    std::size_t g_currentMemoryOffset;
    void *g_dynamicMemoryStart;
    
    std::uint32_t g_width;
    std::uint32_t g_height;
}

void fatal() {
    *(volatile int *)0xffffffff = 0;
}

// Imported from js
extern "C" {
    void js_waiting();
    void js_log(const std::uint16_t *ptr, int length);
    void js_task(const void *ptr);
    void js_fetch(const void *ptr, int pathLen);
    void js_save(const void *path, int pathLen, const void *data, int dataLen);
    void js_editorMsg(const char *msg, int msglen, const char *data, int datalen);
}

// Missing part of runtime
extern "C" {
    int __cxa_atexit(void (*func) (void *), void * arg, void * dso_handle) {
        return 0;
    }
    std::size_t strlen(const char *str) {
        const char *s = str;
        while (*s != 0) s++;
        return s - str;
    }
    int strcmp(const char *left, const char *right) {
        for (; *left == *right && *left; left++, right++);
        return *reinterpret_cast<const char *>(left) - *reinterpret_cast<const char *>(right);
    }
    int memcmp(const void *lhs, const void *rhs, std::size_t count) {
        const unsigned char *l = reinterpret_cast<const unsigned char *>(lhs);
        const unsigned char *r = reinterpret_cast<const unsigned char *>(rhs);
        for (; count && *l == *r; count--, l++, r++);
        return count ? *l - *r : 0;
    }
    void *memchr(const void *ptr, int ch, std::size_t count) {
        const unsigned char *mem = reinterpret_cast<const unsigned char *>(ptr);
        while (count--) {
            if (*mem == (unsigned char)ch) {
                return (void *)mem;
            }
            mem++;
        }
        return nullptr;
    }
    int isalpha(int c) {
        return (((unsigned)c | 32) - 'a') < 26;
    }
    int isdigit(int c) {
        return ((unsigned)c - '0') < 10;
    }
    int isgraph(int c) {
        return ((unsigned)c - 0x21) < 0x5e;
    }
    int isalnum(int c) {
        return isalpha(c) || isdigit(c);
    }
}
    
// Memory management
extern "C" {
    void *sbrk(intptr_t increment) {
        if (increment == 0) {
            return (void *)(__builtin_wasm_memory_size(0) * PAGESIZE);
        }
        if (increment < 0) {
            increment = 0;
        }
        
        std::size_t oldOffset = g_currentMemoryOffset;
        g_currentMemoryOffset += increment;
        
        std::size_t incMemoryPages = ((g_currentMemoryOffset + PAGESIZE - 1) / PAGESIZE) - g_currentMemoryPages;
        
        if (incMemoryPages) {
            std::size_t oldMemoryPages = __builtin_wasm_memory_grow(0, incMemoryPages);
            
            if (oldMemoryPages == std::size_t(-1)) {
                fatal();
            }
            
            g_currentMemoryPages += incMemoryPages;
        }
        
        return (void *)oldOffset;
    }
    void __init() { // This function is called before global ctors
        g_currentMemoryPages = __builtin_wasm_memory_size(0);
        g_currentMemoryOffset = g_currentMemoryPages * PAGESIZE;
        g_dynamicMemoryStart = sbrk(PAGESIZE);
        tlsf::init_memory_pool(PAGESIZE, g_dynamicMemoryStart);
    }
    void *malloc(size_t size) {
        std::uint32_t expected = 0;
        while (g_memoryLock.compare_exchange_strong(expected, 1) == false);
        
        void *result = tlsf::malloc_ex(size, g_dynamicMemoryStart);
        if (result == nullptr) {
            size_t areaSize = PAGESIZE * (size / PAGESIZE + 1);
            tlsf::add_new_area(sbrk(areaSize), areaSize, g_dynamicMemoryStart);
            result = tlsf::malloc_ex(size, g_dynamicMemoryStart);
        }
        
        g_memoryLock.store(0);
        ::memset(result, 0, size);
        return result;
    }
    void *aligned_alloc(size_t alignment, size_t size) {
        return ::malloc(size);
    }
    void free(void *ptr) {
        if (ptr) {
            std::uint32_t expected = 0;
            while (g_memoryLock.compare_exchange_strong(expected, 1) == false);
            
            tlsf::free_ex(ptr, g_dynamicMemoryStart);
            g_memoryLock.store(0);
        }
    }
}

namespace {
    std::weak_ptr<foundation::PlatformInterface> g_instance;

    std::list<std::pair<foundation::EventHandlerToken, util::callback<bool(const std::string &, const std::string &)>>> g_editorHandlers;
    std::list<std::pair<foundation::EventHandlerToken, util::callback<bool(const foundation::PlatformPointerEventArgs &)>>> g_pointerHandlers;
    std::list<std::pair<foundation::EventHandlerToken, util::callback<bool(const foundation::PlatformKeyboardEventArgs &)>>> g_keyboardHandlers;

    util::callback<void(float)> g_updateAndDraw;
    util::callback<void()> g_resizeHandler;
    
    foundation::EventHandlerToken g_tokenCounter = reinterpret_cast<foundation::EventHandlerToken>(0x100);
}

namespace foundation {
    WASMPlatform::WASMPlatform() : _dataPath("data/") {
        
    }
    WASMPlatform::~WASMPlatform() {
        
    }
    
    void WASMPlatform::executeAsync(std::unique_ptr<AsyncTask> &&task) {
        js_task(task.release());
    }
    
    void WASMPlatform::loadFile(const char *filePath, util::callback<void(std::unique_ptr<std::uint8_t[]> &&data, std::size_t size)> &&completion) {
        using cbtype = util::callback<void(std::unique_ptr<std::uint8_t[]> &&, std::size_t)>;
        const std::string fullPath = _dataPath + filePath;
        const std::size_t len = fullPath.length() + 1;
        
        std::uint8_t *block = reinterpret_cast<std::uint8_t *>(malloc(sizeof(cbtype) + sizeof(std::uint16_t) * len));
        std::uint16_t *path = reinterpret_cast<std::uint16_t *>(block + sizeof(cbtype));
        
        for (std::size_t i = 0; i < len; i++) {
            path[i] = fullPath[i];
        }
        
        cbtype *cb = reinterpret_cast<cbtype *>(block);
        new (cb) cbtype(std::move(completion));
        
        js_fetch(path, len);
    }
    void WASMPlatform::saveFile(const char *filePath, const std::uint8_t *data, std::size_t size, util::callback<void(bool)> &&completion) {
        using cbtype = util::callback<void(bool)>;
        const std::size_t pathLen = std::strlen(filePath) + 1;
        
        std::uint8_t *block = reinterpret_cast<std::uint8_t *>(malloc(sizeof(cbtype) + sizeof(std::uint16_t) * pathLen));
        std::uint16_t *path = reinterpret_cast<std::uint16_t *>(block + sizeof(cbtype));
        
        for (std::size_t i = 0; i < pathLen; i++) {
            path[i] = filePath[i];
        }
        
        std::uint8_t *fileData = nullptr;
        
        if (data && size) {
            fileData = reinterpret_cast<std::uint8_t *>(malloc(sizeof(std::uint16_t) * size));
            std::memcpy(fileData, data, size);
        }
        
        cbtype *cb = reinterpret_cast<cbtype *>(block);
        new (cb) cbtype(std::move(completion));
        
        js_save(path, pathLen, fileData, size);
    }
    
    float WASMPlatform::getScreenWidth() const {
        return g_width;
    }
    float WASMPlatform::getScreenHeight() const {
        return g_height;
    }
    
    void *WASMPlatform::attachNativeRenderingContext(void *context) {
        return nullptr;
    }
    
    void WASMPlatform::showCursor() {}
    void WASMPlatform::hideCursor() {}
    
    void WASMPlatform::showKeyboard() {}
    void WASMPlatform::hideKeyboard() {}
    
    void WASMPlatform::sendEditorMsg(const std::string &msg, const std::string &data) {
        js_editorMsg(msg.c_str(), msg.length(), data.c_str(), data.length());
    }
    void WASMPlatform::editorLoopbackMsg(const std::string &msg, const std::string &data) {
        for (auto &index : g_editorHandlers) {
            if (index.second(msg, data)) {
                break;
            }
        }
    }
    
    EventHandlerToken WASMPlatform::addEditorEventHandler(util::callback<bool(const std::string &, const std::string &)> &&handler, bool setTop) {
        EventHandlerToken token = g_tokenCounter++;
        if (setTop) {
            g_editorHandlers.emplace_front(std::make_pair(token, std::move(handler)));
        }
        else {
            g_editorHandlers.emplace_back(std::make_pair(token, std::move(handler)));
        }
        return token;
    }
    EventHandlerToken WASMPlatform::addKeyboardEventHandler(util::callback<bool(const PlatformKeyboardEventArgs &)> &&handler, bool setTop) {
        EventHandlerToken token = g_tokenCounter++;
        if (setTop) {
            g_keyboardHandlers.emplace_front(std::make_pair(token, std::move(handler)));
        }
        else {
            g_keyboardHandlers.emplace_back(std::make_pair(token, std::move(handler)));
        }
        return token;
    }
    EventHandlerToken WASMPlatform::addInputEventHandler(util::callback<bool(const char(&utf8char)[4])> &&input, bool setTop) {
        return nullptr;
    }
    EventHandlerToken WASMPlatform::addPointerEventHandler(util::callback<bool(const PlatformPointerEventArgs &)> &&handler, bool setTop) {
        EventHandlerToken token = g_tokenCounter++;
        if (setTop) {
            g_pointerHandlers.emplace_front(std::make_pair(token, std::move(handler)));
        }
        else {
            g_pointerHandlers.emplace_back(std::make_pair(token, std::move(handler)));
        }
        return token;
    }
    EventHandlerToken WASMPlatform::addGamepadEventHandler(util::callback<bool(const PlatformGamepadEventArgs &)> &&handler, bool setTop) {
        return nullptr;
    }
    
    void WASMPlatform::removeEventHandler(EventHandlerToken token) {
        for (auto index = g_editorHandlers.begin(); index != g_editorHandlers.end(); ++index) {
            if (index->first == token) {
                g_editorHandlers.erase(index);
                return;
            }
        }
        for (auto index = g_pointerHandlers.begin(); index != g_pointerHandlers.end(); ++index) {
            if (index->first == token) {
                g_pointerHandlers.erase(index);
                return;
            }
        }
        for (auto index = g_keyboardHandlers.begin(); index != g_keyboardHandlers.end(); ++index) {
            if (index->first == token) {
                g_keyboardHandlers.erase(index);
                return;
            }
        }
    }
    
    void WASMPlatform::setLoop(util::callback<void(float)> &&updateAndDraw) {
        g_updateAndDraw = std::move(updateAndDraw);
    }
    
    void WASMPlatform::setResizeHandler(util::callback<void()> &&handler) {
        g_resizeHandler = std::move(handler);
    }
    
    void WASMPlatform::exit() {

    }
    
    void WASMPlatform::logMsg(const char *fmt, ...) {
        va_list arglist;
        va_start(arglist, fmt);
        _output(false, fmt, arglist);
        va_end(arglist);
    }
    void WASMPlatform::logError(const char *fmt, ...) {
        va_list arglist;
        va_start(arglist, fmt);
        _output(true, fmt, arglist);
        va_end(arglist);
    }
    
    void WASMPlatform::_output(bool error, const char *fmt, va_list arglist) {
        auto msgFormat = [](std::uint16_t *output, const char *fmt, va_list arglist) {
            const std::uint16_t *start = output;
            while (*fmt != '\0') {
                if (*fmt == '%') {
                    if (*++fmt == 'p') {
                        const void *p = va_arg(arglist, void *);
                        output += util::strstream::ptow(output, p);
                    }
                    else if (*fmt == 'd') {
                        const std::intptr_t d = va_arg(arglist, std::intptr_t);
                        output += util::strstream::ltow(output, d);
                    }
                    else if (*fmt == 'f') {
                        const double f = va_arg(arglist, double);
                        output += util::strstream::ftow(output, f);
                    }
                    else if (*fmt == 's') {
                        const char *s = va_arg(arglist, const char *);
                        while (*s != 0) {
                            *output++ = *s++;
                        }
                    }
                    else {
                        *output++ = *fmt;
                    }
                }
                else {
                    *output++ = *fmt;
                }
                fmt++;
            }
            *output++ = 0;
            return output - start;
        };
        auto msgLength = [](const char *fmt, va_list arglist) {
            std::size_t result = 0;
            while (*fmt != '\0') {
                if (*fmt == '%') {
                    if (*++fmt == 'p') {
                        va_arg(arglist, void *);
                        result += sizeof(std::size_t);
                    }
                    else if (*fmt == 'd') {
                        va_arg(arglist, std::intptr_t);
                        result += 32;
                    }
                    else if (*fmt == 'f') {
                        va_arg(arglist, double);
                        result += 48;
                    }
                    else if (*fmt == 's') {
                        const char *s = va_arg(arglist, const char *);
                        result += (strlen(s) + 63) & ~std::size_t(63);
                    }
                    else {
                        result++;
                    }
                }
                else {
                    result++;
                }
                fmt++;
            }
            
            return result;
        };
        
        const std::size_t capacity = msgLength(fmt, arglist);
        const std::unique_ptr<std::uint16_t[]> logBuffer = std::make_unique<std::uint16_t[]>(capacity);
        const std::size_t length = msgFormat(logBuffer.get(), fmt, arglist);
        js_log(logBuffer.get(), length);
    }
}

// Exported to JS
extern "C" {
    void updateFrame(float dt) {
        if (g_updateAndDraw) {
            g_updateAndDraw(dt);
        }
    }
    void resized(std::uint32_t w, std::uint32_t h) {
        g_width = w;
        g_height = h;
        
        if (g_resizeHandler) {
            g_resizeHandler();
        }
    }
    void fileLoaded(std::uint16_t *block, std::size_t pathLen, std::uint8_t *data, std::size_t length) {
        using cbtype = util::callback<void(std::unique_ptr<std::uint8_t[]> &&, std::size_t)>;
        cbtype *cb = reinterpret_cast<cbtype *>(reinterpret_cast<std::uint8_t *>(block) - sizeof(cbtype));
        cb->operator()(std::unique_ptr<std::uint8_t[]>(data), length);
        cb->~cbtype();
        ::free(cb);
    }
    void fileSaved(std::uint16_t *block, std::size_t pathLen, int result) {
        using cbtype = util::callback<void(bool)>;
        cbtype *cb = reinterpret_cast<cbtype *>(reinterpret_cast<std::uint8_t *>(block) - sizeof(cbtype));
        cb->operator()(bool(result));
        cb->~cbtype();
        ::free(cb);
    }
    void taskExecute(foundation::AsyncTask *ptr) {
        // TODO:
        // Migrate to WASI-SDK (wasm32-wasi-threads)
    }
    void taskComplete(foundation::AsyncTask *ptr) {
        ptr->executeInBackground();
        ptr->executeInMainThread();
        delete ptr;
    }
    void editorEvent(const char *msg, std::size_t msglen, const char *data, std::size_t datalen) {
        if (auto p = g_instance.lock()) {
            std::string amsg = std::string(msg, msglen);
            std::string adata = std::string(data, datalen);
            
            for (auto &index : g_editorHandlers) {
                if (index.second(amsg, adata)) {
                    break;
                }
            }
        }
    }
    void pointerEvent(foundation::PlatformPointerEventArgs::EventType type, std::uint32_t id, float x, float y) {
        if (auto p = g_instance.lock()) {
            foundation::PlatformPointerEventArgs args = {
                .type = type,
                .pointerID = id,
                .coordinateX = x,
                .coordinateY = y
            };
            for (auto &index : g_pointerHandlers) {
                if (index.second(args)) {
                    break;
                }
            }
        }
    }
    void keyboardEvent(foundation::PlatformKeyboardEventArgs::EventType type, foundation::Key key) {
        if (auto p = g_instance.lock()) {
            foundation::PlatformKeyboardEventArgs args = {
                .type = type,
                .key = key
            };
            for (auto &index : g_keyboardHandlers) {
                if (index.second(args)) {
                    break;
                }
            }
        }
    }
    void wheelEvent(std::int16_t increment) {
        if (auto p = g_instance.lock()) {
            foundation::PlatformPointerEventArgs args = {
                .type = foundation::PlatformPointerEventArgs::EventType::WHEEL,
                .pointerID = 0,
                .coordinateX = 0.0f,
                .coordinateY = 0.0f
            };
            args.flags.wheel = increment;
            for (auto &index : g_pointerHandlers) {
                if (index.second(args)) {
                    break;
                }
            }
        }
    }
}

namespace foundation {
    std::shared_ptr<PlatformInterface> PlatformInterface::instance() {
        std::shared_ptr<PlatformInterface> result;
        
        if (g_instance.use_count() == 0) {
            g_instance = result = std::make_shared<WASMPlatform>();
        }
        else {
            result = g_instance.lock();
        }
        
        return result;
    }
}

#endif // PLATFORM_WASM

