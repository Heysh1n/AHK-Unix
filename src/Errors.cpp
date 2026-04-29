#include "ahkunix/Errors.hpp"

#include <cerrno>
#include <cstring>

namespace ahk {

SysError::SysError(const std::string& msg)
    : std::runtime_error(msg + ": " + std::strerror(errno)) {}

} // namespace ahk
