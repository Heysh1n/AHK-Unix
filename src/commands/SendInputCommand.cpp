#include "ahkunix/commands/SendInputCommand.hpp"
#include "ahkunix/Clipboard.hpp"
#include "ahkunix/LayoutProfile.hpp"
#include "ahkunix/UinputKeyboard.hpp"
<<<<<<< HEAD
=======
#include "ahkunix/commands/IfCommand.hpp"
>>>>>>> master

#include <algorithm>
#include <atomic>
#include <chrono>
#include <linux/input-event-codes.h>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace ahk::cmd
{
    namespace
    {
        bool sleep_interruptible(std::chrono::milliseconds duration, const std::atomic<bool> &stop_requested)
        {
            auto remaining = duration;
            while (remaining.count() > 0)
            {
                if (stop_requested.load(std::memory_order_acquire))
                {
                    return false;
                }

                const auto slice = std::min(remaining, std::chrono::milliseconds(10));
                std::this_thread::sleep_for(slice);
                remaining -= slice;
            }

            return !stop_requested.load(std::memory_order_acquire);
        }
    } // namespace

    SendInputCommand::SendInputCommand(std::string sequence)
        : sequence_(std::move(sequence))
    {
        parse_sequence();
    }

<<<<<<< HEAD
=======
    void SendInputCommand::bind_context(const std::shared_ptr<Context> &ctx)
    {
        context_ = ctx;
    }

>>>>>>> master
    void SendInputCommand::parse_sequence()
    {
        auto append_text = [this](const std::string &s) {
            if (s.empty())
            {
                return;
            }

            if (!parts_.empty() && !parts_.back().is_special_key)
            {
                parts_.back().text += s;
                return;
            }

            Part p;
            p.is_special_key = false;
            p.text = s;
            parts_.push_back(std::move(p));
        };

        std::size_t pos = 0;

        while (pos < sequence_.size())
        {
            const auto bracket_start = sequence_.find('{', pos);

            if (bracket_start == std::string::npos)
            {
                append_text(sequence_.substr(pos));
                break;
            }

            if (bracket_start > pos)
            {
                append_text(sequence_.substr(pos, bracket_start - pos));
            }

            const auto bracket_end = sequence_.find('}', bracket_start);
            if (bracket_end == std::string::npos)
            {
                throw std::runtime_error("unterminated { in SendInput sequence");
            }

            std::string cmd = sequence_.substr(bracket_start + 1, bracket_end - bracket_start - 1);

            // AHK escaped literals: {!} {+} {^} {#} {{}} ...
            if (cmd == "!" || cmd == "+" || cmd == "^" || cmd == "#" || cmd == "{" || cmd == "}")
            {
                append_text(cmd);
                pos = bracket_end + 1;
                continue;
            }

            std::istringstream cmd_stream(cmd);
            std::string key_name;
            int repeat_count = 1;
            cmd_stream >> key_name >> repeat_count;

            Part p;
            p.is_special_key = true;
            p.key_code = LayoutProfile::parse_special_key(key_name);
            p.repeat_count = repeat_count < 1 ? 1 : repeat_count;
            parts_.push_back(std::move(p));

            pos = bracket_end + 1;
        }
    }

    void SendInputCommand::execute_interruptible(
        UinputKeyboard &injector,
        Clipboard &clipboard,
        const std::atomic<bool> &stop_requested) const
    {
        for (const auto &part : parts_)
        {
            if (stop_requested.load(std::memory_order_acquire))
            {
                return;
            }

            if (part.is_special_key)
            {
                for (int i = 0; i < part.repeat_count; ++i)
                {
                    if (stop_requested.load(std::memory_order_acquire))
                    {
                        return;
                    }
                    injector.tap(part.key_code);
                    if (!sleep_interruptible(std::chrono::milliseconds(5), stop_requested))
                    {
                        return;
                    }
                }
                continue;
            }

            if (part.text.empty())
            {
                continue;
            }

<<<<<<< HEAD
            clipboard.paste_text_synchronously(part.text, injector);
=======
            std::string final_text = context_ ? context_->interpolate(part.text) : part.text;
            clipboard.paste_text_synchronously(final_text, injector);
>>>>>>> master
        }
    }

    void SendInputCommand::execute(UinputKeyboard &injector, Clipboard &clipboard) const
    {
        for (const auto &part : parts_)
        {
            if (part.is_special_key)
            {
                for (int i = 0; i < part.repeat_count; ++i)
                {
                    injector.tap(part.key_code);
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            }
            else if (!part.text.empty())
            {
<<<<<<< HEAD
                clipboard.paste_text_synchronously(part.text, injector);
=======
                std::string final_text = context_ ? context_->interpolate(part.text) : part.text;
                clipboard.paste_text_synchronously(final_text, injector);
>>>>>>> master
            }
        }
    }

    std::string SendInputCommand::describe() const
    {
        return "SendInput: " + sequence_.substr(0, 40) + (sequence_.size() > 40 ? "..." : "");
    }
} // namespace ahk::cmd
