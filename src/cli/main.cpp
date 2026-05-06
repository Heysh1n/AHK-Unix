#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
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
            << "  " << argv0 << " stop\n";
    }

    std::string to_daemon_command(std::string command)
    {
        if (command == "ping" || command == "PING")
        {
            return "PING\n";
        }

        if (command == "stop" || command == "STOP")
        {
            return "STOP\n";
        }

        throw std::runtime_error("unknown command: " + command);
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
} // namespace

int main(int argc, char **argv)
{
    try
    {
        if (argc != 2)
        {
            print_usage(argv[0]);
            return 2;
        }

        const std::string command = to_daemon_command(argv[1]);
        const int fd = connect_to_daemon();

        write_all(fd, command);
        const std::string response = read_response(fd);
        ::close(fd);

        std::cout << response;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "fatal: " << e.what() << '\n';
        return 1;
    }
}
