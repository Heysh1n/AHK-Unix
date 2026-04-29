#include "ahkunix/RingBuffer.hpp"

namespace ahk {

RingBuffer::RingBuffer(std::size_t capacity) : capacity_(capacity) {}

void RingBuffer::push(int key_code) {
    if (keys_.size() == capacity_) {
        keys_.pop_front();
    }
    keys_.push_back(key_code);
}

bool RingBuffer::ends_with(const std::vector<int>& sequence) const {
    if (sequence.size() > keys_.size()) {
        return false;
    }

    const auto offset = keys_.size() - sequence.size();
    for (std::size_t i = 0; i < sequence.size(); ++i) {
        if (keys_[offset + i] != sequence[i]) {
            return false;
        }
    }
    return true;
}

void RingBuffer::clear() {
    keys_.clear();
}

} // namespace ahk
