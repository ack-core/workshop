
#pragma once

#include <cstddef>
#include <memory>

namespace foundation {
    // Interface provides low-level core methods
    //
    class SoundInterface {
    public:
        static std::shared_ptr<SoundInterface> instance();

    public:

    protected:
        virtual ~SoundInterface() = default;
    };
}
