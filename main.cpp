
#include "platform/interfaces.h"

int main(int argc, const char * argv[]) {
    auto platform = foundation::PlatformInterface::instance();

    platform->run([](float dtSec) {

    });

    return 0;
}

