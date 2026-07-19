#pragma once

#include "ahkunix/commands/Command.hpp"

namespace ahk::cmd
{
    class CancelCommand : public Command
    {
    public:
        void execute(UinputKeyboard &injector, Clipboard &clipboard) const override;
        void execute_interruptible(
            UinputKeyboard &injector,
            Clipboard &clipboard,
            const std::atomic<bool> &stop_requested) const override;
        std::string describe() const override;
        bool is_interrupt() const override;
    };
} // namespace ahk::cmd
