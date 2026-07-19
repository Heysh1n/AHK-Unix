#include "ahkunix/commands/CancelCommand.hpp"

#include "ahkunix/Clipboard.hpp"
#include "ahkunix/UinputKeyboard.hpp"

namespace ahk::cmd
{
    void CancelCommand::execute(UinputKeyboard &injector, Clipboard &clipboard) const
    {
        (void)injector;
        (void)clipboard;
    }

    void CancelCommand::execute_interruptible(
        UinputKeyboard &injector,
        Clipboard &clipboard,
        const std::atomic<bool> &stop_requested) const
    {
        (void)stop_requested;
        execute(injector, clipboard);
    }

    std::string CancelCommand::describe() const
    {
        return "Cancel";
    }

    bool CancelCommand::is_interrupt() const
    {
        return true;
    }
} // namespace ahk::cmd
