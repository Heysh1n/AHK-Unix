#include "ahkunix/commands/ShowMarkdownCommand.hpp"

#include <cstdlib>
#include <string>

namespace ahk::cmd
{
    ShowMarkdownCommand::ShowMarkdownCommand(std::string file_path)
        : file_path_(std::move(file_path))
    {
    }

    void ShowMarkdownCommand::execute(UinputKeyboard &injector, Clipboard &clipboard) const
    {
        (void)injector;
        (void)clipboard;

        std::string cmd = "pkill -f md_overlay.py 2>/dev/null; "
                          "python3 /home/heysh1n/scripts/md_overlay.py \""
                          + file_path_ + "\" > /dev/null 2>&1 &";
        std::system(cmd.c_str());
    }

    std::string ShowMarkdownCommand::describe() const
    {
        return "ShowMarkdown: " + file_path_;
    }
} // namespace ahk::cmd
