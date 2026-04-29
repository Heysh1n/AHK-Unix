#include "ahkunix/Daemon.hpp"

#include "ahkunix/Errors.hpp"
#include "ahkunix/Signals.hpp"

#include <linux/input-event-codes.h>
#include <poll.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <iostream>
#include <thread>
#include <utility>

namespace ahk {

Daemon::Daemon(std::filesystem::path input_path, std::vector<Hotstring> hotstrings)
    : physical_(input_path),
      injector_(),
      clipboard_(),
      ring_(ring_capacity(hotstrings)),
      hotstrings_(std::move(hotstrings)) {}

void Daemon::run() {
    std::cerr << "Input: " << physical_.name() << "\n";

    physical_.grab();
    std::cerr << "Grabbed. Transparent forwarding enabled.\n";
    std::cerr << "Loaded hotstrings: " << hotstrings_.size() << "\n";

    while (!g_stop) {
        pollfd pfd {
            .fd = physical_.fd(),
            .events = POLLIN,
            .revents = 0,
        };

        const int ready = ::poll(&pfd, 1, 250);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw SysError("poll input device");
        }
        if (ready == 0) {
            continue;
        }

        while (auto ev = physical_.read_event()) {
            handle_event(*ev);
        }
    }

    physical_.ungrab();
    std::cerr << "Stopped.\n";
}

void Daemon::handle_event(const RawEvent& ev) {
    injector_.forward(ev);

    if (ev.type != EV_KEY || ev.value != 1) {
        return;
    }

    ring_.push(ev.code);
    const Hotstring* matched = find_match();
    if (!matched) {
        return;
    }

    injector_.backspace(matched->trigger_keys.size());

    clipboard_.set_text(matched->replacement_utf8);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    injector_.hold_combo_and_tap({KEY_LEFTCTRL}, KEY_V);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    for (const auto& [key, count] : matched->tail_keys) {
        for (int i = 0; i < count; ++i) {
            injector_.tap(key);
        }
    }

    ring_.clear();
}

const Hotstring* Daemon::find_match() const {
    const Hotstring* best = nullptr;
    for (const auto& hotstring : hotstrings_) {
        if (!ring_.ends_with(hotstring.trigger_keys)) {
            continue;
        }
        if (!best || hotstring.trigger_keys.size() > best->trigger_keys.size()) {
            best = &hotstring;
        }
    }
    return best;
}

std::size_t Daemon::ring_capacity(const std::vector<Hotstring>& hotstrings) {
    std::size_t max_trigger = 16;
    for (const auto& hotstring : hotstrings) {
        max_trigger = std::max(max_trigger, hotstring.trigger_keys.size());
    }
    return max_trigger + 8;
}

} // namespace ahk
