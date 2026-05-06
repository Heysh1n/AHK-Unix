#pragma once

#include "ahkunix/Clipboard.hpp"
#include "ahkunix/EvdevKeyboard.hpp"
#include "ahkunix/Hotstring.hpp"
#include "ahkunix/RingBuffer.hpp"
#include "ahkunix/UinputKeyboard.hpp"

#include <filesystem>
#include <mutex>
#include <optional>
#include <set>
#include <vector>

namespace ahk
{

    class Daemon
    {
    public:
        Daemon(std::filesystem::path input_path, std::vector<Hotstring> hotstrings);

        void run();
        void reload_script(const std::filesystem::path &path, bool strict_mode);

    private:
        void handle_event(const RawEvent &ev);
        std::optional<Hotstring> find_match() const;
        static std::size_t ring_capacity(const std::vector<Hotstring> &hotstrings);

        EvdevKeyboard physical_;
        UinputKeyboard injector_;
        Clipboard clipboard_;
        mutable std::mutex mutex_;
        RingBuffer ring_;
        std::vector<Hotstring> hotstrings_;
        std::set<int> pressed_keys_; // Currently pressed modifier keys
    };

} // namespace ahk
