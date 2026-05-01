#pragma once

#include "ahkunix/commands/Command.hpp"

namespace ahk::cmd
{
    class SleepCommand : public Command
    {
    public:
        explicit SleepCommand(int milliseconds);

        void execute(UinputKeyboard &injector, Clipboard &clipboard) const override;
        std::string describe() const override;

    private:
        int milliseconds_;
    };
} // namespace ahk::cmd