#pragma once

#include "ahkunix/commands/Command.hpp"
#include <string>

namespace ahk::cmd
{
    class IncrementCommand : public Command
    {
    public:
        explicit IncrementCommand(std::string var_name);

        void execute(UinputKeyboard &injector, Clipboard &clipboard) const override;
        std::string describe() const override;

        void bind_context(const std::shared_ptr<Context> &ctx) override;

    private:
        std::string var_name_;
        mutable std::shared_ptr<Context> context_;
    };
} // namespace ahk::cmd
