
#pragma once

#include <cstddef>
#include <memory>
#include <string>

// TODO: utils -> editor_utils

namespace editor {
    struct RandomSource {
        RandomSource() {}
        RandomSource(std::uint64_t seed, std::uint64_t seq);
        std::uint32_t getNextRandom();
        
        std::uint64_t _state = 0;
        std::uint64_t _inc = 0;
    };

    std::string getRandomName();
    std::string getParentName(const std::string &nodeName);
    std::pair<std::unique_ptr<std::uint8_t[]>, std::size_t> writeTGA(const std::uint8_t *rgbaData, std::size_t width, std::size_t height);
}
