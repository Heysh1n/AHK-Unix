#pragma once

#include <string>
#include <unordered_map>

namespace ahk {

class LayoutProfile {
public:
    static LayoutProfile russian_qwerty();

    int key_for_utf8_char(const std::string& ch) const;

private:
    std::unordered_map<std::string, int> char_to_key_;
};

} // namespace ahk
