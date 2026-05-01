#include "ahkunix/AhkParser.hpp"

#include "ahkunix/Errors.hpp"
#include "ahkunix/StringUtil.hpp"
#include "ahkunix/commands/ScriptParser.hpp"

#include <linux/input-event-codes.h>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace ahk
{

    using namespace cmd;

    std::pair<std::vector<int>, std::vector<int>> AhkParser::parse_trigger_expr(const std::string &expr, const LayoutProfile &layout)
    {
        std::vector<int> trigger_keys;
        std::vector<int> modifiers;
        
        // Check for modifier syntax: "Mod & Key"
        const auto ampersand = expr.find('&');
        if (ampersand != std::string::npos)
        {
            // Parse modifier part
            std::string mod_part = expr.substr(0, ampersand);
            mod_part = trim(mod_part);
            
            if (!mod_part.empty())
            {
                try
                {
                    modifiers.push_back(LayoutProfile::parse_modifier_key(mod_part));
                }
                catch (...)
                {
                    // If not a single modifier, try parsing as special key
                    // In case like "Alt & NumPad9", we ignore the issue for now
                }
            }

            // Parse key part after &
            std::string key_part = expr.substr(ampersand + 1);
            key_part = trim(key_part);

            // Try special key first
            try
            {
                trigger_keys.push_back(LayoutProfile::parse_special_key(key_part));
                return {trigger_keys, modifiers};
            }
            catch (...)
            {
                // Fall through to UTF-8 parsing
            }

            // Try UTF-8 character
            if (key_part.size() == 1 && static_cast<unsigned char>(key_part[0]) < 128)
            {
                try
                {
                    trigger_keys.push_back(layout.key_for_utf8_char(key_part));
                }
                catch (...)
                {
                    throw std::runtime_error("cannot parse key after &: " + key_part);
                }
            }
            else
            {
                auto chars = parse_trigger_expr(key_part, layout).first;
                trigger_keys.insert(trigger_keys.end(), chars.begin(), chars.end());
            }

            return {trigger_keys, modifiers};
        }

        // Try special key first
        try
        {
            trigger_keys.push_back(LayoutProfile::parse_special_key(expr));
            return {trigger_keys, modifiers};
        }
        catch (...)
        {
            // Fall through to UTF-8 parsing
        }

        // Try UTF-8 characters
        for (const auto &ch : split_utf8(expr))
        {
            try
            {
                trigger_keys.push_back(layout.key_for_utf8_char(ch));
            }
            catch (...)
            {
                throw std::runtime_error("cannot parse trigger: " + expr);
            }
        }

        return {trigger_keys, modifiers};
    }

    Hotstring AhkParser::parse_hotstring_line(const std::string &line, const LayoutProfile &layout)
    {
        Hotstring hs;
        std::string trigger;
        std::string replacement;

        if (line.starts_with(":"))
        {
            // Old format: ":?*:trigger::replacement"
            const auto first_separator = line.find("::");
            if (first_separator == std::string::npos)
            {
                throw std::runtime_error("AHK hotstring has no trigger separator");
            }

            const auto trigger_start = line.rfind(':', first_separator - 1);
            if (trigger_start == std::string::npos)
            {
                throw std::runtime_error("AHK hotstring has malformed options");
            }

            trigger = line.substr(trigger_start + 1, first_separator - trigger_start - 1);
            replacement = line.substr(first_separator + 2);
        }
        else
        {
            // New format: "Key::replacement" or "Mod & Key::replacement"
            const auto separator = line.find("::");
            if (separator == std::string::npos)
            {
                throw std::runtime_error("AHK hotstring has no trigger separator");
            }

            trigger = line.substr(0, separator);
            replacement = line.substr(separator + 2);
        }
        trigger = trim(trigger);
        auto [trigger_keys, modifiers] = parse_trigger_expr(trigger, layout);
        hs.trigger_keys = std::move(trigger_keys);
        hs.trigger_modifiers = std::move(modifiers);
        hs.erase_trigger = line.starts_with(":"); // old :*?:... syntax only

        parse_replacement(replacement, hs);
        return hs;
    }
    

std::vector<Hotstring> AhkParser::parse_file(const std::filesystem::path &path, const LayoutProfile &layout, bool strict_mode)
{
    std::ifstream in(path);
    if (!in)
    {
        throw SysError("open script " + path.string());
    }

    std::vector<std::string> raw_lines;
    {
        std::string l;
        while (std::getline(in, l))
        {
            raw_lines.push_back(l);
        }
    }

    auto is_comment_or_empty = [](const std::string &line) {
        const std::string t = trim(line);
        return t.empty() || t.starts_with(";") || t.starts_with("#");
    };

    auto starts_with_ci = [](const std::string &text, const std::string &prefix) {
        if (text.size() < prefix.size()) return false;
        for (std::size_t i = 0; i < prefix.size(); ++i) {
            const auto a = static_cast<unsigned char>(text[i]);
            const auto b = static_cast<unsigned char>(prefix[i]);
            if (std::tolower(a) != std::tolower(b)) return false;
        }
        return true;
    };

    auto looks_like_trigger_line = [&](const std::string &line) {
        std::string t = trim(line);
        if (t.empty()) return false;

        // Never treat command lines as trigger lines, even if they contain "::"
        if (starts_with_ci(t, "SendInput") ||
            starts_with_ci(t, "Sleep") ||
            starts_with_ci(t, "Random") ||
            starts_with_ci(t, "If") ||
            starts_with_ci(t, "Else") ||
            starts_with_ci(t, "SendMessage") ||
            starts_with_ci(t, "Input") ||
            starts_with_ci(t, "Return") ||
            t == "{" || t == "}") {
            return false;
        }

        // Old hotstring format
        if (t.starts_with(":")) {
            return t.find("::") != std::string::npos;
        }

        // New hotkey format
        const auto sep = t.find("::");
        if (sep == std::string::npos) return false;

        const std::string lhs = trim(t.substr(0, sep));
        if (lhs.empty()) return false;

        // Reject obvious non-trigger fragments
        if (lhs.find('{') != std::string::npos ||
            lhs.find('}') != std::string::npos ||
            lhs.find(',') != std::string::npos) {
            return false;
        }

        return true;
    };

        auto looks_like_action_line = [&](const std::string &line) {
            std::string t = trim(line);
            if (t.empty())
            {
                return false;
            }

            return starts_with_ci(t, "SendInput") ||
                starts_with_ci(t, "Sleep") ||
                starts_with_ci(t, "Random") ||
                starts_with_ci(t, "If") ||
                starts_with_ci(t, "Else") ||
                starts_with_ci(t, "SendMessage") ||
                starts_with_ci(t, "Input") ||
                starts_with_ci(t, "Return") ||
                t == "{" || t == "}";
        };

    
    std::vector<Hotstring> hotstrings;

    for (std::size_t i = 0; i < raw_lines.size(); ++i)
    {
        std::string line = trim(raw_lines[i]);
        if (is_comment_or_empty(line))
        {
            continue;
        }

        if (!looks_like_trigger_line(line))
        {
            continue;
        }

        Hotstring hs;
        try
        {
            hs = parse_hotstring_line(line, layout);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(path.string() + ":" + std::to_string(i + 1) + ": " + e.what());
        }

        // If replacement was inline, we're done.
        if (!hs.replacement_utf8.empty() || !hs.tail_keys.empty())
        {
            hotstrings.push_back(std::move(hs));
            continue;
        }

        // Collect multiline block after trigger:: until Return or next trigger.
        std::vector<std::string> block_lines;
        bool saw_return = false;
        int brace_depth = 0;
        std::size_t j = i + 1;

        for (; j < raw_lines.size(); ++j)
        {
            std::string t = trim(raw_lines[j]);

            if (t == "{")
            {
                ++brace_depth;
            }
            else if (t == "}" && brace_depth > 0)
            {
                --brace_depth;
            }

            if ((t == "Return" || t == "return") && brace_depth == 0)
            {
                saw_return = true;
                break;
            }

            if (brace_depth == 0 && looks_like_trigger_line(t))
            {
                break;
            }

            block_lines.push_back(raw_lines[j]);
        }

        bool as_action_block = saw_return;
        if (!as_action_block)
        {
            for (const auto &bl : block_lines)
            {
                if (looks_like_action_line(bl))
                {
                    as_action_block = true;
                    break;
                }
            }
        }

        std::string joined;
        for (std::size_t k = 0; k < block_lines.size(); ++k)
        {
            if (k != 0)
            {
                joined.push_back('\n');
            }
            joined += block_lines[k];
        }

        try
        {
            if (as_action_block)
            {
                hs.commands = parse_action_block(joined, strict_mode, path.string() + ":" + std::to_string(i + 1));
            }
            else
            {
                parse_replacement(joined, hs);
            }
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(path.string() + ":" + std::to_string(i + 1) + ": " + e.what());
        }

        hotstrings.push_back(std::move(hs));

        if (saw_return)
        {
            i = j;
        }
        else
        {
            i = (j == 0) ? 0 : (j - 1);
        }
    }

    if (hotstrings.empty())
    {
        throw std::runtime_error("script has no supported hotstrings: " + path.string());
    }

    return hotstrings;
}

    std::vector<std::string> AhkParser::split_utf8(const std::string &s)
    {
        std::vector<std::string> out;
        for (std::size_t i = 0; i < s.size();)
        {
            const auto c = static_cast<unsigned char>(s[i]);
            std::size_t len = 1;
            if ((c & 0xE0) == 0xC0)
            {
                len = 2;
            }
            else if ((c & 0xF0) == 0xE0)
            {
                len = 3;
            }
            else if ((c & 0xF8) == 0xF0)
            {
                len = 4;
            }

            if (i + len > s.size())
            {
                throw std::runtime_error("invalid UTF-8 in trigger");
            }

            out.push_back(s.substr(i, len));
            i += len;
        }
        return out;
    }

    void AhkParser::parse_replacement(const std::string &replacement, Hotstring &hs)
    {
        std::string text;
        std::size_t pos = 0;

        while (pos < replacement.size())
        {
            const auto bracket = replacement.find('{', pos);

            if (bracket == std::string::npos)
            {
                // No more tail commands
                text += replacement.substr(pos);
                break;
            }

            // Add text before the bracket
            text += replacement.substr(pos, bracket - pos);

            // Find closing bracket
            const auto close_bracket = replacement.find('}', bracket);
            if (close_bracket == std::string::npos)
            {
                throw std::runtime_error("unterminated { in replacement");
            }

            const auto cmd = replacement.substr(bracket + 1, close_bracket - bracket - 1);
            // AHK escaped literals in braces
            if (cmd == "!" || cmd == "+" || cmd == "^" || cmd == "#" || cmd == "{" || cmd == "}")
            {
                text += cmd;
                pos = close_bracket + 1;
                continue;
            }
            // Parse tail command
            std::istringstream cmd_stream(cmd);
            std::string cmd_name;
            cmd_stream >> cmd_name;

            if (cmd_name == "Left")
            {
                int count = 1;
                cmd_stream >> count;
                hs.tail_keys.emplace_back(KEY_LEFT, count);
            }
            else if (cmd_name == "Right")
            {
                int count = 1;
                cmd_stream >> count;
                hs.tail_keys.emplace_back(KEY_RIGHT, count);
            }
            else if (cmd_name == "Up")
            {
                int count = 1;
                cmd_stream >> count;
                hs.tail_keys.emplace_back(KEY_UP, count);
            }
            else if (cmd_name == "Down")
            {
                int count = 1;
                cmd_stream >> count;
                hs.tail_keys.emplace_back(KEY_DOWN, count);
            }
            else if (cmd_name == "Home")
            {
                int count = 1;
                cmd_stream >> count;
                hs.tail_keys.emplace_back(KEY_HOME, count);
            }
            else if (cmd_name == "End")
            {
                int count = 1;
                cmd_stream >> count;
                hs.tail_keys.emplace_back(KEY_END, count);
            }
            else if (cmd_name == "Delete" || cmd_name == "Del")
            {
                int count = 1;
                cmd_stream >> count;
                hs.tail_keys.emplace_back(KEY_DELETE, count);
            }
            else if (cmd_name == "Backspace" || cmd_name == "BS")
            {
                int count = 1;
                cmd_stream >> count;
                hs.tail_keys.emplace_back(KEY_BACKSPACE, count);
            }
            else if (cmd_name == "Enter" || cmd_name == "Return")
            {
                hs.tail_keys.emplace_back(KEY_ENTER, 1);
            }
            else if (cmd_name == "Tab")
            {
                hs.tail_keys.emplace_back(KEY_TAB, 1);
            }
            else if (cmd_name == "Escape" || cmd_name == "Esc")
            {
                hs.tail_keys.emplace_back(KEY_ESC, 1);
            }
            else if (cmd_name == "Space" || cmd_name == "SPACE")
            {
                hs.tail_keys.emplace_back(KEY_SPACE, 1);
            }   
            else
            {
                throw std::runtime_error("unknown tail command: {" + cmd + "}");
            }

            pos = close_bracket + 1;
        }

        hs.replacement_utf8 = text;
    }

    CommandList AhkParser::parse_action_block(const std::string &code_block, bool strict_mode, const std::string &origin)
    {
        return ScriptParser::parse_action_block(code_block, strict_mode, origin);
    }

} // namespace ahk
