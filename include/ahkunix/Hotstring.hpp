#pragma once

#include <string>
#include <utility>
#include <vector>
#include <memory>

namespace ahk::cmd
{
    class Command;
    class Context;
    using CommandPtr = std::shared_ptr<Command>;
    using CommandList = std::vector<CommandPtr>;
}

namespace ahk
{

    struct Hotstring
    {
        std::vector<int> trigger_keys;
        std::vector<int> trigger_modifiers; // Alt, Ctrl, Shift, Meta keys
        bool require_all_modifiers = true;  // If true, require ALL modifiers; if false, trigger on base key alone
        bool erase_trigger = true; // true for text hotstrings, false for key hotkeys (NumPad/F keys etc.)
        // NEW: List of commands instead of simple text replacement
        cmd::CommandList commands;

        // Shared context for random/variables across all commands in this hotstring
        mutable std::shared_ptr<cmd::Context> context;

        // Legacy fields (for backward compatibility)
        std::string replacement_utf8;
        std::vector<std::pair<int, int>> tail_keys;
    };

} // namespace ahk
