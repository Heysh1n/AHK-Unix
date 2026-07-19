#include "ahkunix/commands/TextCommand.hpp"
#include "ahkunix/UinputKeyboard.hpp"
#include "ahkunix/Clipboard.hpp"
#include <atomic>

namespace ahk
{
    namespace cmd
    {

        TextCommand::TextCommand(std::string text) : text_(std::move(text)) {}

        void TextCommand::execute(UinputKeyboard &injector, Clipboard &clipboard) const
        {
            clipboard.paste_text_synchronously(text_, injector);
        }

        void TextCommand::execute_interruptible(
            UinputKeyboard &injector,
            Clipboard &clipboard,
            const std::atomic<bool> &stop_requested) const
        {
            if (stop_requested.load(std::memory_order_acquire))
            {
                return;
            }

            execute(injector, clipboard);
        }

        std::string TextCommand::describe() const
        {
            return "Text: " + text_.substr(0, 30) + (text_.size() > 30 ? "..." : "");
        }

    } // namespace cmd
} // namespace ahk
