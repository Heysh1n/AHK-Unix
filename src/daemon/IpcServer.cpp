#include "ahkunix/daemon/IpcServer.hpp"

#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace
{
    std::system_error errno_error(const char *what)
    {
        return std::system_error(errno, std::generic_category(), what);
    }

    std::string trim_copy(const std::string &text)
    {
        const auto is_space = [](unsigned char ch) {
            return std::isspace(ch) != 0;
        };

        const auto first = std::find_if_not(text.begin(), text.end(), is_space);
        const auto last = std::find_if_not(text.rbegin(), text.rend(), is_space).base();

        if (first >= last)
        {
            return {};
        }

        return {first, last};
    }

    std::string single_line_error(std::string message)
    {
        std::replace(message.begin(), message.end(), '\n', ' ');
        std::replace(message.begin(), message.end(), '\r', ' ');
        return message;
    }

    void close_fd(int &fd) noexcept
    {
        if (fd >= 0)
        {
            ::close(fd);
            fd = -1;
        }
    }
} // namespace

namespace ahkunix::daemon
{

    IpcServer::IpcServer(std::filesystem::path socket_path)
        : socket_path_(std::move(socket_path))
    {
    }

    IpcServer::~IpcServer()
    {
        stop();
    }

    void IpcServer::start(StopCallback on_stop, LoadCallback on_load)
    {
        bool expected = false;
        if (!running_.compare_exchange_strong(expected, true))
        {
            throw std::runtime_error("IPC server is already running");
        }

        on_stop_ = std::move(on_stop);
        on_load_ = std::move(on_load);

        try
        {
            setup_socket();
            worker_ = std::thread(&IpcServer::run, this);
        }
        catch (...)
        {
            running_.store(false);
            close_fd(listen_fd_);
            ::unlink(socket_path_.c_str());
            throw;
        }
    }

    void IpcServer::stop() noexcept
    {
        running_.store(false);

        if (worker_.joinable() && worker_.get_id() != std::this_thread::get_id())
        {
            worker_.join();
        }

        close_fd(listen_fd_);
        ::unlink(socket_path_.c_str());
    }

    bool IpcServer::running() const noexcept
    {
        return running_.load();
    }

    const std::filesystem::path &IpcServer::socket_path() const noexcept
    {
        return socket_path_;
    }

