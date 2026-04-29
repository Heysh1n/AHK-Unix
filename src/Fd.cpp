#include "ahkunix/Fd.hpp"

#include <unistd.h>

#include <utility>

namespace ahk {

Fd::Fd(int fd) : fd_(fd) {}

Fd::Fd(Fd&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}

Fd& Fd::operator=(Fd&& other) noexcept {
    if (this != &other) {
        reset();
        fd_ = std::exchange(other.fd_, -1);
    }
    return *this;
}

Fd::~Fd() {
    reset();
}

int Fd::get() const {
    return fd_;
}

void Fd::reset(int new_fd) {
    if (fd_ >= 0) {
        ::close(fd_);
    }
    fd_ = new_fd;
}

} // namespace ahk
