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

namespace {

void print_architecture() {
    std::cerr
        << "Architecture:\n"
        << "  evdev physical keyboard -> RawEvent -> transparent uinput forwarding\n"
        << "  RawEvent press -> RingBuffer -> HotstringMatcher -> clipboard + Ctrl+V\n"
        << "  Parser maps AHK hotstrings into physical key sequences using LayoutProfile\n\n";
}

void print_usage(const char* argv0) {
    std::cerr
        << "Usage:\n"
        << "  " << argv0 << " script.ahkl\n"
        << "  " << argv0 << " --device /dev/input/eventX script.ahkl\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            print_usage(argv[0]);
            return 2;
        }

        std::signal(SIGINT, ahk::handle_signal);
        std::signal(SIGTERM, ahk::handle_signal);

        std::filesystem::path device;
        std::filesystem::path script;

        if (argc == 4 && std::string(argv[1]) == "--device") {
            device = argv[2];
            script = argv[3];
        } else if (argc == 3 && std::string(argv[1]).starts_with("/dev/input/")) {
            device = argv[1];
            script = argv[2];
        } else if (argc == 2) {
            device = ahk::autodetect_keyboard();
            script = argv[1];
        } else {
            throw std::runtime_error("invalid arguments");
        }

        print_architecture();

        const auto layout = ahk::LayoutProfile::russian_qwerty();
        auto hotstrings = ahk::AhkParser::parse_file(script, layout);

        std::cerr << "Script: " << script << "\n";
        std::cerr << "Device: " << device << "\n";

        ahk::Daemon daemon(device, std::move(hotstrings));
        daemon.run();
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << "\n";
        return 1;
    }
}
