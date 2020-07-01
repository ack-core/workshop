
#include "interfaces.h"

#ifdef PLATFORM_IOS

#endif // PLATFORM_IOS

namespace foundation {
    std::shared_ptr<PlatformInterface> PlatformInterface::instance() {
        return {};
    }
}
