#pragma once

#include "ahkunix/Clipboard.hpp"
#include "ahkunix/EvdevKeyboard.hpp"
#include "ahkunix/Hotstring.hpp"
#include "ahkunix/RingBuffer.hpp"
#include "ahkunix/UinputKeyboard.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <mutex>
#include <optional>
#include <set>
#include <thread>
#include <vector>

namespace ahk
{

    class Daemon
    {
    public:
        Daemon(std::filesystem::path input_path, std::vector<Hotstring> hotstrings);
        ~Daemon();

        void run();
        void reload_script(const std::filesystem::path &path, bool strict_mode);

    private:
        void handle_event(const RawEvent &ev);
        void submit_macro(Hotstring hotstring);
        void request_macro_stop();
        void stop_macro_worker() noexcept;
        void macro_worker_loop() noexcept;
        void execute_macro(const Hotstring &hotstring);
        void execute_legacy_replacement(const Hotstring &hotstring);
        static bool is_interrupt_macro(const Hotstring &hotstring);
        std::optional<Hotstring> find_match() const;
        static std::size_t ring_capacity(const std::vector<Hotstring> &hotstrings);

        EvdevKeyboard physical_;
        UinputKeyboard injector_;
        Clipboard clipboard_;
        mutable std::mutex mutex_;
        RingBuffer ring_;
        std::vector<Hotstring> hotstrings_;
        std::set<int> pressed_keys_; // Currently pressed modifier keys

        std::atomic<bool> stop_requested_{false};
        std::mutex macro_mutex_;
        std::condition_variable macro_cv_;
        std::deque<Hotstring> macro_queue_;
        std::thread macro_worker_;
        bool macro_worker_stop_ = false;
    };

} // namespace ahk
