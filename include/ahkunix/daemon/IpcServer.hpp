#pragma once

#include <atomic>
#include <filesystem>
#include <functional>
#include <string>
#include <thread>

namespace ahkunix::daemon
{

    class IpcServer
    {
    public:
        using StopCallback = std::function<void()>;
        using LoadCallback = std::function<void(std::string)>;
<<<<<<< HEAD
=======
        using StatusCallback = std::function<std::string()>;
>>>>>>> master

        static constexpr const char *default_socket_path = "/tmp/ahkunix.sock";

        explicit IpcServer(std::filesystem::path socket_path = default_socket_path);
        ~IpcServer();

        IpcServer(const IpcServer &) = delete;
        IpcServer &operator=(const IpcServer &) = delete;

<<<<<<< HEAD
        void start(StopCallback on_stop, LoadCallback on_load);
=======
        void start(StopCallback on_stop, LoadCallback on_load, StatusCallback on_status);
>>>>>>> master
        void stop() noexcept;

        bool running() const noexcept;
        const std::filesystem::path &socket_path() const noexcept;

    private:
        void setup_socket();
        void run() noexcept;
        void accept_pending_clients() noexcept;
        void handle_client(int client_fd) noexcept;

        static std::string read_command(int client_fd);
        static void send_response(int client_fd, const std::string &response) noexcept;
        static void set_nonblocking(int fd);
        static std::string normalize_command(std::string command);

        std::filesystem::path socket_path_;
        StopCallback on_stop_;
        LoadCallback on_load_;
<<<<<<< HEAD
=======
        StatusCallback on_status_;
>>>>>>> master
        std::thread worker_;
        std::atomic_bool running_{false};
        int listen_fd_{-1};
    };

} // namespace ahkunix::daemon
