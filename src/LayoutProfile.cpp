#include "ahkunix/LayoutProfile.hpp"

#include <linux/input-event-codes.h>

#include <stdexcept>

namespace ahk {

LayoutProfile LayoutProfile::russian_qwerty() {
    LayoutProfile profile;

    profile.char_to_key_ = {
        {"й", KEY_Q}, {"ц", KEY_W}, {"у", KEY_E}, {"к", KEY_R}, {"е", KEY_T},
        {"н", KEY_Y}, {"г", KEY_U}, {"ш", KEY_I}, {"щ", KEY_O}, {"з", KEY_P},
        {"х", KEY_LEFTBRACE}, {"ъ", KEY_RIGHTBRACE},

        {"ф", KEY_A}, {"ы", KEY_S}, {"в", KEY_D}, {"а", KEY_F}, {"п", KEY_G},
        {"р", KEY_H}, {"о", KEY_J}, {"л", KEY_K}, {"д", KEY_L}, {"ж", KEY_SEMICOLON},
        {"э", KEY_APOSTROPHE},

        {"я", KEY_Z}, {"ч", KEY_X}, {"с", KEY_C}, {"м", KEY_V}, {"и", KEY_B},
        {"т", KEY_N}, {"ь", KEY_M}, {"б", KEY_COMMA}, {"ю", KEY_DOT},

        {"1", KEY_1}, {"2", KEY_2}, {"3", KEY_3}, {"4", KEY_4}, {"5", KEY_5},
        {"6", KEY_6}, {"7", KEY_7}, {"8", KEY_8}, {"9", KEY_9}, {"0", KEY_0},
    };

    return profile;
}

int LayoutProfile::key_for_utf8_char(const std::string& ch) const {
    auto it = char_to_key_.find(ch);
    if (it == char_to_key_.end()) {
        throw std::runtime_error("no key mapping for trigger char: " + ch);
    }
    return it->second;
}

} // namespace ahk
