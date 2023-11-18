
// TODO:
// 

#ifdef PLATFORM_WASM
#include "platform_wasm.h"

#include <stdarg.h>
#include <cstdio>
#include <pthread.h>

namespace {
    const std::size_t PAGESIZE = 65536;

    std::size_t g_currentMemoryPages = __builtin_wasm_memory_size(0);
    std::size_t g_currentMemoryOffset = g_currentMemoryPages * PAGESIZE;
    std::atomic<bool> g_memoryLock {false};
}

// These function shouldn't be called
extern "C" {
    void *realloc(void *ptr, size_t size) {
        abort();
    }
    void *aligned_alloc(size_t alignment, size_t size) {
        abort();
    }
}

// From js
extern "C" {
    void js_waiting();
    void js_log(const std::uint16_t *ptr, int length);
    void js_task(const void *ptr);
    void js_fetch(const void *ptr, int pathLen);

    float js_canvas_width();
    float js_canvas_height();
}

// Missing part of runtime
extern "C" {
    int __cxa_atexit(void (*func) (void *), void * arg, void * dso_handle) {
        return 0;
    }
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
                abort();
            }
            
            g_currentMemoryPages += incMemoryPages;
        }
        
        return (void *)oldOffset;
    }
        
    void *__real_malloc(size_t size);
    void *__wrap_malloc(size_t size) {
        while (g_memoryLock.exchange(true) == true) {
            js_waiting();
        }
        void *result = __real_malloc(size);
        g_memoryLock.store(false);
        return result;
    }
    void __real_free(void *ptr);
    void __wrap_free(void *ptr) {
        if (ptr) {
            while (g_memoryLock.exchange(true) == true) {
                js_waiting();
            }
            __real_free(ptr);
            g_memoryLock.store(false);
        }
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
}

// Message formatting
namespace {
    const char alphabet[] = "0123456789ABCDEF";
    const std::size_t CONVERSION_BUFFER_MAX = 32;
    