    void IpcServer::setup_socket()
    {
        const std::string path = socket_path_.string();
        if (path.size() >= sizeof(sockaddr_un::sun_path))
        {
            throw std::runtime_error("IPC socket path is too long: " + path);
        }

        const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (fd < 0)
        {
            throw errno_error("socket");
        }
        listen_fd_ = fd;
        set_nonblocking(listen_fd_);

        ::unlink(path.c_str());

        sockaddr_un address {};
        address.sun_family = AF_UNIX;
        std::strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path) - 1);

        if (::bind(listen_fd_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0)
        {
            throw errno_error("bind");
        }

        if (::chmod(path.c_str(), 0600) < 0)
        {
            throw errno_error("chmod");
        }

        if (::listen(listen_fd_, 16) < 0)
        {
            throw errno_error("listen");
        }
    }

    void IpcServer::run() noexcept
    {
        std::cerr << "IPC server listening on " << socket_path_ << '\n';

        while (running_.load())
        {
            pollfd pfd {
                .fd = listen_fd_,
                .events = POLLIN,
                .revents = 0,
            };

            const int ready = ::poll(&pfd, 1, 250);
            if (ready < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                std::cerr << "IPC poll failed: " << std::strerror(errno) << '\n';
                break;
            }

            if (ready == 0)
            {
                continue;
            }

            if ((pfd.revents & POLLIN) != 0)
            {
                accept_pending_clients();
            }

            if ((pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
            {
                break;
            }
        }

        running_.store(false);
    }

    void IpcServer::accept_pending_clients() noexcept
    {
        while (running_.load())
        {
            const int client_fd = ::accept(listen_fd_, nullptr, nullptr);
            if (client_fd < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    return;
                }
                if (errno == EINTR)
                {
                    continue;
                }

                std::cerr << "IPC accept failed: " << std::strerror(errno) << '\n';
                return;
            }

            try
            {
                set_nonblocking(client_fd);
            }
            catch (const std::exception &e)
            {
                std::cerr << "IPC client setup failed: " << e.what() << '\n';
                ::close(client_fd);
                continue;
            }

            handle_client(client_fd);
            ::close(client_fd);
        }
    }

    void IpcServer::handle_client(int client_fd) noexcept
    {
        try
        {
            const std::string command = normalize_command(read_command(client_fd));

            if (command == "PING")
            {
                send_response(client_fd, "PONG\n");
                return;
            }

            if (command == "STOP")
            {
                send_response(client_fd, "OK stopping\n");
                if (on_stop_)
                {
                    on_stop_();
                }
                running_.store(false);
                return;
            }

            if (command.size() >= 5 && command.starts_with("LOAD") &&
                std::isspace(static_cast<unsigned char>(command[4])) != 0)
            {
                const std::string path = trim_copy(command.substr(4));
                if (path.empty())
                {
                    send_response(client_fd, "ERR LOAD requires path\n");
                    return;
                }

                if (!on_load_)
                {
                    send_response(client_fd, "ERR LOAD unavailable\n");
                    return;
                }

                try
                {
                    on_load_(path);
                    send_response(client_fd, "OK loaded\n");
                }
                catch (const std::exception &e)
                {
                    send_response(client_fd, "ERR " + single_line_error(e.what()) + "\n");
                }
                catch (...)
                {
                    send_response(client_fd, "ERR unknown load failure\n");
                }
                return;
            }

            send_response(client_fd, "ERR unknown command\n");
        }
        catch (const std::exception &e)
        {
            std::cerr << "IPC client handling failed: " << e.what() << '\n';
            send_response(client_fd, "ERR failed to process command\n");
        }
    }

    std::string IpcServer::read_command(int client_fd)
    {
        std::string command;
        std::array<char, 256> buffer {};

        while (command.size() < 4096)
        {
            const ssize_t bytes = ::read(client_fd, buffer.data(), buffer.size());
            if (bytes > 0)
            {
                command.append(buffer.data(), static_cast<std::size_t>(bytes));
                if (command.find('\n') != std::string::npos)
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

            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                if (!command.empty())
                {
                    break;
                }

                pollfd pfd {
                    .fd = client_fd,
                    .events = POLLIN,
                    .revents = 0,
                };

                const int ready = ::poll(&pfd, 1, 1000);
                if (ready > 0)
                {
                    continue;
                }
                if (ready == 0)
                {
                    break;
                }
                if (errno == EINTR)
                {
                    continue;
                }

                throw errno_error("poll client");
            }

            throw errno_error("read client");
        }

        return command;
    }

    void IpcServer::send_response(int client_fd, const std::string &response) noexcept
    {
        std::size_t offset = 0;

        while (offset < response.size())
        {
            const ssize_t bytes = ::send(client_fd,
                                         response.data() + offset,
                                         response.size() - offset,
                                         MSG_NOSIGNAL);
            if (bytes > 0)
            {
                offset += static_cast<std::size_t>(bytes);
                continue;
            }

            if (bytes == 0)
            {
                return;
            }

            if (errno == EINTR)
            {
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                pollfd pfd {
                    .fd = client_fd,
                    .events = POLLOUT,
                    .revents = 0,
                };
                if (::poll(&pfd, 1, 1000) >= 0 || errno == EINTR)
                {
                    continue;
                }
            }

            return;
        }
    }

    void IpcServer::set_nonblocking(int fd)
    {
        const int flags = ::fcntl(fd, F_GETFL, 0);
        if (flags < 0)
        {
            throw errno_error("fcntl F_GETFL");
        }

        if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        {
            throw errno_error("fcntl F_SETFL");
        }
    }

    std::string IpcServer::normalize_command(std::string command)
    {
        const auto is_space = [](unsigned char ch) {
            return std::isspace(ch) != 0;
        };

        const auto first = std::find_if_not(command.begin(), command.end(), is_space);
        const auto last = std::find_if_not(command.rbegin(), command.rend(), is_space).base();

        if (first >= last)
        {
            return {};
        }

        std::string normalized(first, last);
        const auto verb_end = std::find_if(normalized.begin(), normalized.end(), is_space);
        std::transform(normalized.begin(), verb_end, normalized.begin(),
                       [](unsigned char ch) {
                           return static_cast<char>(std::toupper(ch));
                       });
        return normalized;
    }

} // namespace ahkunix::daemon
