#pragma once

#include "ahkunix/commands/Command.hpp"
#include <string>
#include <vector>

namespace ahk::cmd
{
    class SendInputCommand : public Command
    {
    public:
        explicit SendInputCommand(std::string sequence);

        void execute(UinputKeyboard &injector, Clipboard &clipboard) const override;
        void execute_interruptible(
            UinputKeyboard &injector,
            Clipboard &clipboard,
            const std::atomic<bool> &stop_requested) const override;
        std::string describe() const override;

<<<<<<< HEAD
=======
        void bind_context(const std::shared_ptr<Context> &ctx) override;

>>>>>>> master
    private:
        std::string sequence_;

        struct Part
        {
            bool is_special_key = false;
            int key_code = 0;
            int repeat_count = 1;
            std::string text;
        };
        std::vector<Part> parts_;

        void parse_sequence();
<<<<<<< HEAD
=======

        mutable std::shared_ptr<Context> context_;
>>>>>>> master
    };
} // namespace ahk::cmd
