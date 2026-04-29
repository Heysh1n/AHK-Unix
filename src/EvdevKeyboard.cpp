#include "ahkunix/EvdevKeyboard.hpp"

#include "ahkunix/Errors.hpp"
#include "ahkunix/Fd.hpp"

#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>

#include <cerrno>
#include <stdexcept>

namespace ahk {

class EvdevKeyboard::Impl {
public:
    explicit Impl(const std::filesystem::path& path) {
        fd.reset(::open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC));
        if (fd.get() < 0) {
            throw SysError("open " + path.string());
        }

        const int rc = libevdev_new_from_fd(fd.get(), &dev);
        if (rc < 0) {
            errno = -rc;
            throw SysError("libevdev_new_from_fd");
        }

        if (!libevdev_has_event_type(dev, EV_KEY)) {
            throw std::runtime_error(path.string() + " is not a keyboard-like evdev device");
        }
    }

    ~Impl() {
        ungrab();
        if (dev) {
            libevdev_free(dev);
        }
    }

    void grab() {
        if (grabbed) {
            return;
        }
        if (ioctl(fd.get(), EVIOCGRAB, 1) < 0) {
            throw SysError("EVIOCGRAB");
        }
        grabbed = true;
    }

    void ungrab() noexcept {
        if (grabbed) {
            ioctl(fd.get(), EVIOCGRAB, 0);
            grabbed = false;
        }
    }

    Fd fd;
    libevdev* dev = nullptr;
    bool grabbed = false;
};

EvdevKeyboard::EvdevKeyboard(const std::filesystem::path& path)
    : impl_(new Impl(path)) {}

EvdevKeyboard::~EvdevKeyboard() {
    delete impl_;
}

const char* EvdevKeyboard::name() const {
    const char* n = libevdev_get_name(impl_->dev);
    return n ? n : "unknown";
}

int EvdevKeyboard::fd() const {
    return impl_->fd.get();
}

void EvdevKeyboard::grab() {
    impl_->grab();
}

void EvdevKeyboard::ungrab() noexcept {
    impl_->ungrab();
}

std::optional<RawEvent> EvdevKeyboard::read_event() {
    input_event ev {};
    const int rc = libevdev_next_event(impl_->dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
    if (rc == -EAGAIN) {
        return std::nullopt;
    }
    if (rc < 0) {
        errno = -rc;
        throw SysError("libevdev_next_event");
    }
    return RawEvent{ev.type, ev.code, ev.value};
}

} // namespace ahk
