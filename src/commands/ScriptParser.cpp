#include "ahkunix/commands/ScriptParser.hpp"

#include "ahkunix/StringUtil.hpp"
#include "ahkunix/commands/IfCommand.hpp"
#include "ahkunix/commands/SendInputCommand.hpp"
#include "ahkunix/commands/SleepCommand.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>

namespace ahk::cmd
{
    namespace
    {
        class RandomAssignCommand : public Command
        {
        public:
            RandomAssignCommand(std::string var_name, int min_val, int max_val)
                : var_name_(std::move(var_name)), min_val_(min_val), max_val_(max_val)
            {
            }

            void bind_context(const std::shared_ptr<Context> &ctx) override
            {
                context_ = ctx;
            }

            void execute(UinputKeyboard &, Clipboard &) const override
            {
                if (!context_)
                {
                    context_ = std::make_shared<Context>();
                }

                int lo = min_val_;
                int hi = max_val_;
                if (lo > hi)
                {
                    std::swap(lo, hi);
                }

                context_->set_variable(var_name_, Context::random_range(lo, hi));
            }

            std::string describe() const override
            {
                return "Random " + var_name_;
            }

        private:
            std::string var_name_;
            int min_val_;
            int max_val_;
            mutable std::shared_ptr<Context> context_;
        };

        std::string extract_condition(const std::string &line)
        {
            const auto open = line.find('(');
            const auto close = line.rfind(')');
            if (open == std::string::npos || close == std::string::npos || close <= open)
            {
                throw std::runtime_error("If condition must be in parentheses: " + line);
            }
            return trim(line.substr(open + 1, close - open - 1));
        }

        bool has_inline_open_brace(const std::string &line)
        {
            const auto close = line.rfind(')');
            const auto brace = line.find('{');
            return close != std::string::npos && brace != std::string::npos && brace > close;
        }

        bool starts_with_word(const std::string &line, const std::string &word)
        {
            if (line.size() < word.size())
            {
                return false;
            }
            if (line.compare(0, word.size(), word) != 0)
            {
                return false;
            }
            if (line.size() == word.size())
            {
                return true;
            }
            const char next = line[word.size()];
            return std::isspace(static_cast<unsigned char>(next)) || next == '(' || next == '{';
        }

        bool starts_with_word_ci(const std::string &line, const std::string &word)
        {
            if (line.size() < word.size())
            {
                return false;
            }
            for (std::size_t i = 0; i < word.size(); ++i)
            {
                const auto a = static_cast<unsigned char>(line[i]);
                const auto b = static_cast<unsigned char>(word[i]);
                if (std::tolower(a) != std::tolower(b))
                {
                    return false;
                }
            }
            if (line.size() == word.size())
            {
                return true;
            }
            const char next = line[word.size()];
            return std::isspace(static_cast<unsigned char>(next)) || next == '(' || next == '{';
        }

        bool is_if_else_line_ci(const std::string &line)
        {
            return starts_with_word_ci(line, "if") || starts_with_word_ci(line, "else");
        }

        bool is_if_else_case_allowed(const std::string &line)
        {
            return starts_with_word(line, "if") ||
                   starts_with_word(line, "IF") ||
                   starts_with_word(line, "else") ||
                   starts_with_word(line, "ELSE");
        }

        [[noreturn]] void throw_if_case_error(const std::string &origin, const std::string &line)
        {
            const std::string where = origin.empty() ? "script" : origin;
            throw std::runtime_error(
                where + ": unsupported If/Else case. Use only if/else or IF/ELSE. Line: " + line);
        }

        void warn_if_case_error(const std::string &origin, const std::string &line)
        {
            const std::string where = origin.empty() ? "script" : origin;
            std::cerr << "[warn] " << where
                      << ": ignored line due to unsupported If/Else case (allowed: if/else or IF/ELSE): "
                      << line << "\n";
        }
    } // namespace

    bool ScriptParser::starts_with_ci(const std::string &text, const std::string &prefix)
    {
        if (text.size() < prefix.size())
        {
            return false;
        }

        for (std::size_t i = 0; i < prefix.size(); ++i)
        {
            const auto a = static_cast<unsigned char>(text[i]);
            const auto b = static_cast<unsigned char>(prefix[i]);
            if (std::tolower(a) != std::tolower(b))
            {
                return false;
            }
        }

        return true;
    }

