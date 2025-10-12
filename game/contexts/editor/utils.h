
#pragma once

#include <cstddef>
#include <memory>

namespace editor {
    std::pair<std::unique_ptr<std::uint8_t[]>, std::size_t> writeTGA(const std::uint8_t *rgbaData, std::size_t width, std::size_t height);
}
