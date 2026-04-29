#pragma once

#include <stdexcept>
#include <string>

namespace ahk {

class SysError : public std::runtime_error {
public:
    explicit SysError(const std::string& msg);
};

} // namespace ahk
