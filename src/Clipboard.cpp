#include "ahkunix/Clipboard.hpp"

#include "ahkunix/Errors.hpp"
#include "ahkunix/Fd.hpp"

#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <filesystem>

extern char** environ;

namespace ahk {

void Clipboard::set_text(const std::string& text) const {
    if (command_exists("wl-copy") && has_env("WAYLAND_DISPLAY")) {
        run_with_stdin({"wl-copy"}, text);
        return;
    }

    if (command_exists("xclip") && has_env("DISPLAY")) {
        run_with_stdin({"xclip", "-selection", "clipboard"}, text);
        return;
    }

    if (command_exists("xsel") && has_env("DISPLAY")) {
        run_with_stdin({"xsel", "--clipboard", "--input"}, text);
        return;
    }

    throw std::runtime_error("no clipboard backend found: install wl-clipboard, xclip, or xsel");
}

bool Clipboard::has_env(const char* name) {
    const char* value = std::getenv(name);
    return value && value[0] != '\0';
}

bool Clipboard::command_exists(const char* name) {
    const char* path = std::getenv("PATH");
    if (!path) {
        return false;
    }

    std::string paths(path);
    std::size_t start = 0;
    while (start <= paths.size()) {
        const auto end = paths.find(':', start);
        const auto dir = paths.substr(start, end == std::string::npos ? std::string::npos : end - start);
        const auto candidate = std::filesystem::path(dir.empty() ? "." : dir) / name;
        if (::access(candidate.c_str(), X_OK) == 0) {
            return true;
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return false;
}

void Clipboard::run_with_stdin(const std::vector<std::string>& args, const std::string& input) {
    int pipefd[2] {};
    if (::pipe(pipefd) < 0) {
        throw SysError("pipe clipboard stdin");
    }

    Fd read_end(pipefd[0]);
    Fd write_end(pipefd[1]);

    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    posix_spawn_file_actions_t actions {};
    if (posix_spawn_file_actions_init(&actions) != 0) {
        throw SysError("posix_spawn_file_actions_init");
    }

    posix_spawn_file_actions_adddup2(&actions, read_end.get(), STDIN_FILENO);
    posix_spawn_file_actions_addclose(&actions, write_end.get());

    pid_t pid = -1;
    const int rc = posix_spawnp(&pid, argv[0], &actions, nullptr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    read_end.reset();

    if (rc != 0) {
        errno = rc;
        throw SysError("posix_spawnp clipboard backend");
    }

    std::size_t written = 0;
    while (written < input.size()) {
        const ssize_t n = ::write(write_end.get(), input.data() + written, input.size() - written);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw SysError("write clipboard stdin");
        }
        written += static_cast<std::size_t>(n);
    }
    write_end.reset();

    int status = 0;
    while (::waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }
        throw SysError("waitpid clipboard backend");
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        throw std::runtime_error("clipboard backend failed");
    }
}

} // namespace ahk
