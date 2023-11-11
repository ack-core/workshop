
#include "foundation/platform.h"

foundation::PlatformInterfacePtr platform;

extern "C" void initialize() {
	//int* x0 = (int *)malloc(4096);
		// x0[100] = 999;
		// consoleLog(int(x0));
	//memset(x0, 0, 4096);

		// memcpy(x0, x0 + 100, 100);

    platform = foundation::PlatformInterface::instance();
    //platform->setLoop([&](float dtSec) {

	//memset(x0, 1, 4096);

    //});

    platform->logMsg("[WASM]_Hello %s -->>> %s -->>> %f", "ewfwegwegwegweggew", "foundation::PlatformInterface::instance", 66.11);
}

extern "C" void test() {
	//consoleLog(111);
}

extern "C" void deinitialize() {

}
