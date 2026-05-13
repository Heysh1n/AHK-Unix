#include "ahkunix/commands/SleepCommand.hpp"
#include "ahkunix/Clipboard.hpp"
#include "ahkunix/UinputKeyboard.hpp"

#include <algorithm>
#include <atomic>
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

    void SleepCommand::execute_interruptible(
        UinputKeyboard &injector,
        Clipboard &clipboard,
        const std::atomic<bool> &stop_requested) const
    {
        (void)injector;
        (void)clipboard;

        int remaining = milliseconds_;
        while (remaining > 0 && !stop_requested.load(std::memory_order_acquire))
        {
            const int slice = std::min(remaining, 10);
            std::this_thread::sleep_for(std::chrono::milliseconds(slice));
            remaining -= slice;
        }
    }

    std::string SleepCommand::describe() const
    {
        std::stringstream ss;
        ss << "Sleep " << milliseconds_ << "ms";
        return ss.str();
    }
} // namespace ahk::cmd
