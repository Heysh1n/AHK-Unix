#pragma once

#include <string>
#include <utility>
#include <vector>

namespace ahk {

struct Hotstring {
    std::vector<int> trigger_keys;
    std::string replacement_utf8;
    std::vector<std::pair<int, int>> tail_keys;
};

} // namespace ahk
