#include "ahkunix/commands/IfCommand.hpp"
#include "ahkunix/Clipboard.hpp"
#include "ahkunix/UinputKeyboard.hpp"

#include <chrono>
#include <random>
#include <regex>
#include <stdexcept>

namespace ahk::cmd
{
    int Context::get_variable(const std::string &name) const
    {
        auto it = variables_.find(name);
        return (it != variables_.end()) ? it->second : 0;
    }

    void Context::set_variable(const std::string &name, int value)
    {
        variables_[name] = value;
    }

    int Context::random_range(int min_val, int max_val)
    {
        static std::mt19937 gen(
            static_cast<std::mt19937::result_type>(
                std::chrono::high_resolution_clock::now().time_since_epoch().count()));
        std::uniform_int_distribution<> dist(min_val, max_val);
        return dist(gen);
    }

    IfCommand::IfCommand(std::string condition, CommandList true_branch, CommandList false_branch)
        : condition_(std::move(condition)),
          true_branch_(std::move(true_branch)),
          false_branch_(std::move(false_branch))
    {
    }

    void IfCommand::set_context(std::shared_ptr<Context> ctx)
    {
        context_ = std::move(ctx);
        for (const auto &cmd : true_branch_)
        {
            cmd->bind_context(context_);
        }
        for (const auto &cmd : false_branch_)
        {
            cmd->bind_context(context_);
        }
    }

    void IfCommand::bind_context(const std::shared_ptr<Context> &ctx)
    {
        set_context(ctx);
    }

    bool IfCommand::evaluate_condition() const
    {
        std::smatch match;

        // random(1,3) = 2
        const std::regex random_pattern(
            R"(^\s*random\(\s*(-?\d+)\s*,\s*(-?\d+)\s*\)\s*(==|=|!=|>=|<=|>|<)\s*(-?\d+)\s*$)",
            std::regex::icase);

        if (std::regex_match(condition_, match, random_pattern))
        {
            int min_val = std::stoi(match[1].str());
            int max_val = std::stoi(match[2].str());
            std::string op = match[3].str();
            int cmp = std::stoi(match[4].str());

            if (min_val > max_val)
            {
                std::swap(min_val, max_val);
            }

            int value = Context::random_range(min_val, max_val);

            if (op == "=" || op == "==") return value == cmp;
            if (op == "!=") return value != cmp;
            if (op == ">") return value > cmp;
            if (op == "<") return value < cmp;
            if (op == ">=") return value >= cmp;
            if (op == "<=") return value <= cmp;
            return false;
        }

        // variable comparison: announceType = 1
        const std::regex var_pattern(
            R"(^\s*([A-Za-z_]\w*)\s*(==|=|!=|>=|<=|>|<)\s*(-?\d+)\s*$)");

        if (std::regex_match(condition_, match, var_pattern))
        {
            const std::string var_name = match[1].str();
            const std::string op = match[2].str();
            const int cmp = std::stoi(match[3].str());

            if (!context_)
            {
                context_ = std::make_shared<Context>();
            }

            int value = context_->get_variable(var_name);

            if (op == "=" || op == "==") return value == cmp;
            if (op == "!=") return value != cmp;
            if (op == ">") return value > cmp;
            if (op == "<") return value < cmp;
            if (op == ">=") return value >= cmp;
            if (op == "<=") return value <= cmp;
            return false;
        }

        throw std::runtime_error("cannot evaluate condition: " + condition_);
    }

    void IfCommand::execute(UinputKeyboard &injector, Clipboard &clipboard) const
    {
        if (!context_)
        {
            context_ = std::make_shared<Context>();
        }

        const bool ok = evaluate_condition();
        const auto &branch = ok ? true_branch_ : false_branch_;

        for (const auto &cmd : branch)
        {
            cmd->bind_context(context_);
            cmd->execute(injector, clipboard);
        }
    }

    std::string IfCommand::describe() const
    {
        return "If (" + condition_ + ")";
    }
} // namespace ahk::cmd