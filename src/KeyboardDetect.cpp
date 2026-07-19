#include "ahkunix/KeyboardDetect.hpp"

#include "ahkunix/Errors.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

namespace ahk
{

    std::filesystem::path autodetect_keyboard()
    {
        std::ifstream in("/proc/bus/input/devices");
        if (!in)
        {
            throw SysError("open /proc/bus/input/devices");
        }

        std::string line;
        std::string name;
        while (std::getline(in, line))
        {
            if (line.starts_with("N: Name="))
            {
                name = line;
                continue;
            }

            if (!line.starts_with("H: Handlers="))
            {
                continue;
            }

            if (line.find("kbd") == std::string::npos)
            {
                continue;
            }

            // Skip touchpad and mouse devices - they should not be grabbed
            if (name.find("Touchpad") != std::string::npos ||
                name.find("TouchPad") != std::string::npos ||
                name.find("trackpad") != std::string::npos ||
                name.find("Trackpad") != std::string::npos ||
                name.find("Mouse") != std::string::npos ||
                name.find("mouse") != std::string::npos)
            {
                continue;
            }

            // Skip system buttons
            if (name.find("Power Button") != std::string::npos || name.find("Sleep Button") != std::string::npos)
            {
                continue;
            }

            const auto event_pos = line.find("event");
            if (event_pos == std::string::npos)
            {
                continue;
            }

            std::size_t end = event_pos;
            while (end < line.size() && line[end] != ' ' && line[end] != '\t')
            {
                ++end;
            }

            const auto event_name = line.substr(event_pos, end - event_pos);
            return std::filesystem::path("/dev/input") / event_name;
        }

        throw std::runtime_error("no keyboard event device found; pass --device /dev/input/eventX");
    }

} // namespace ahk
