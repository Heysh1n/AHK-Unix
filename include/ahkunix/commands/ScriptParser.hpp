#pragma once

#include "ahkunix/commands/Command.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace ahk::cmd
{
    class ScriptParser
    {
    public:
        static CommandList parse_action_block(const std::string &block, bool strict_mode = false, const std::string &origin = "");

    private:
        static std::vector<std::string> preprocess_lines(const std::string &block, bool strict_mode, const std::string &origin);
        static CommandList parse_block_lines(const std::vector<std::string> &lines, std::size_t &idx, bool stop_at_closing_brace);
        static CommandPtr parse_simple_command(const std::string &line);
        static CommandPtr parse_if_chain(const std::vector<std::string> &lines, std::size_t &idx);
        static bool starts_with_ci(const std::string &text, const std::string &prefix);
    };
} // namespace ahk::cmd