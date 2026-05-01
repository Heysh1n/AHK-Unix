#include "ahkunix/LayoutProfile.hpp"

#include <linux/input-event-codes.h>

#include <stdexcept>

namespace ahk
{

    LayoutProfile LayoutProfile::russian_qwerty()
    {
        LayoutProfile profile;

        profile.char_to_key_ = {
            {"й", KEY_Q},
            {"ц", KEY_W},
            {"у", KEY_E},
            {"к", KEY_R},
            {"е", KEY_T},
            {"н", KEY_Y},
            {"г", KEY_U},
            {"ш", KEY_I},
            {"щ", KEY_O},
            {"з", KEY_P},
            {"х", KEY_LEFTBRACE},
            {"ъ", KEY_RIGHTBRACE},

            {"ф", KEY_A},
            {"ы", KEY_S},
            {"в", KEY_D},
            {"а", KEY_F},
            {"п", KEY_G},
            {"р", KEY_H},
            {"о", KEY_J},
            {"л", KEY_K},
            {"д", KEY_L},
            {"ж", KEY_SEMICOLON},
            {"э", KEY_APOSTROPHE},

            {"я", KEY_Z},
            {"ч", KEY_X},
            {"с", KEY_C},
            {"м", KEY_V},
            {"и", KEY_B},
            {"т", KEY_N},
            {"ь", KEY_M},
            {"б", KEY_COMMA},
            {"ю", KEY_DOT},

            {"1", KEY_1},
            {"2", KEY_2},
            {"3", KEY_3},
            {"4", KEY_4},
            {"5", KEY_5},
            {"6", KEY_6},
            {"7", KEY_7},
            {"8", KEY_8},
            {"9", KEY_9},
            {"0", KEY_0},
        };

        return profile;
    }

    int LayoutProfile::key_for_utf8_char(const std::string &ch) const
    {
        auto it = char_to_key_.find(ch);
        if (it == char_to_key_.end())
        {
            throw std::runtime_error("no key mapping for trigger char: " + ch);
        }
        return it->second;
    }

    int LayoutProfile::parse_special_key(const std::string &key_name)
    {
        // NumPad keys
        if (key_name == "NumPad0" || key_name == "Numpad0")
            return KEY_KP0;
        if (key_name == "NumPad1" || key_name == "Numpad1")
            return KEY_KP1;
        if (key_name == "NumPad2" || key_name == "Numpad2")
            return KEY_KP2;
        if (key_name == "NumPad3" || key_name == "Numpad3")
            return KEY_KP3;
        if (key_name == "NumPad4" || key_name == "Numpad4")
            return KEY_KP4;
        if (key_name == "NumPad5" || key_name == "Numpad5")
            return KEY_KP5;
        if (key_name == "NumPad6" || key_name == "Numpad6")
            return KEY_KP6;
        if (key_name == "NumPad7" || key_name == "Numpad7")
            return KEY_KP7;
        if (key_name == "NumPad8" || key_name == "Numpad8")
            return KEY_KP8;
        if (key_name == "NumPad9" || key_name == "Numpad9")
            return KEY_KP9;
        if (key_name == "NumPadAdd")
            return KEY_KPPLUS;
        if (key_name == "NumPadMult")
            return KEY_KPASTERISK;
        if (key_name == "NumPadDiv")
            return KEY_KPSLASH;
        if (key_name == "NumPadSub")
            return KEY_KPMINUS;

        // Function keys
        if (key_name == "F1")
            return KEY_F1;
        if (key_name == "F2")
            return KEY_F2;
        if (key_name == "F3")
            return KEY_F3;
        if (key_name == "F4")
            return KEY_F4;
        if (key_name == "F5")
            return KEY_F5;
        if (key_name == "F6")
            return KEY_F6;
        if (key_name == "F7")
            return KEY_F7;
        if (key_name == "F8")
            return KEY_F8;
        if (key_name == "F9")
            return KEY_F9;
        if (key_name == "F10")
            return KEY_F10;
        if (key_name == "F11")
            return KEY_F11;
        if (key_name == "F12")
            return KEY_F12;

        // Navigation keys
        if (key_name == "Left")
            return KEY_LEFT;
        if (key_name == "Right")
            return KEY_RIGHT;
        if (key_name == "Up")
            return KEY_UP;
        if (key_name == "Down")
            return KEY_DOWN;
        if (key_name == "Home")
            return KEY_HOME;
        if (key_name == "End")
            return KEY_END;
        if (key_name == "PageUp")
            return KEY_PAGEUP;
        if (key_name == "PageDown")
            return KEY_PAGEDOWN;
        if (key_name == "Insert")
            return KEY_INSERT;
        if (key_name == "Delete" || key_name == "Del")
            return KEY_DELETE;

        // Special keys
        if (key_name == "Space")
            return KEY_SPACE;
        if (key_name == "Enter" || key_name == "Return")
            return KEY_ENTER;
        if (key_name == "Tab")
            return KEY_TAB;
        if (key_name == "Backspace" || key_name == "BS")
            return KEY_BACKSPACE;
        if (key_name == "Escape" || key_name == "Esc")
            return KEY_ESC;

        throw std::runtime_error("unknown special key: " + key_name);
    }

    int LayoutProfile::parse_modifier_key(const std::string &mod_name)
    {
        if (mod_name == "Alt")
            return KEY_LEFTALT;
        if (mod_name == "LCtrl" || mod_name == "Ctrl")
            return KEY_LEFTCTRL;
        if (mod_name == "Shift")
            return KEY_LEFTSHIFT;
        if (mod_name == "Meta" || mod_name == "Super" || mod_name == "Win")
            return KEY_LEFTMETA;

        throw std::runtime_error("unknown modifier key: " + mod_name);
    }

} // namespace ahk
