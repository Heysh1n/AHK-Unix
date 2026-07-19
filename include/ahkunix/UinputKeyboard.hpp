#pragma once

#include "ahkunix/EvdevKeyboard.hpp"

#include <vector>

namespace ahk {

class UinputKeyboard {
public:
    UinputKeyboard();

    UinputKeyboard(const UinputKeyboard&) = delete;
    UinputKeyboard& operator=(const UinputKeyboard&) = delete;

    ~UinputKeyboard();

    void tap(int key_code);
    void backspace(std::size_t count);
    void forward(const RawEvent& ev);
    void hold_combo_and_tap(const std::vector<int>& modifiers, int key_code);

private:
    class Impl;
    Impl* impl_ = nullptr;
};

} // namespace ahk
