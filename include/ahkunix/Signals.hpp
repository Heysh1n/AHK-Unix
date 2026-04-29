#pragma once

#include <csignal>

namespace ahk {

extern volatile sig_atomic_t g_stop;

void handle_signal(int);

} // namespace ahk
