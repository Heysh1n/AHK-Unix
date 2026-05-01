#include "ahkunix/AhkParser.hpp"
#include "ahkunix/Daemon.hpp"
#include "ahkunix/KeyboardDetect.hpp"
#include "ahkunix/LayoutProfile.hpp"
#include "ahkunix/Signals.hpp"

#include <csignal>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
    void print_architecture()
    {
        std::cerr
            << "Architecture:\n"
            << "  evdev physical keyboard -> RawEvent -> transparent uinput forwarding\n"
            << "  RawEvent press -> RingBuffer -> HotstringMatcher -> clipboard + Ctrl+V\n"
            << "  Parser maps AHK hotstrings into physical key sequences using LayoutProfile\n\n";
    }

    void print_usage(const char *argv0)
    {
        std::cerr
            << "Usage:\n"
            << "  " << argv0 << " [options] script.ahkl\n"
            << "  " << argv0 << " --device /dev/input/eventX [options] script.ahkl\n"
            << "\n"
            << "Options:\n"
            << "  --no-daemon       Run in foreground (default: daemonize)\n"
            << "  --strict          Strict parser mode (invalid If/Else case => fatal)\n"
            << "  --lint            Validate script and exit (no device grab)\n";
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

        std::signal(SIGINT, ahk::handle_signal);
        std::signal(SIGTERM, ahk::handle_signal);

        std::filesystem::path device;
        std::filesystem::path script;
        bool no_daemon = false;
        bool strict_mode = false;
        bool lint_only = false;

        int i = 1;
        while (i < argc)
        {
            const std::string arg(argv[i]);

            if (arg == "--no-daemon")
            {
                no_daemon = true;
                ++i;
                continue;
            }

            if (arg == "--strict")
            {
                strict_mode = true;
                ++i;
                continue;
            }

            if (arg == "--lint")
            {
                lint_only = true;
                ++i;
                continue;
            }

            if (arg == "--device")
            {
                if (i + 2 >= argc)
                {
                    throw std::runtime_error("--device requires path and script");
                }
                device = argv[++i];
                script = argv[++i];
                ++i;
                continue;
            }

            if (arg.starts_with("/dev/input/"))
            {
                device = arg;
                if (i + 1 >= argc)
                {
                    throw std::runtime_error("script path required");
                }
                script = argv[++i];
                ++i;
                continue;
            }

            if (arg.starts_with("--"))
            {
                throw std::runtime_error("unknown option: " + arg);
            }

            if (script.empty())
            {
                script = arg;
            }

            ++i;
        }

        if (script.empty())
        {
            print_usage(argv[0]);
            return 2;
        }

        const auto layout = ahk::LayoutProfile::russian_qwerty();

        if (lint_only)
        {
            auto hotstrings = ahk::AhkParser::parse_file(script, layout, strict_mode);
            std::cerr << "Lint OK. Parsed hotstrings: " << hotstrings.size() << "\n";
            return 0;
        }

        if (device.empty())
        {
            device = ahk::autodetect_keyboard();
        }

        ahk::daemonize(no_daemon);

        print_architecture();

        auto hotstrings = ahk::AhkParser::parse_file(script, layout, strict_mode);

        std::cerr << "Script: " << script << "\n";
        std::cerr << "Device: " << device << "\n";

        ahk::Daemon daemon(device, std::move(hotstrings));
        daemon.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "fatal: " << e.what() << "\n";
        return 1;
    }
}