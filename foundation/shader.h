
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include "math.h"

namespace foundation {
    struct InputLayout;
    std::tuple<std::string, std::string, std::uint32_t> makePlatformShaderSource(const char *src, const InputLayout &layout, std::string &error);
}