    std::size_t align64(std::size_t len) {
        return (len + 63) & ~std::size_t(63);
    }
    std::size_t ptoa(std::uint16_t *output, const void *p) {
        std::uint16_t *buffer = output + sizeof(std::size_t) - 1;
        std::size_t n = std::size_t(p);
        
        for (int i = 0; i < sizeof(std::size_t); i++) {
            *buffer-- = alphabet[n % 16];
            n /= 16;
        }
        
        return sizeof(std::size_t);
    }
    std::size_t ntoa(std::uint16_t *output, std::intptr_t n, int base) {
        std::intptr_t an = std::abs(n);
        std::uint16_t buffer[CONVERSION_BUFFER_MAX];
        std::uint16_t *ptr = buffer + CONVERSION_BUFFER_MAX;
        do {
            *--ptr = alphabet[an % base];
            an /= base;
        }
        while (an != 0);
        if (n < 0) {
            *--ptr = '-';
        }
        
        const std::size_t tocopy = buffer + CONVERSION_BUFFER_MAX - ptr;
        memcpy(output, ptr, sizeof(std::uint16_t) * tocopy);
        return tocopy;
    }
    std::size_t ftoa(std::uint16_t *output, double f) {
        const double af = std::abs(f);
        std::int64_t ipart = std::int64_t(af);
        std::int64_t fpart = std::int64_t(10000.0 * (af - double(ipart)));
        
        std::uint16_t buffer[CONVERSION_BUFFER_MAX];
        std::uint16_t *ptr = buffer + CONVERSION_BUFFER_MAX;
        int fleft = 4;
        do {
            std::int64_t digit = fpart % 10;
            *--ptr = alphabet[digit];
            fpart /= 10;
        }
        while (--fleft);
        *--ptr = '.';
        do {
            *--ptr = alphabet[ipart % 10];
            ipart /= 10;
        }
        while (ipart != 0);
        if (f < 0.0f) {
            *--ptr = '-';
        }
        const std::size_t tocopy = buffer + CONVERSION_BUFFER_MAX - ptr;
        memcpy(output, ptr, sizeof(std::uint16_t) * tocopy);
        return tocopy;
    }
    std::size_t msgFormat(std::uint16_t *output, const char *fmt, va_list arglist) {
        const std::uint16_t *start = output;
        while (*fmt != '\0') {
            if (*fmt == '%') {
                if (*++fmt == 'p') {
                    const void *p = va_arg(arglist, void *);
                    output += ptoa(output, p);
                }
                else if (*fmt == 'd') {
                    const std::intptr_t d = va_arg(arglist, std::intptr_t);
                    output += ntoa(output, d, 10);
                }
                else if (*fmt == 'f') {
                    const double f = va_arg(arglist, double);
                    output += ftoa(output, f);
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
    }
    std::size_t msgLength(const char *fmt, va_list arglist) {
        std::size_t result = 0;
        while (*fmt != '\0') {
            if (*fmt == '%') {
                if (*++fmt == 'p') {
                    va_arg(arglist, void *);
                    result += sizeof(std::size_t);
                }
                else if (*fmt == 'd') {
                    va_arg(arglist, std::intptr_t);
                    result += CONVERSION_BUFFER_MAX;
                }
                else if (*fmt == 'f') {
                    va_arg(arglist, double);
                    result += CONVERSION_BUFFER_MAX;
                }
                else if (*fmt == 's') {
                    const char *s = va_arg(arglist, const char *);
                    result += align64(int(strlen(s)));
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
    }
}

namespace {
    std::weak_ptr<foundation::PlatformInterface> g_instance;
    util::callback<void(float)> g_updateAndDraw;
}

namespace foundation {
    WASMPlatform::WASMPlatform() {
        
    }
    WASMPlatform::~WASMPlatform() {
        
    }
    
    void WASMPlatform::executeAsync(std::unique_ptr<AsyncTask> &&task) {
        js_task(task.release());
    }
    void WASMPlatform::loadFile(const char *filePath, util::callback<void(std::unique_ptr<std::uint8_t[]> &&data, std::size_t size)> &&completion) {
        using cbtype = util::callback<void(std::unique_ptr<std::uint8_t[]> &&, std::size_t)>;
        const std::string fullPath = "data/" + std::string(filePath);
        const std::size_t len = fullPath.length() + 1;
        std::uint16_t *block = reinterpret_cast<std::uint16_t *>(malloc(sizeof(std::uint16_t) * len + sizeof(cbtype)));
        
        for (std::size_t i = 0; i < len; i++) {
            block[i] = fullPath[i];
        }
        
        cbtype *cb = reinterpret_cast<cbtype *>(block + len);
        new (cb) cbtype(std::move(completion));
        js_fetch(block, len);
    }
    
    float WASMPlatform::getScreenWidth() const {
        return js_canvas_width();
    }
    float WASMPlatform::getScreenHeight() const {
        return js_canvas_height();
    }
    
    void *WASMPlatform::attachNativeRenderingContext(void *context) {
        return nullptr;
    }
    
    void WASMPlatform::showCursor() {}
    void WASMPlatform::hideCursor() {}
    
    void WASMPlatform::showKeyboard() {}
    void WASMPlatform::hideKeyboard() {}
    
    EventHandlerToken WASMPlatform::addKeyboardEventHandler(util::callback<void(const PlatformKeyboardEventArgs &)> &&handler) {
        return nullptr;
    }
    
    EventHandlerToken WASMPlatform::addInputEventHandler(util::callback<void(const char(&utf8char)[4])> &&input, util::callback<void()> &&backspace) {
        return nullptr;
    }
    
    EventHandlerToken WASMPlatform::addPointerEventHandler(util::callback<bool(const PlatformPointerEventArgs &)> &&handler) {
        return nullptr;
    }
    
    EventHandlerToken WASMPlatform::addGamepadEventHandler(util::callback<void(const PlatformGamepadEventArgs &)> &&handler) {
        return nullptr;
    }
    
    void WASMPlatform::removeEventHandler(EventHandlerToken token) {

    }
    
    void WASMPlatform::setLoop(util::callback<void(float)> &&updateAndDraw) {
        g_updateAndDraw = std::move(updateAndDraw);
    }
    
    void WASMPlatform::exit() {

    }
    
    void WASMPlatform::logMsg(const char *fmt, ...) {
        va_list arglist;
        va_start(arglist, fmt);
        const std::size_t capacity = msgLength(fmt, arglist);
        std::uint16_t *logBuffer = reinterpret_cast<std::uint16_t *>(malloc(sizeof(std::uint16_t) * capacity));
        va_end(arglist);
        va_start(arglist, fmt);
        const std::size_t length = msgFormat(logBuffer, fmt, arglist);
        va_end(arglist);
        js_log(logBuffer, length);
    }
    void WASMPlatform::logError(const char *fmt, ...) {
        va_list arglist;
        va_start(arglist, fmt);
        const std::size_t capacity = msgLength(fmt, arglist);
        std::uint16_t *logBuffer = reinterpret_cast<std::uint16_t *>(malloc(sizeof(std::uint16_t) * capacity));
        va_end(arglist);
        va_start(arglist, fmt);
        const std::size_t length = msgFormat(logBuffer, fmt, arglist);
        va_end(arglist);
        js_log(logBuffer, length);
        abort();
    }
}

extern "C" {
    void frame(float dt) {
        if (g_updateAndDraw) {
            g_updateAndDraw(dt);
        }
    }
    
    void file(std::uint16_t *block, std::size_t pathLen, std::uint8_t *data, std::size_t length) {
        using cbtype = util::callback<void(std::unique_ptr<std::uint8_t[]> &&, std::size_t)>;
        cbtype *cb = reinterpret_cast<cbtype *>(block + pathLen);
        cb->operator()(std::unique_ptr<std::uint8_t[]>(data), length);
        cb->~cbtype();
        ::free(block);
    }
    
    void task_execute(foundation::AsyncTask *ptr) {
        ptr->executeInBackground();
    }
    
    void task_complete(foundation::AsyncTask *ptr) {
        ptr->executeInMainThread();
        delete ptr;
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

