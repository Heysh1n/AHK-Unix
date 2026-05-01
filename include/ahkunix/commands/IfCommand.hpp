#pragma once

#include "ahkunix/commands/Command.hpp"
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace ahk::cmd
{
    class Context
    {
    public:
        int get_variable(const std::string &name) const;
        void set_variable(const std::string &name, int value);
        static int random_range(int min_val, int max_val);

    private:
        std::unordered_map<std::string, int> variables_;
    };

    class IfCommand : public Command
    {
    public:
        IfCommand(std::string condition, CommandList true_branch, CommandList false_branch = {});

        void execute(UinputKeyboard &injector, Clipboard &clipboard) const override;
        std::string describe() const override;

        void set_context(std::shared_ptr<Context> ctx);
        void bind_context(const std::shared_ptr<Context> &ctx) override;

    private:
        std::string condition_;
        CommandList true_branch_;
        CommandList false_branch_;
        mutable std::shared_ptr<Context> context_;

        bool evaluate_condition() const;
    };
} // namespace ahk::cmd