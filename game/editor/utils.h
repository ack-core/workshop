
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <foundation/util.h>

// TODO: utils -> editor_utils

namespace editor {
    std::string getRandomName();
    std::string getParentName(const std::string &nodeName);
}