    std::vector<std::string> ScriptParser::preprocess_lines(const std::string &block, bool strict_mode, const std::string &origin)
    {
        std::vector<std::string> out;
        std::string current;

        auto push_processed_line = [&](std::string line) {
            line = trim(line);
            if (line.empty() || line.starts_with(";") || line.starts_with("#"))
            {
                return;
            }

            while (!line.empty() && line.front() == '}')
            {
                out.push_back("}");
                line = trim(line.substr(1));
            }

            if (line.empty())
            {
                return;
            }

            if (is_if_else_line_ci(line) && !is_if_else_case_allowed(line))
            {
                if (strict_mode)
                {
                    throw_if_case_error(origin, line);
                }
                warn_if_case_error(origin, line);
                return;
            }

            out.push_back(std::move(line));
        };

        for (char ch : block)
        {
            if (ch == '\r')
            {
                continue;
            }

            if (ch == '\n')
            {
                push_processed_line(current);
                current.clear();
            }
            else
            {
                current.push_back(ch);
            }
        }

        if (!current.empty())
        {
            push_processed_line(current);
        }

        return out;
    }

    CommandPtr ScriptParser::parse_simple_command(const std::string &line)
    {
        if (starts_with_ci(line, "SendInput"))
        {
            std::string args = trim(line.substr(9));
            if (!args.empty() && args.front() == ',')
            {
                args = trim(args.substr(1));
            }
            if (args.empty())
            {
                throw std::runtime_error("SendInput requires arguments");
            }
            return std::make_shared<SendInputCommand>(args);
        }

        if (starts_with_ci(line, "Sleep"))
        {
            std::string args = trim(line.substr(5));
            if (!args.empty() && args.front() == ',')
            {
                args = trim(args.substr(1));
            }
            if (args.empty())
            {
                throw std::runtime_error("Sleep requires milliseconds value");
            }
            return std::make_shared<SleepCommand>(std::stoi(args));
        }

        if (starts_with_ci(line, "Random"))
        {
            static const std::regex re(
                R"(^\s*Random\s*,\s*([A-Za-z_]\w*)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*$)",
                std::regex::icase);

            std::smatch m;
            if (!std::regex_match(line, m, re))
            {
                throw std::runtime_error("invalid Random syntax: " + line);
            }

            return std::make_shared<RandomAssignCommand>(
                m[1].str(),
                std::stoi(m[2].str()),
                std::stoi(m[3].str()));
        }

        if (starts_with_ci(line, "SendMessage") || starts_with_ci(line, "Input") || starts_with_ci(line, "Return"))
        {
            return nullptr;
        }

        return nullptr;
    }

    CommandPtr ScriptParser::parse_if_chain(const std::vector<std::string> &lines, std::size_t &idx)
    {
        if (idx >= lines.size())
        {
            throw std::runtime_error("unexpected end of script while parsing If");
        }

        std::string line = lines[idx];

        if (starts_with_ci(line, "Else If"))
        {
            line = trim(line.substr(5));
        }

        if (!starts_with_ci(line, "If"))
        {
            throw std::runtime_error("expected If/Else If, got: " + line);
        }

        const std::string condition = extract_condition(line);

        ++idx;
        if (!has_inline_open_brace(line))
        {
            if (idx >= lines.size() || lines[idx] != "{")
            {
                throw std::runtime_error("expected { after If condition");
            }
            ++idx;
        }

        CommandList true_branch = parse_block_lines(lines, idx, true);
        CommandList false_branch;

        if (idx < lines.size() && starts_with_ci(lines[idx], "Else"))
        {
            const std::string else_line = lines[idx];

            if (starts_with_ci(else_line, "Else If"))
            {
                false_branch.push_back(parse_if_chain(lines, idx));
            }
            else
            {
                ++idx;
                if (else_line.find('{') == std::string::npos)
                {
                    if (idx >= lines.size() || lines[idx] != "{")
                    {
                        throw std::runtime_error("expected { after Else");
                    }
                    ++idx;
                }
                false_branch = parse_block_lines(lines, idx, true);
            }
        }

        return std::make_shared<IfCommand>(condition, std::move(true_branch), std::move(false_branch));
    }

    CommandList ScriptParser::parse_block_lines(const std::vector<std::string> &lines, std::size_t &idx, bool stop_at_closing_brace)
    {
        CommandList commands;

        while (idx < lines.size())
        {
            const std::string &line = lines[idx];

            if (line == "}")
            {
                if (stop_at_closing_brace)
                {
                    ++idx;
                    break;
                }
                ++idx;
                continue;
            }

            if (starts_with_ci(line, "If"))
            {
                commands.push_back(parse_if_chain(lines, idx));
                continue;
            }

            if (starts_with_ci(line, "Else"))
            {
                break;
            }

            if (CommandPtr cmd = parse_simple_command(line))
            {
                commands.push_back(std::move(cmd));
            }

            ++idx;
        }

        return commands;
    }

    CommandList ScriptParser::parse_action_block(const std::string &block, bool strict_mode, const std::string &origin)
    {
        const auto lines = preprocess_lines(block, strict_mode, origin);
        std::size_t idx = 0;
        return parse_block_lines(lines, idx, false);
    }
} // namespace ahk::cmd