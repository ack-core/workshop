
#include "foundation/platform.h"

foundation::PlatformInterfacePtr platform;
int dbg_loading = 0;

extern "C" void initialize() {
    platform = foundation::PlatformInterface::instance();
    platform->logMsg("[PLATFORM] initializing...");
//    platform->loadFile("textures/ui/panel.png", [p = platform](std::unique_ptr<std::uint8_t[]> &&data, std::size_t size) {
//        if (data) {
//            p->logMsg("-->>> yep 1\n");
//        }
//    });
    platform->setLoop([&](float dtSec) {
//        if (dbg_loading == 0) {
//            dbg_loading = 1;
//            
//            platform->loadFile("arial.ttf", [](std::unique_ptr<std::uint8_t[]> &&data, std::size_t size) {
//                if (data) {
//                    platform->logMsg("-->>> yep2\n");
//                    dbg_loading = 2;
//                }
//            });
//        }
//        
//        if (dbg_loading == 2) {
//            dbg_loading = 0;
//        }
    });
}

extern "C" void deinitialize() {
    platform->logMsg("[Platform] deinitializing...");
}
