#pragma once

#include <string>
#include <vector>

namespace ahk {

class Clipboard {
public:
    void set_text(const std::string& text) const;

private:
    static bool has_env(const char* name);
    static bool command_exists(const char* name);
    static void run_with_stdin(const std::vector<std::string>& args, const std::string& input);
};

} // namespace ahk
