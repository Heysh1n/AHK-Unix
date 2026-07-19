#include "ahkunix/commands/IncrementCommand.hpp"
#include "ahkunix/commands/IfCommand.hpp" // Context definition
#include "ahkunix/Clipboard.hpp"
#include "ahkunix/UinputKeyboard.hpp"

#include <chrono>
#include <fstream>
#include <random>
#include <string>

namespace ahk::cmd
{
    IncrementCommand::IncrementCommand(std::string var_name)
        : var_name_(std::move(var_name))
    {
    }

    void IncrementCommand::bind_context(const std::shared_ptr<Context> &ctx)
    {
        context_ = ctx;
    }

    void IncrementCommand::execute(UinputKeyboard & /*injector*/, Clipboard & /*clipboard*/) const
    {
        if (!context_)
        {
            context_ = std::make_shared<Context>();
        }

        const std::string file_path = "/tmp/ahkunix_var_" + var_name_ + ".txt";

        int value = 0;
        bool read_ok = false;

        {
            std::ifstream in(file_path);
            if (in && (in >> value))
            {
                read_ok = true;
            }
        }

        if (read_ok)
        {
            value += 1;
        }
        else
        {
            // Generate a random value from 0 to 50 on first use
            static std::mt19937 gen(
                static_cast<std::mt19937::result_type>(
                    std::chrono::high_resolution_clock::now().time_since_epoch().count()));
            std::uniform_int_distribution<> dist(0, 50);
            value = dist(gen);
        }

        {
            std::ofstream out(file_path, std::ios::trunc);
            out << value;
        }

        context_->set_variable_str(var_name_, std::to_string(value));
    }

    std::string IncrementCommand::describe() const
    {
        return "Increment " + var_name_;
    }
} // namespace ahk::cmd
