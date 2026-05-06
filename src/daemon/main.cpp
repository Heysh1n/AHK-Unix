#include "ahkunix/daemon/Daemonizer.hpp"
#include "ahkunix/daemon/IpcServer.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
    void print_usage(const char *argv0)
    {
        std::cerr
            << "Usage:\n"
            << "  " << argv0 << " [--foreground|--no-daemon]\n"
            << "\n"
            << "The daemon listens on /tmp/ahkunix.sock and logs to /tmp/ahkunix.log.\n";
    }

    bool parse_foreground(int argc, char **argv)
    {
        bool foreground = false;

        for (int i = 1; i < argc; ++i)
        {
            const std::string arg(argv[i]);
            if (arg == "--foreground" || arg == "--no-daemon")
            {
                foreground = true;
                continue;
            }

            if (arg == "--help" || arg == "-h")
            {
                print_usage(argv[0]);
                std::exit(0);
            }

            throw std::runtime_error("unknown option: " + arg);
        }

        return foreground;
    }
} // namespace

int main(int argc, char **argv)
{
    try
    {
        const bool foreground = parse_foreground(argc, argv);

        ahkunix::daemon::Daemonizer daemonizer;
        daemonizer.install_signal_handlers();
        daemonizer.daemonize(foreground);

        ahkunix::daemon::IpcServer ipc_server;
        daemonizer.add_cleanup_handler([&ipc_server] {
            ipc_server.stop();
        });

        daemonizer.add_cleanup_handler([] {
            // Register the real libevdev cleanup here when the existing event loop is moved in:
            // physical_keyboard.ungrab() or EVIOCREVOKE-style revocation belongs in this hook.
            std::cerr << "Input cleanup hook completed.\n";
        });

        ipc_server.start([&daemonizer] {
            daemonizer.request_shutdown();
        });

        std::cerr << "AHKUnix daemon started.\n";

        while (!daemonizer.shutdown_requested())
        {
            // Placeholder for the existing libevdev/uinput event loop.
            // Keep this loop responsive so SIGINT, SIGTERM, and IPC STOP unwind cleanly.
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        std::cerr << "AHKUnix daemon stopping.\n";
        daemonizer.run_cleanup();
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "fatal: " << e.what() << '\n';
        return 1;
    }
}
