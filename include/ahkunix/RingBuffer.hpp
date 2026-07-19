#pragma once

#include <cstddef>
#include <deque>
#include <vector>

namespace ahk {

class RingBuffer {
public:
    explicit RingBuffer(std::size_t capacity);

    void push(int key_code);
    bool ends_with(const std::vector<int>& sequence) const;
    void clear();

private:
    std::size_t capacity_ = 0;
    std::deque<int> keys_;
};

} // namespace ahk
