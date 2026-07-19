#include "ahkunix/commands/ScriptParser.hpp"

#include "ahkunix/StringUtil.hpp"
#include "ahkunix/commands/CancelCommand.hpp"
#include "ahkunix/commands/IfCommand.hpp"
#include "ahkunix/commands/IncrementCommand.hpp"
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

        char lower_ascii(char ch)
        {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }

        bool is_command_boundary(char ch)
        {
            return std::isspace(static_cast<unsigned char>(ch)) || ch == ',' || ch == '(' || ch == '{';
        }

        bool starts_with_command(const std::string &line, const std::string &command)
        {
            if (line.size() < command.size())
            {
                return false;
            }
            if (line.compare(0, command.size(), command) != 0)
            {
                return false;
            }

            return line.size() == command.size() || is_command_boundary(line[command.size()]);
        }

        void lowercase_range(std::string &line, std::size_t pos, std::size_t len)
        {
            for (std::size_t i = pos; i < pos + len; ++i)
            {
                line[i] = lower_ascii(line[i]);
            }
        }

        bool normalize_command_at(std::string &line, std::size_t pos, const std::string &command)
        {
            if (line.size() < pos + command.size())
            {
                return false;
            }
            for (std::size_t i = 0; i < command.size(); ++i)
            {
                if (lower_ascii(line[pos + i]) != command[i])
                {
                    return false;
                }
            }

            const std::size_t end = pos + command.size();
            if (end != line.size() && !is_command_boundary(line[end]))
            {
                return false;
            }

            lowercase_range(line, pos, command.size());
            return true;
        }

        std::string normalize_command_tokens(std::string line)
        {
            static const std::vector<std::string> commands = {
                "sendinput",
                "sendmessage",
                "random",
                "return",
                "cancel",
                "sleep",
                "pause",
                "input",
                "else",
                "if",
            };

            for (const auto &command : commands)
            {
                if (!normalize_command_at(line, 0, command))
                {
                    continue;
                }

                if (command == "else")
                {
                    std::size_t if_pos = command.size();
                    while (if_pos < line.size() && std::isspace(static_cast<unsigned char>(line[if_pos])))
                    {
                        ++if_pos;
                    }
                    (void)normalize_command_at(line, if_pos, "if");
                }

                break;
            }

            return line;
        }
    } // namespace

    std::vector<std::string> ScriptParser::preprocess_lines(const std::string &block, bool strict_mode, const std::string &origin)
    {
        (void)strict_mode;
        (void)origin;

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

            out.push_back(normalize_command_tokens(std::move(line)));
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
        if (starts_with_command(line, "sendinput"))
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

        if (starts_with_command(line, "sleep"))
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

        if (starts_with_command(line, "random"))
        {
            static const std::regex re(
                R"(^\s*random\s*,\s*([A-Za-z_]\w*)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*$)");

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

        if (starts_with_command(line, "sendmessage") || starts_with_command(line, "input") || starts_with_command(line, "return"))
        {
            return nullptr;
        }

        if (starts_with_command(line, "cancel") || starts_with_command(line, "pause"))
        {
            return std::make_shared<CancelCommand>();
        }

        {
            static const std::regex increment_re(
                R"(^\s*([A-Za-z_]\w*)\+\+\s*$)");
            std::smatch m;
            if (std::regex_match(line, m, increment_re))
            {
                return std::make_shared<IncrementCommand>(m[1].str());
            }
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

        if (starts_with_command(line, "else"))
        {
            const std::string maybe_if = trim(line.substr(4));
            if (starts_with_command(maybe_if, "if"))
            {
                line = maybe_if;
            }
        }

        if (!starts_with_command(line, "if"))
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

        if (idx < lines.size() && starts_with_command(lines[idx], "else"))
        {
            const std::string else_line = lines[idx];

            const std::string maybe_if = trim(else_line.substr(4));
            if (starts_with_command(maybe_if, "if"))
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

            if (starts_with_command(line, "if"))
            {
                commands.push_back(parse_if_chain(lines, idx));
                continue;
            }

            if (starts_with_command(line, "else"))
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
