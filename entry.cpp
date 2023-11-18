
#include "foundation/platform.h"
#include <csetjmp>

foundation::PlatformInterfacePtr platform;

struct AsyncContext {
    int result;
};

extern "C" void initialize() {
    platform = foundation::PlatformInterface::instance();
    platform->logMsg("[PLATFORM] initializing...");
    platform->loadFile("test1.txt", [p = platform](std::unique_ptr<std::uint8_t[]> &&data, std::size_t size) {
        if (data) {
            p->logMsg("-->>> %d %s", int(size), data.get());
        }
    });
    platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([p = platform, x = 10, y = 20](AsyncContext &ctx) {
        ctx.result = x + y;
        p->logMsg("-->>> working! %d %d\n", x, y);
    },
    [p = platform](AsyncContext &ctx) {
        p->logMsg("-->>> result! %d\n", ctx.result);
    }));
    
    platform->setLoop([](float dtSec) {
        platform->logMsg("-->> %d %d %f", (int)platform->getScreenWidth(), (int)platform->getScreenHeight(), dtSec);
    });
}

extern "C" void deinitialize() {

}
