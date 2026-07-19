#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace ahk
{
    class UinputKeyboard;
    class Clipboard;

    namespace cmd
    {
        class Context;

        class Command
        {
        public:
            virtual ~Command() = default;
            virtual void execute(UinputKeyboard &injector, Clipboard &clipboard) const = 0;
            virtual void execute_interruptible(
                UinputKeyboard &injector,
                Clipboard &clipboard,
                const std::atomic<bool> &stop_requested) const
            {
                if (!stop_requested.load(std::memory_order_acquire))
                {
                    execute(injector, clipboard);
                }
            }
            virtual std::string describe() const = 0;
            virtual bool is_interrupt() const
            {
                return false;
            }

            // Optional context binding for commands that need variables/random state.
            virtual void bind_context(const std::shared_ptr<Context> &) {}
        };

        using CommandPtr = std::shared_ptr<Command>;
        using CommandList = std::vector<CommandPtr>;
    } // namespace cmd
} // namespace ahk
