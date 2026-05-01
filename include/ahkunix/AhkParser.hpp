#pragma once

#include "ahkunix/Hotstring.hpp"
#include "ahkunix/LayoutProfile.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace ahk
{
    class AhkParser
    {
    public:
        static Hotstring parse_hotstring_line(const std::string &line, const LayoutProfile &layout);
        static std::vector<Hotstring> parse_file(const std::filesystem::path &path, const LayoutProfile &layout, bool strict_mode = false);

    private:
        static std::vector<std::string> split_utf8(const std::string &s);
        static void parse_replacement(const std::string &replacement, Hotstring &hs);

        static cmd::CommandList parse_action_block(const std::string &code_block, bool strict_mode, const std::string &origin);
        static std::pair<std::vector<int>, std::vector<int>> parse_trigger_expr(const std::string &expr, const LayoutProfile &layout);
    };
} // namespace ahk