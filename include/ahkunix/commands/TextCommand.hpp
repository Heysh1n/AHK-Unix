#pragma once

#include "ahkunix/commands/Command.hpp"
#include <string>

namespace ahk {
namespace cmd {

class TextCommand : public Command {
public:
    explicit TextCommand(std::string text);

    void execute(UinputKeyboard &injector, Clipboard &clipboard) const override;
    void execute_interruptible(
        UinputKeyboard &injector,
        Clipboard &clipboard,
        const std::atomic<bool> &stop_requested) const override;
    std::string describe() const override;

private:
    std::string text_;
};

} // namespace cmd
} // namespace ahk
