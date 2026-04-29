#include "ahkunix/Signals.hpp"

namespace ahk {

volatile sig_atomic_t g_stop = 0;

void handle_signal(int) {
    g_stop = 1;
}

} // namespace ahk
