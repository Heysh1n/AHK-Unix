#include "ahkunix/AhkParser.hpp"
#include "ahkunix/Daemon.hpp"
#include "ahkunix/KeyboardDetect.hpp"
#include "ahkunix/LayoutProfile.hpp"
#include "ahkunix/Signals.hpp"
#include "ahkunix/daemon/Daemonizer.hpp"
#include "ahkunix/daemon/IpcServer.hpp"

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

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
            << "  " << argv0 << " [--device /dev/input/eventX] [--strict] [--foreground|--no-daemon] script.ahkl\n"
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

        if (options.script.empty())
        {
            throw std::runtime_error("script path required");
        }

        return options;
    }
} // namespace

int main(int argc, char **argv)
{
    try
    {
        if (argc < 2)
        {
            print_usage(argv[0]);
            return 2;
        }

        auto options = parse_options(argc, argv);
        const auto layout = ahk::LayoutProfile::russian_qwerty();
        auto hotstrings = ahk::AhkParser::parse_file(options.script, layout, options.strict_mode);

        if (options.device.empty())
        {
            options.device = ahk::autodetect_keyboard();
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

        ipc_server.start(
            [&daemonizer] {
                daemonizer.request_shutdown();
                ahk::g_stop = 1;
            },
            [&daemon, strict_mode = options.strict_mode](std::string path) {
                daemon.reload_script(path, strict_mode);
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
