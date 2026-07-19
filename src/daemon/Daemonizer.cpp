#include "ahkunix/daemon/Daemonizer.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace
{
    volatile std::sig_atomic_t g_signal_shutdown_requested = 0;

    [[noreturn]] void child_exit_success() noexcept
    {
        ::_exit(0);
    }

    std::system_error errno_error(const char *what)
    {
        return std::system_error(errno, std::generic_category(), what);
    }
} // namespace

namespace ahkunix::daemon
{

    Daemonizer::~Daemonizer()
    {
        run_cleanup();
    }

    void Daemonizer::daemonize(bool foreground, const std::filesystem::path &log_path)
    {
        if (foreground)
        {
            return;
        }

        pid_t pid = ::fork();
        if (pid < 0)
        {
            throw errno_error("fork");
        }
        if (pid > 0)
        {
            child_exit_success();
        }

        if (::setsid() < 0)
        {
            throw errno_error("setsid");
        }

        pid = ::fork();
        if (pid < 0)
        {
            throw errno_error("fork");
        }
        if (pid > 0)
        {
            child_exit_success();
        }

        if (::chdir("/") < 0)
        {
            throw errno_error("chdir");
        }

        ::umask(0);
        redirect_standard_fds(log_path);
    }

    void Daemonizer::install_signal_handlers()
    {
        install_handler(SIGINT);
        install_handler(SIGTERM);

        struct sigaction ignore_pipe {};
        ignore_pipe.sa_handler = SIG_IGN;
        sigemptyset(&ignore_pipe.sa_mask);
        ignore_pipe.sa_flags = 0;
        if (::sigaction(SIGPIPE, &ignore_pipe, nullptr) < 0)
        {
            throw errno_error("sigaction SIGPIPE");
        }
    }

    void Daemonizer::request_shutdown() noexcept
    {
        shutdown_requested_.store(true);
    }

    bool Daemonizer::shutdown_requested() const noexcept
    {
        return shutdown_requested_.load() || g_signal_shutdown_requested != 0;
    }

    void Daemonizer::add_cleanup_handler(std::function<void()> handler)
    {
        std::lock_guard lock(cleanup_mutex_);
        cleanup_handlers_.push_back(std::move(handler));
    }

    void Daemonizer::run_cleanup() noexcept
    {
        bool expected = false;
        if (!cleanup_ran_.compare_exchange_strong(expected, true))
        {
            return;
        }

        std::vector<std::function<void()>> handlers;
        {
            std::lock_guard lock(cleanup_mutex_);
            handlers.swap(cleanup_handlers_);
        }

        for (auto it = handlers.rbegin(); it != handlers.rend(); ++it)
        {
            try
            {
                (*it)();
            }
            catch (const std::exception &e)
            {
                std::cerr << "cleanup handler failed: " << e.what() << '\n';
            }
            catch (...)
            {
                std::cerr << "cleanup handler failed with an unknown exception\n";
            }
        }
    }

    void Daemonizer::signal_handler(int) noexcept
    {
        g_signal_shutdown_requested = 1;
    }

    void Daemonizer::install_handler(int signo)
    {
        struct sigaction action {};
        action.sa_handler = &Daemonizer::signal_handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;

        if (::sigaction(signo, &action, nullptr) < 0)
        {
            throw errno_error("sigaction");
        }
    }

    void Daemonizer::redirect_standard_fds(const std::filesystem::path &log_path)
    {
        const int null_fd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        if (null_fd < 0)
        {
            throw errno_error("open /dev/null");
        }

        const int log_fd = ::open(log_path.c_str(),
                                  O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC,
                                  0644);
        if (log_fd < 0)
        {
            const int saved_errno = errno;
            ::close(null_fd);
            errno = saved_errno;
            throw errno_error("open log");
        }

        if (::dup2(null_fd, STDIN_FILENO) < 0 ||
            ::dup2(log_fd, STDOUT_FILENO) < 0 ||
            ::dup2(log_fd, STDERR_FILENO) < 0)
        {
            const int saved_errno = errno;
            ::close(null_fd);
            ::close(log_fd);
            errno = saved_errno;
            throw errno_error("dup2");
        }

        ::close(null_fd);
        ::close(log_fd);
    }

} // namespace ahkunix::daemon
