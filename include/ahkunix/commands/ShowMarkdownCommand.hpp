#pragma once

#include "ahkunix/commands/Command.hpp"
#include <string>

namespace ahk::cmd
{
    class ShowMarkdownCommand : public Command
    {
    public:
        explicit ShowMarkdownCommand(std::string file_path);

        void execute(UinputKeyboard &injector, Clipboard &clipboard) const override;
        std::string describe() const override;

    private:
        std::string file_path_;
    };
} // namespace ahk::cmd
