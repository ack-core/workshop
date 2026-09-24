
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include "math.h"

struct InputLayout;

namespace foundation {
    std::pair<std::string, std::string> makePlatformShaderSource(const char *src, const InputLayout &layout, std::string &error);
}

