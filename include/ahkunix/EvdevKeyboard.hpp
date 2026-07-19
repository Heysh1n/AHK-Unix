#pragma once

#include <libevdev/libevdev.h>

#include <filesystem>
#include <optional>

namespace ahk {

struct RawEvent {
    int type = 0;
    int code = 0;
    int value = 0;
};

class EvdevKeyboard {
public:
    explicit EvdevKeyboard(const std::filesystem::path& path);

    EvdevKeyboard(const EvdevKeyboard&) = delete;
    EvdevKeyboard& operator=(const EvdevKeyboard&) = delete;

    ~EvdevKeyboard();

    const char* name() const;
    int fd() const;

    void grab();
    void ungrab() noexcept;

    std::optional<RawEvent> read_event();

private:
    class Impl;
    Impl* impl_ = nullptr;
};

} // namespace ahk
