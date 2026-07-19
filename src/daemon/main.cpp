#include "ahkunix/AhkParser.hpp"
#include "ahkunix/Daemon.hpp"
#include "ahkunix/KeyboardDetect.hpp"
#include "ahkunix/LayoutProfile.hpp"
#include "ahkunix/Signals.hpp"
#include "ahkunix/daemon/Daemonizer.hpp"
#include "ahkunix/daemon/IpcServer.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace
{
    struct Options
    {
        std::filesystem::path device;
        std::filesystem::path script;
        bool foreground = false;
        bool strict_mode = false;
    };

    std::system_error errno_error(const char *what)
    {
        return std::system_error(errno, std::generic_category(), what);
    }

    void print_usage(const char *argv0)
    {
        std::cerr
            << "Usage:\n"
            << "  " << argv0 << " [--device /dev/input/eventX] [--strict] [--foreground|--no-daemon] [script.ahkl]\n"
            << "\n"
            << "The daemon listens on /tmp/ahkunix.sock and logs to /tmp/ahkunix.log when daemonized.\n";
    }

    void install_ahk_stop_handler(int signo)
    {
        struct sigaction action {};
        action.sa_handler = ahk::handle_signal;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;

        if (::sigaction(signo, &action, nullptr) < 0)
        {
            throw errno_error("sigaction");
        }
    }

    Options parse_options(int argc, char **argv)
    {
        Options options;
        for (int i = 1; i < argc; ++i)
        {
            const std::string arg(argv[i]);
            if (arg == "--foreground" || arg == "--no-daemon")
            {
                options.foreground = true;
                continue;
            }

            if (arg == "--strict")
            {
                options.strict_mode = true;
                continue;
            }

            if (arg == "--device")
            {
                if (i + 1 >= argc)
                {
                    throw std::runtime_error("--device requires a path");
                }
                options.device = argv[++i];
                continue;
            }

            if (arg == "--help" || arg == "-h")
            {
                print_usage(argv[0]);
                std::exit(0);
            }

            if (arg.starts_with("--"))
            {
                throw std::runtime_error("unknown option: " + arg);
            }

            if (!options.script.empty())
            {
                throw std::runtime_error("multiple script paths provided");
            }

            options.script = arg;
        }

        return options;
    }
} // namespace

int main(int argc, char **argv)
{
    try
    {
        auto options = parse_options(argc, argv);
        const auto layout = ahk::LayoutProfile::russian_qwerty();
        std::vector<ahk::Hotstring> hotstrings;
        if (!options.script.empty())
        {
            hotstrings = ahk::AhkParser::parse_file(options.script, layout, options.strict_mode);
        }

        if (options.device.empty())
        {
            options.device = ahk::autodetect_keyboard();
        }

        // ЗАЩИТА ОТ ДЕМЕНЦИИ: Проверяем, жив ли сокет от предыдущего демона
        {
            int check_fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
            if (check_fd >= 0) {
                sockaddr_un addr {};
                addr.sun_family = AF_UNIX;
                std::strncpy(addr.sun_path, "/tmp/ahkunix.sock", sizeof(addr.sun_path) - 1);
                if (::connect(check_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
                    std::cerr << "fatal: Daemon is already running (active socket found)\n";
                    ::close(check_fd);
                    return 1;
                }
                ::close(check_fd);
            }
        }

        ahkunix::daemon::Daemonizer daemonizer;
        daemonizer.install_signal_handlers();
        install_ahk_stop_handler(SIGINT);
        install_ahk_stop_handler(SIGTERM);
        daemonizer.daemonize(options.foreground);

        ahk::Daemon daemon(options.device, std::move(hotstrings));
        ahkunix::daemon::IpcServer ipc_server;
        daemonizer.add_cleanup_handler([&ipc_server] {
            ipc_server.stop();
        });

        std::mutex state_mutex;
        std::string active_script = options.script.empty() ? "[IDLE - NO SCRIPT LOADED]" : options.script.string();
        std::time_t load_time = std::time(nullptr);

        ipc_server.start(
            [&daemonizer] {
                daemonizer.request_shutdown();
                ahk::g_stop = 1;
            },
            [&](std::string path) {
                daemon.reload_script(path, options.strict_mode);
                std::lock_guard lock(state_mutex);
                active_script = path;
                load_time = std::time(nullptr);
            },
            [&]() -> std::string {
                std::lock_guard lock(state_mutex);
                std::string time_str = std::ctime(&load_time);
                time_str.pop_back();

                return "PONG\n"
                       "════════════════════════════════════════\n"
                       " 🟢 AHKUnix Daemon (v0.6.0)\n"
                       "════════════════════════════════════════\n"
                       " Script: " + active_script + "\n"
                       " Loaded: " + time_str + "\n"
                       " Device: " + options.device.string() + "\n"
                       " PID:    " + std::to_string(::getpid()) + "\n"
                       " Action: ahkunixctl load <path/to/script>\n"
                       "════════════════════════════════════════\n";
            });

        std::cerr << "AHKUnix daemon started.\n";
        std::cerr << "Script: " << options.script << '\n';
        std::cerr << "Device: " << options.device << '\n';

        daemon.run();

        std::cerr << "AHKUnix daemon stopping.\n";
        daemonizer.request_shutdown();
        daemonizer.run_cleanup();
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "fatal: " << e.what() << '\n';
        return 1;
    }
}
