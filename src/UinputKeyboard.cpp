#include "ahkunix/UinputKeyboard.hpp"

#include "ahkunix/Errors.hpp"
#include "ahkunix/Fd.hpp"

#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <thread>

namespace ahk {

class UinputKeyboard::Impl {
public:
    Impl() {
        fd.reset(::open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC));
        if (fd.get() < 0) {
            throw SysError("open /dev/uinput");
        }

        enable(EV_KEY);
        enable(EV_SYN);

        for (int key = 0; key <= KEY_MAX; ++key) {
            ioctl_or_throw(UI_SET_KEYBIT, key, "UI_SET_KEYBIT");
        }

        uinput_setup setup {};
        setup.id.bustype = BUS_USB;
        setup.id.vendor = 0x1209;
        setup.id.product = 0xA11C;
        std::strncpy(setup.name, "AHKUnix virtual keyboard", UINPUT_MAX_NAME_SIZE - 1);

        if (ioctl(fd.get(), UI_DEV_SETUP, &setup) < 0) {
            throw SysError("UI_DEV_SETUP");
        }
        if (ioctl(fd.get(), UI_DEV_CREATE) < 0) {
            throw SysError("UI_DEV_CREATE");
        }

        created = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    ~Impl() {
        if (created) {
            ioctl(fd.get(), UI_DEV_DESTROY);
        }
    }

    void emit(int type, int code, int value) {
        input_event ev {};
        ev.type = type;
        ev.code = code;
        ev.value = value;
        if (::write(fd.get(), &ev, sizeof(ev)) != sizeof(ev)) {
            throw SysError("write uinput event");
        }
    }

    void sync() {
        emit(EV_SYN, SYN_REPORT, 0);
    }

    Fd fd;
    bool created = false;

private:
    void enable(int event_type) {
        ioctl_or_throw(UI_SET_EVBIT, event_type, "UI_SET_EVBIT");
    }

    void ioctl_or_throw(unsigned long request, int value, const char* label) {
        if (ioctl(fd.get(), request, value) < 0) {
            throw SysError(label);
        }
    }
};

UinputKeyboard::UinputKeyboard() : impl_(new Impl()) {}

UinputKeyboard::~UinputKeyboard() {
    delete impl_;
}

void UinputKeyboard::tap(int key_code) {
    impl_->emit(EV_KEY, key_code, 1);
    impl_->sync();
    impl_->emit(EV_KEY, key_code, 0);
    impl_->sync();
}

void UinputKeyboard::backspace(std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        tap(KEY_BACKSPACE);
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}

void UinputKeyboard::forward(const RawEvent& ev) {
    if (ev.type != EV_KEY) {
        return;
    }
    impl_->emit(EV_KEY, ev.code, ev.value);
    impl_->sync();
}

void UinputKeyboard::hold_combo_and_tap(const std::vector<int>& modifiers, int key_code) {
    for (int mod : modifiers) {
        impl_->emit(EV_KEY, mod, 1);
    }
    impl_->emit(EV_KEY, key_code, 1);
    impl_->sync();
    impl_->emit(EV_KEY, key_code, 0);
    for (auto it = modifiers.rbegin(); it != modifiers.rend(); ++it) {
        impl_->emit(EV_KEY, *it, 0);
    }
    impl_->sync();
}

} // namespace ahk
