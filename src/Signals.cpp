#include "ahkunix/Signals.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>

namespace ahk
{
    volatile sig_atomic_t g_stop = 0;

    void handle_signal(int)
    {
        g_stop = 1;
    }

    void daemonize(bool nofork)
    {
        if (nofork)
        {
            return;
        }

        pid_t pid = ::fork();
        if (pid < 0)
        {
            std::perror("fork (1)");
            std::exit(1);
        }
        if (pid > 0)
        {
            std::exit(0);
        }

        if (::setsid() < 0)
        {
            std::perror("setsid");
            std::exit(1);
        }

        pid = ::fork();
        if (pid < 0)
        {
            std::perror("fork (2)");
            std::exit(1);
        }
        if (pid > 0)
        {
            std::exit(0);
        }

        if (::chdir("/") < 0)
        {
            std::perror("chdir");
            std::exit(1);
        }

        ::umask(0);

        const int devnull = ::open("/dev/null", O_RDWR);
        if (devnull < 0)
        {
            std::perror("open /dev/null");
            std::exit(1);
        }

        if (::dup2(devnull, STDIN_FILENO) < 0 ||
            ::dup2(devnull, STDOUT_FILENO) < 0 ||
            ::dup2(devnull, STDERR_FILENO) < 0)
        {
            std::perror("dup2");
            std::exit(1);
        }

        if (devnull > STDERR_FILENO)
        {
            ::close(devnull);
        }
    }
} // namespace ahk