
#include "utils.h"

namespace editor {
    util::RandomSource g_defaultRandomSource = util::RandomSource(100, 100);

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
}
