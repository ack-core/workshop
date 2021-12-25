
#include "platform.h"

#ifdef PLATFORM_IOS

namespace foundation {

}

namespace foundation {
    std::shared_ptr<PlatformInterface> PlatformInterface::instance() {
        return {};
    }
}

#endif // PLATFORM_IOS

