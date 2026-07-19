#pragma once

#include <string>
#include <unordered_map>

namespace ahk
{

    class LayoutProfile
    {
    public:
        static LayoutProfile russian_qwerty();

        int key_for_utf8_char(const std::string &ch) const;

        // Parse special key names like "NumPad1", "Space", "Enter", etc.
        static int parse_special_key(const std::string &key_name);
        static int parse_modifier_key(const std::string &mod_name);

    private:
        std::unordered_map<std::string, int> char_to_key_;
    };

} // namespace ahk
