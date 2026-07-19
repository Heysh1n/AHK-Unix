#pragma once

#include <atomic>
#include <csignal>
#include <filesystem>
#include <functional>
#include <mutex>
#include <vector>

namespace ahkunix::daemon
{

    class Daemonizer
    {
    public:
        static constexpr const char *default_log_path = "/tmp/ahkunix.log";

        Daemonizer() = default;
        ~Daemonizer();

        Daemonizer(const Daemonizer &) = delete;
        Daemonizer &operator=(const Daemonizer &) = delete;

        void daemonize(bool foreground = false,
                       const std::filesystem::path &log_path = default_log_path);
        void install_signal_handlers();

        void request_shutdown() noexcept;
        bool shutdown_requested() const noexcept;

        void add_cleanup_handler(std::function<void()> handler);
        void run_cleanup() noexcept;

    private:
        static void signal_handler(int signo) noexcept;
        static void install_handler(int signo);
        static void redirect_standard_fds(const std::filesystem::path &log_path);

        std::atomic_bool shutdown_requested_{false};
        std::atomic_bool cleanup_ran_{false};
        std::mutex cleanup_mutex_;
        std::vector<std::function<void()>> cleanup_handlers_;
    };

} // namespace ahkunix::daemon
