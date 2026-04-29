#include "ahkunix/AhkParser.hpp"

#include "ahkunix/Errors.hpp"
#include "ahkunix/StringUtil.hpp"

#include <linux/input-event-codes.h>

#include <fstream>
#include <stdexcept>

namespace ahk {

Hotstring AhkParser::parse_hotstring_line(const std::string& line, const LayoutProfile& layout) {
    if (!line.starts_with(":")) {
        throw std::runtime_error("AHK hotstring must start with ':'");
    }

    const auto first_separator = line.find("::");
    if (first_separator == std::string::npos) {
        throw std::runtime_error("AHK hotstring has no trigger separator");
    }

    const auto trigger_start = line.rfind(':', first_separator - 1);
    if (trigger_start == std::string::npos) {
        throw std::runtime_error("AHK hotstring has malformed options");
    }

    const auto trigger = line.substr(trigger_start + 1, first_separator - trigger_start - 1);
    const auto replacement = line.substr(first_separator + 2);

    Hotstring hs;
    for (const auto& ch : split_utf8(trigger)) {
        hs.trigger_keys.push_back(layout.key_for_utf8_char(ch));
    }

    parse_replacement(replacement, hs);
    return hs;
}

std::vector<Hotstring> AhkParser::parse_file(const std::filesystem::path& path, const LayoutProfile& layout) {
    std::ifstream in(path);
    if (!in) {
        throw SysError("open script " + path.string());
    }

    std::vector<Hotstring> hotstrings;
    std::string line;
    std::size_t line_no = 0;

    while (std::getline(in, line)) {
        ++line_no;
        line = trim(line);
        if (line.empty() || line.starts_with(';') || line.starts_with('#')) {
            continue;
        }

        if (!line.starts_with(":")) {
            continue;
        }

        try {
            hotstrings.push_back(parse_hotstring_line(line, layout));
        } catch (const std::exception& e) {
            throw std::runtime_error(path.string() + ":" + std::to_string(line_no) + ": " + e.what());
        }
    }

    if (hotstrings.empty()) {
        throw std::runtime_error("script has no supported hotstrings: " + path.string());
    }

    return hotstrings;
}

std::vector<std::string> AhkParser::split_utf8(const std::string& s) {
    std::vector<std::string> out;
    for (std::size_t i = 0; i < s.size();) {
        const auto c = static_cast<unsigned char>(s[i]);
        std::size_t len = 1;
        if ((c & 0xE0) == 0xC0) {
            len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            len = 4;
        }

        if (i + len > s.size()) {
            throw std::runtime_error("invalid UTF-8 in trigger");
        }

        out.push_back(s.substr(i, len));
        i += len;
    }
    return out;
}

void AhkParser::parse_replacement(const std::string& replacement, Hotstring& hs) {
    const auto token = replacement.find("{Left ");
    if (token == std::string::npos) {
        hs.replacement_utf8 = replacement;
        return;
    }

    hs.replacement_utf8 = replacement.substr(0, token);

    const auto count_begin = token + std::string("{Left ").size();
    const auto count_end = replacement.find('}', count_begin);
    if (count_end == std::string::npos) {
        throw std::runtime_error("unterminated {Left N} token");
    }

    const auto count = std::stoi(replacement.substr(count_begin, count_end - count_begin));
    hs.tail_keys.emplace_back(KEY_LEFT, count);
}

} // namespace ahk
