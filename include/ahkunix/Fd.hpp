#pragma once

namespace ahk {

class Fd {
public:
    Fd() = default;
    explicit Fd(int fd);

    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;

    Fd(Fd&& other) noexcept;
    Fd& operator=(Fd&& other) noexcept;

    ~Fd();

    int get() const;
    void reset(int new_fd = -1);

private:
    int fd_ = -1;
};

} // namespace ahk
