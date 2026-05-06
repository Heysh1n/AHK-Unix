#include "ahkunix/AhkParser.hpp"
#include "ahkunix/Fd.hpp"
#include "ahkunix/LayoutProfile.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace
{
    constexpr const char *socket_path = "/tmp/ahkunix.sock";

    std::system_error errno_error(const char *what)
    {
        return std::system_error(errno, std::generic_category(), what);
    }

    void print_usage(const char *argv0)
    {
        std::cerr
            << "Usage:\n"
            << "  " << argv0 << " ping\n"
            << "  " << argv0 << " stop\n"
            << "  " << argv0 << " load script.ahkl\n"
            << "  " << argv0 << " lint [--strict] script.ahkl\n";
    }

    void write_all(int fd, const std::string &message)
    {
        std::size_t offset = 0;
        while (offset < message.size())
        {
            const ssize_t bytes = ::write(fd, message.data() + offset, message.size() - offset);
            if (bytes > 0)
            {
                offset += static_cast<std::size_t>(bytes);
                continue;
            }

            if (bytes < 0 && errno == EINTR)
            {
                continue;
            }

            throw errno_error("write");
        }
    }

    std::string read_response(int fd)
    {
        std::string response;
        char buffer[512] {};

        while (true)
        {
            const ssize_t bytes = ::read(fd, buffer, sizeof(buffer));
            if (bytes > 0)
            {
                response.append(buffer, static_cast<std::size_t>(bytes));
                if (response.find('\n') != std::string::npos)
                {
                    break;
                }
                continue;
            }

            if (bytes == 0)
            {
                break;
            }

            if (errno == EINTR)
            {
                continue;
            }

            throw errno_error("read");
        }

        return response;
    }

    int connect_to_daemon()
    {
        const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (fd < 0)
        {
            throw errno_error("socket");
        }

        sockaddr_un address {};
        address.sun_family = AF_UNIX;
        std::strncpy(address.sun_path, socket_path, sizeof(address.sun_path) - 1);

        if (::connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0)
        {
            const int saved_errno = errno;
            ::close(fd);
            errno = saved_errno;
            throw errno_error("connect /tmp/ahkunix.sock");
        }

        return fd;
    }

    bool response_is_error(const std::string &response)
    {
        return response.starts_with("ERR");
    }

    int send_daemon_command(const std::string &command)
    {
        ahk::Fd fd(connect_to_daemon());

        write_all(fd.get(), command);
        const std::string response = read_response(fd.get());

        if (response_is_error(response))
        {
            std::cerr << response;
            return 1;
        }

        std::cout << response;
        return 0;
    }

    int run_lint(int argc, char **argv)
    {
        bool strict_mode = false;
        std::filesystem::path script;

        for (int i = 2; i < argc; ++i)
        {
            const std::string arg(argv[i]);
            if (arg == "--strict")
            {
                strict_mode = true;
                continue;
            }

            if (arg.starts_with("--"))
            {
                throw std::runtime_error("unknown lint option: " + arg);
            }

            if (!script.empty())
            {
                throw std::runtime_error("multiple script paths provided");
            }

            script = arg;
        }

        if (script.empty())
        {
            throw std::runtime_error("lint requires a script path");
        }

        const auto layout = ahk::LayoutProfile::russian_qwerty();
        (void)ahk::AhkParser::parse_file(script, layout, strict_mode);
        std::cout << "Lint OK\n";
        return 0;
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

        const std::string command = argv[1];

        if (command == "ping")
        {
            if (argc != 2)
            {
                print_usage(argv[0]);
                return 2;
            }
            return send_daemon_command("PING\n");
        }

        if (command == "stop")
        {
            if (argc != 2)
            {
                print_usage(argv[0]);
                return 2;
            }
            return send_daemon_command("STOP\n");
        }

        if (command == "load")
        {
            if (argc != 3)
            {
                print_usage(argv[0]);
                return 2;
            }

            const std::filesystem::path script = std::filesystem::absolute(argv[2]);
            return send_daemon_command("LOAD " + script.string() + "\n");
        }

        if (command == "lint")
        {
            return run_lint(argc, argv);
        }

        print_usage(argv[0]);
        return 2;
    }
    catch (const std::exception &e)
    {
        std::cerr << "fatal: " << e.what() << '\n';
        return 1;
    }
}
