#include "ahkunix/commands/TextCommand.hpp"
#include "ahkunix/UinputKeyboard.hpp"
#include "ahkunix/Clipboard.hpp"
#include <linux/input-event-codes.h>
#include <chrono>
#include <thread>

namespace ahk
{
    namespace cmd
    {

        TextCommand::TextCommand(std::string text) : text_(std::move(text)) {}

        void TextCommand::execute(UinputKeyboard &injector, Clipboard &clipboard) const
        {
            const std::string old_clipboard = clipboard.get_text();
            clipboard.set_text(text_);
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            injector.hold_combo_and_tap({KEY_LEFTCTRL}, KEY_V);
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            clipboard.set_text(old_clipboard);
        }

        std::string TextCommand::describe() const
        {
            return "Text: " + text_.substr(0, 30) + (text_.size() > 30 ? "..." : "");
        }

    } // namespace cmd
} // namespace ahk
