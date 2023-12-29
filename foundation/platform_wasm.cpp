
// TODO:
// 

#ifdef PLATFORM_WASM
#include "platform_wasm.h"

#include <stdarg.h>
#include <cstdio>
#include <pthread.h>
#include <list>

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
    auto js_canvas_width() -> float;
    auto js_canvas_height() -> float;
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

// Message formatting
namespace {
    std::size_t align64(std::size_t len) {
        return (len + 63) & ~std::size_t(63);
    }
    std::size_t ptoa(std::uint16_t *p, const void *ptr) {
        const int len = 2 * sizeof(std::size_t);
        std::uint16_t *output = p + len - 1;
        std::size_t n = std::size_t(ptr);
        
        for (int i = 0; i < len; i++) {
            *output-- = "0123456789ABCDEF"[n % 16];
            n /= 16;
        }
        return len;
    }
    std::size_t ltoa(std::uint16_t* p, std::int64_t value) {
        std::int64_t absvalue = value < 0 ? -value : value;
        std::uint16_t *output = p;

        do {
            *output++ = "0123456789ABCDEF"[absvalue % 10];
            absvalue /= 10;
        }
        while (absvalue);

        if (value < 0) *output++ = '-';
        std::size_t result = output - p;
        output--;
        
        while(p < output) {
            char tmp = *output;
            *output-- = *p;
            *p++ = tmp;
        }

        return result;
    }
    std::size_t ftoa(std::uint16_t *p, double f) {
        std::uint16_t *output = p;
        const double af = std::abs(f);
        std::int64_t ipart = std::int64_t(af);
        double remainder = 1000000000.0 * (af - double(ipart));
        std::int64_t fpart = std::int64_t(remainder);
        
        if ((remainder - double(fpart)) > 0.5) {
            fpart++;
            if (fpart > 1000000000) {
                fpart = 0;
                ipart++;
            }
        }
        
        if (f < 0.0) *output++ = '-';
        output += ltoa(output, ipart);
        
        int width = 9;
        while (fpart % 10 == 0 && width-- > 0) {
            fpart /= 10;
        }
        output += width + 1;
        std::size_t result = output - p;
        while (width--) {
            *--output = fpart % 10 + '0';
            fpart /= 10;
        }
        *--output = '.';
        return result;
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
                    output += ltoa(output, d);
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
                    result += 32;
                }
                else if (*fmt == 'f') {
                    va_arg(arglist, double);
                    result += 48;
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
    struct Handler {
        foundation::EventHandlerToken token;
        util::callback<bool(const foundation::PlatformPointerEventArgs &)> handler;
    };
    
    std::weak_ptr<foundation::PlatformInterface> g_instance;
    std::list<Handler> g_pointerHandlers;

    util::callback<void(float)> g_updateAndDraw;
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
        EventHandlerToken token = g_tokenCounter++;
        g_pointerHandlers.emplace_back(Handler{token, std::move(handler)});
        return token;
    }
    
    EventHandlerToken WASMPlatform::addGamepadEventHandler(util::callback<void(const PlatformGamepadEventArgs &)> &&handler) {
        return nullptr;
    }
    
    void WASMPlatform::removeEventHandler(EventHandlerToken token) {
        for (auto index = g_pointerHandlers.begin(); index != g_pointerHandlers.end(); ++index) {
            if (index->token == token) {
                g_pointerHandlers.erase(index);
                return;
            }
        }
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
        //std::unique_ptr<std::uint16_t[]> logBuffer = std::make_unique<std::uint16_t[]>(capacity);
        std::uint16_t *logBuffer = new std::uint16_t [capacity];
        va_end(arglist);
        va_start(arglist, fmt);
        const std::size_t length = msgFormat(logBuffer, fmt, arglist);
        va_end(arglist);
        js_log(logBuffer, length);
        delete [] logBuffer;
    }
    void WASMPlatform::logError(const char *fmt, ...) {
        va_list arglist;
        va_start(arglist, fmt);
        const std::size_t capacity = msgLength(fmt, arglist);
        std::uint16_t *logBuffer = new std::uint16_t [capacity];
        va_end(arglist);
        va_start(arglist, fmt);
        const std::size_t length = msgFormat(logBuffer, fmt, arglist);
        va_end(arglist);
        js_log(logBuffer, length);
        delete [] logBuffer;
        abort();
    }
}

extern "C" {
    void updateFrame(float dt) {
        if (g_updateAndDraw) {
            g_updateAndDraw(dt);
        }
    }
    
    void fileLoaded(std::uint16_t *block, std::size_t pathLen, std::uint8_t *data, std::size_t length) {
        using cbtype = util::callback<void(std::unique_ptr<std::uint8_t[]> &&, std::size_t)>;
        cbtype *cb = reinterpret_cast<cbtype *>(block + pathLen);
        cb->operator()(std::unique_ptr<std::uint8_t[]>(data), length);
        cb->~cbtype();
        ::free(block);
    }
    
    void taskExecute(foundation::AsyncTask *ptr) {
        ptr->executeInBackground();
    }
    
    void taskComplete(foundation::AsyncTask *ptr) {
        ptr->executeInMainThread();
        delete ptr;
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
                if (index.handler(args)) {
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

