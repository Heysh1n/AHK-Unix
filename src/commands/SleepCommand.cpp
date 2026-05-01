#include "ahkunix/commands/SleepCommand.hpp"
#include "ahkunix/Clipboard.hpp"
#include "ahkunix/UinputKeyboard.hpp"

#include <chrono>
#include <sstream>
#include <thread>

namespace ahk::cmd
{
    SleepCommand::SleepCommand(int milliseconds)
        : milliseconds_(milliseconds < 0 ? 0 : milliseconds)
    {
    }

    void SleepCommand::execute(UinputKeyboard &injector, Clipboard &clipboard) const
    {
        (void)injector;
        (void)clipboard;
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds_));
    }

    std::string SleepCommand::describe() const
    {
        std::stringstream ss;
        ss << "Sleep " << milliseconds_ << "ms";
        return ss.str();
    }
} // namespace ahk::cmd