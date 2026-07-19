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

    Daemon::Daemon(std::filesystem::path input_path, std::vector<Hotstring> hotstrings)
        : physical_(input_path),
          injector_(),
          clipboard_(),
          ring_(ring_capacity(hotstrings)),
          hotstrings_(std::move(hotstrings))
    {
        macro_worker_ = std::thread(&Daemon::macro_worker_loop, this);
    }

    Daemon::~Daemon()
    {
        stop_macro_worker();
    }

    void Daemon::reload_script(const std::filesystem::path &path, bool strict_mode)
    {
        request_macro_stop();

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

        stop_macro_worker();
        physical_.ungrab();
        std::cerr << "Stopped.\n";
    }

    void Daemon::handle_event(const RawEvent &ev)
    {
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

                if (ev.value == 1)
                {
                    ring_.push(ev.code);
                }
            }
        }

        injector_.forward(ev);

        if (ev.type != EV_KEY || ev.value != 1)
        {
            return;
        }

        auto matched = find_match();
        if (!matched)
        {
            return;
        }

        if (is_interrupt_macro(*matched))
        {
            request_macro_stop();
            std::lock_guard lock(mutex_);
            ring_.clear();
            return;
        }

        submit_macro(std::move(*matched));

        {
            std::lock_guard lock(mutex_);
            ring_.clear();
        }
    }

    void Daemon::submit_macro(Hotstring hotstring)
    {
        {
            std::lock_guard lock(macro_mutex_);
            if (macro_worker_stop_)
            {
                return;
            }
            macro_queue_.push_back(std::move(hotstring));
        }

        macro_cv_.notify_one();
    }

    void Daemon::request_macro_stop()
    {
        {
            std::lock_guard lock(macro_mutex_);
            macro_queue_.clear();
            stop_requested_.store(true, std::memory_order_release);
        }

        macro_cv_.notify_one();
    }

    void Daemon::stop_macro_worker() noexcept
    {
        request_macro_stop();

        {
            std::lock_guard lock(macro_mutex_);
            macro_worker_stop_ = true;
            macro_queue_.clear();
        }

        macro_cv_.notify_one();

        if (macro_worker_.joinable() && macro_worker_.get_id() != std::this_thread::get_id())
        {
            macro_worker_.join();
        }
    }

    void Daemon::macro_worker_loop() noexcept
    {
        while (true)
        {
            Hotstring job;

            {
                std::unique_lock lock(macro_mutex_);
                macro_cv_.wait(lock, [this] {
                    return macro_worker_stop_ || !macro_queue_.empty();
                });

                if (macro_worker_stop_)
                {
                    break;
                }

                job = std::move(macro_queue_.front());
                macro_queue_.pop_front();
                stop_requested_.store(false, std::memory_order_release);
            }

            try
            {
                execute_macro(job);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Macro execution failed: " << e.what() << '\n';
            }
            catch (...)
            {
                std::cerr << "Macro execution failed: unknown error\n";
            }
        }
    }

    void Daemon::execute_macro(const Hotstring &hotstring)
    {
        if (hotstring.erase_trigger)
        {
            for (std::size_t i = 0; i < hotstring.trigger_keys.size(); ++i)
            {
                if (stop_requested_.load(std::memory_order_acquire))
                {
                    return;
                }
                injector_.tap(KEY_BACKSPACE);
                if (!sleep_interruptible(std::chrono::milliseconds(8), stop_requested_))
                {
                    return;
                }
            }
        }

        if (stop_requested_.load(std::memory_order_acquire))
        {
            return;
        }

        if (!hotstring.commands.empty())
        {
            if (!hotstring.context)
            {
                hotstring.context = std::make_shared<cmd::Context>();
            }

            for (const auto &cmd : hotstring.commands)
            {
                if (stop_requested_.load(std::memory_order_acquire))
                {
                    return;
                }

                cmd->bind_context(hotstring.context);
                cmd->execute_interruptible(injector_, clipboard_, stop_requested_);
            }
            return;
        }

        if (!hotstring.replacement_utf8.empty())
        {
            execute_legacy_replacement(hotstring);
        }
    }

    void Daemon::execute_legacy_replacement(const Hotstring &hotstring)
    {
        if (stop_requested_.load(std::memory_order_acquire))
        {
            return;
        }

        clipboard_.paste_text_synchronously(hotstring.replacement_utf8, injector_);

        for (const auto &[key, count] : hotstring.tail_keys)
        {
            for (int i = 0; i < count; ++i)
            {
                if (stop_requested_.load(std::memory_order_acquire))
                {
                    return;
                }
                injector_.tap(key);
            }
        }
    }

    bool Daemon::is_interrupt_macro(const Hotstring &hotstring)
    {
        return std::any_of(hotstring.commands.begin(), hotstring.commands.end(), [](const auto &cmd) {
            return cmd && cmd->is_interrupt();
        });
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
