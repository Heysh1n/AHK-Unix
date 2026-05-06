#include "ahkunix/Daemon.hpp"

#include "ahkunix/AhkParser.hpp"
#include "ahkunix/Errors.hpp"
#include "ahkunix/LayoutProfile.hpp"
#include "ahkunix/Signals.hpp"
#include "ahkunix/commands/IfCommand.hpp"

#include <linux/input-event-codes.h>
#include <poll.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <utility>

namespace ahk
{

    using namespace ahk::cmd;

    Daemon::Daemon(std::filesystem::path input_path, std::vector<Hotstring> hotstrings)
        : physical_(input_path),
          injector_(),
          clipboard_(),
          ring_(ring_capacity(hotstrings)),
          hotstrings_(std::move(hotstrings)) {}

    void Daemon::reload_script(const std::filesystem::path &path, bool strict_mode)
    {
        const auto layout = LayoutProfile::russian_qwerty();
        auto hotstrings = AhkParser::parse_file(path, layout, strict_mode);
        RingBuffer new_ring(ring_capacity(hotstrings));

        {
            std::lock_guard lock(mutex_);
            hotstrings_ = std::move(hotstrings);
            ring_ = std::move(new_ring);
            pressed_keys_.clear();
        }

        std::cerr << "Reloaded script: " << path << '\n';
    }

    void Daemon::run()
    {
        std::cerr << "Input: " << physical_.name() << "\n";

        physical_.grab();
        std::cerr << "Grabbed. Transparent forwarding enabled.\n";
        std::size_t loaded_hotstrings = 0;
        {
            std::lock_guard lock(mutex_);
            loaded_hotstrings = hotstrings_.size();
        }
        std::cerr << "Loaded hotstrings: " << loaded_hotstrings << "\n";

        while (!g_stop)
        {
            pollfd pfd{
                .fd = physical_.fd(),
                .events = POLLIN,
                .revents = 0,
            };

            const int ready = ::poll(&pfd, 1, 250);
            if (ready < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                throw SysError("poll input device");
            }
            if (ready == 0)
            {
                continue;
            }

            while (auto ev = physical_.read_event())
            {
                handle_event(*ev);
            }
        }

        physical_.ungrab();
        std::cerr << "Stopped.\n";
    }

    void Daemon::handle_event(const RawEvent &ev)
    {
        injector_.forward(ev);

        {
            std::lock_guard lock(mutex_);

            // Track modifier key presses/releases
            if (ev.type == EV_KEY)
            {
                if (ev.code == KEY_LEFTCTRL || ev.code == KEY_RIGHTCTRL ||
                    ev.code == KEY_LEFTALT || ev.code == KEY_RIGHTALT ||
                    ev.code == KEY_LEFTSHIFT || ev.code == KEY_RIGHTSHIFT ||
                    ev.code == KEY_LEFTMETA || ev.code == KEY_RIGHTMETA)
                {

                    if (ev.value == 1)
                    {
                        // Key pressed
                        pressed_keys_.insert(ev.code);
                    }
                    else if (ev.value == 0)
                    {
                        // Key released
                        pressed_keys_.erase(ev.code);
                    }
                }
            }

            if (ev.type == EV_KEY && ev.value == 1)
            {
                ring_.push(ev.code);
            }
        }

        if (ev.type != EV_KEY || ev.value != 1)
        {
            return;
        }

        auto matched = find_match();
        if (!matched)
        {
            return;
        }

        if (matched->erase_trigger)
        {
            injector_.backspace(matched->trigger_keys.size());
        }

        // NEW: Execute commands if available, otherwise use legacy replacement
        if (!matched->commands.empty())
        {
            // Initialize context if needed (for random/variables)
            if (!matched->context)
            {
                matched->context = std::make_shared<cmd::Context>();
            }

            for (const auto &cmd : matched->commands)
            {
                cmd->bind_context(matched->context);
                cmd->execute(injector_, clipboard_);
            }
        }
        else if (!matched->replacement_utf8.empty())
        {
            // Legacy path: simple text replacement
            clipboard_.set_text(matched->replacement_utf8);
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            injector_.hold_combo_and_tap({KEY_LEFTCTRL}, KEY_V);
            std::this_thread::sleep_for(std::chrono::milliseconds(30));

            for (const auto &[key, count] : matched->tail_keys)
            {
                for (int i = 0; i < count; ++i)
                {
                    injector_.tap(key);
                }
            }
        }

        {
            std::lock_guard lock(mutex_);
            ring_.clear();
        }
    }

    std::optional<Hotstring> Daemon::find_match() const
    {
        const Hotstring *best = nullptr;

        std::lock_guard lock(mutex_);
        for (const auto &hotstring : hotstrings_)
        {
            if (!ring_.ends_with(hotstring.trigger_keys))
            {
                continue;
            }

            // Check if required modifiers are pressed
            if (!hotstring.trigger_modifiers.empty())
            {
                bool modifiers_match = true;
                for (int required_mod : hotstring.trigger_modifiers)
                {
                    // Check if required modifier (or its counterpart) is pressed
                    bool found = pressed_keys_.count(required_mod) > 0;

                    // Also check counterpart (left/right variants)
                    if (!found)
                    {
                        if (required_mod == KEY_LEFTCTRL && pressed_keys_.count(KEY_RIGHTCTRL) > 0)
                            found = true;
                        else if (required_mod == KEY_RIGHTCTRL && pressed_keys_.count(KEY_LEFTCTRL) > 0)
                            found = true;
                        else if (required_mod == KEY_LEFTALT && pressed_keys_.count(KEY_RIGHTALT) > 0)
                            found = true;
                        else if (required_mod == KEY_RIGHTALT && pressed_keys_.count(KEY_LEFTALT) > 0)
                            found = true;
                        else if (required_mod == KEY_LEFTSHIFT && pressed_keys_.count(KEY_RIGHTSHIFT) > 0)
                            found = true;
                        else if (required_mod == KEY_RIGHTSHIFT && pressed_keys_.count(KEY_LEFTSHIFT) > 0)
                            found = true;
                        else if (required_mod == KEY_LEFTMETA && pressed_keys_.count(KEY_RIGHTMETA) > 0)
                            found = true;
                        else if (required_mod == KEY_RIGHTMETA && pressed_keys_.count(KEY_LEFTMETA) > 0)
                            found = true;
                    }

                    if (!found)
                    {
                        modifiers_match = false;
                        break;
                    }
                }

                if (!modifiers_match)
                {
                    continue;
                }
            }

            if (!best || hotstring.trigger_keys.size() > best->trigger_keys.size())
            {
                best = &hotstring;
            }
        }

        if (!best)
        {
            return std::nullopt;
        }

        if (!best->commands.empty() && !best->context)
        {
            best->context = std::make_shared<cmd::Context>();
        }

        return *best;
    }

    std::size_t Daemon::ring_capacity(const std::vector<Hotstring> &hotstrings)
    {
        std::size_t max_trigger = 16;
        for (const auto &hotstring : hotstrings)
        {
            max_trigger = std::max(max_trigger, hotstring.trigger_keys.size());
        }
        return max_trigger + 8;
    }

} // namespace ahk
