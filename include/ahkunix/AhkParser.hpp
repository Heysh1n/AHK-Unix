#pragma once

#include "ahkunix/Hotstring.hpp"
#include "ahkunix/LayoutProfile.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace ahk {

class AhkParser {
public:
    static Hotstring parse_hotstring_line(const std::string& line, const LayoutProfile& layout);
    static std::vector<Hotstring> parse_file(const std::filesystem::path& path, const LayoutProfile& layout);

private:
    static std::vector<std::string> split_utf8(const std::string& s);
    static void parse_replacement(const std::string& replacement, Hotstring& hs);
};

} // namespace ahk
