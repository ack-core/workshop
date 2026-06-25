
#include "utils.h"

namespace editor {
    RandomSource::RandomSource(std::uint64_t seed, std::uint64_t seq) {
        _state = 0U;
        _inc = (seq << 1u) | 1u;
        getNextRandom();
        _state += seed;
        getNextRandom();
    }
    std::uint32_t RandomSource::getNextRandom() {
        uint64_t oldstate = _state;
        _state = oldstate * 6364136223846793005ULL + (_inc | 1);
        uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
        uint32_t rot = oldstate >> 59u;
        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }

    RandomSource g_defaultRandomSource = RandomSource(100, 100);

    std::string getRandomName() {
        std::string result = "aaaaaa";
        for (auto &ch : result) {
            ch = "abcdefghijklmnopqrstuvwxyz"[g_defaultRandomSource.getNextRandom() % 26];
        }
        return result;
    }
    
    std::string getParentName(const std::string &nodeName) {
        std::size_t pos = nodeName.rfind('.');
        if (pos == std::string::npos)
            return "";

        return nodeName.substr(0, pos);
    }
    
    std::pair<std::unique_ptr<std::uint8_t[]>, std::size_t> writeTGA(const std::uint8_t *rgbaData, std::size_t width, std::size_t height) {
        std::size_t totalSize = 18 + width * height * 4;
        std::unique_ptr<std::uint8_t[]> tga = std::make_unique<std::uint8_t[]>(totalSize);
        std::uint8_t *header = tga.get();
        std::memset(header, 0, 18);
        
        header[2]  = 2;              // image type: uncompressed true-color
        header[12] = width  & 0xFF;  // width low byte
        header[13] = width  >> 8;    // width high byte
        header[14] = height & 0xFF;  // height low byte
        header[15] = height >> 8;    // height high byte
        header[16] = 32;             // bits per pixel
        header[17] = 0x20;           // image descriptor: origin top-left
        
        std::uint8_t *out = tga.get() + 18;
        
        for (std::size_t i = 0; i < width * height; i++) {
            out[i * 4 + 0] = rgbaData[i * 4 + 2];
            out[i * 4 + 1] = rgbaData[i * 4 + 1];
            out[i * 4 + 2] = rgbaData[i * 4 + 0];
            out[i * 4 + 3] = rgbaData[i * 4 + 3];
        }

        return std::make_pair(std::move(tga), totalSize);
    }
}
