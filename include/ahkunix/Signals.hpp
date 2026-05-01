#pragma once

#include <csignal>

namespace ahk
{

    extern volatile sig_atomic_t g_stop;

    void handle_signal(int);

    // Daemonize the process: fork into background, detach from parent,
    // create new session, and redirect file descriptors.
    // nofork=true skips forking (useful for debugging/testing)
    void daemonize(bool nofork = false);

} // namespace ahk
