
#include "platform/interfaces.h"

int main(int argc, const char * argv[]) {
    auto platform = engine::Platform::instance();

    platform->run([](float dtSec) {

    });

    return 0;
}

