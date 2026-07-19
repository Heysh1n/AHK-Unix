#include "ahkunix/commands/IfCommand.hpp"
#include "ahkunix/Clipboard.hpp"
#include "ahkunix/UinputKeyboard.hpp"

#include <atomic>
#include <chrono>
#include <random>
#include <regex>
#include <stdexcept>
<<<<<<< HEAD
=======
#include <iomanip>
#include <limits>
#include <sstream>
#include <ctime>
>>>>>>> master

namespace ahk::cmd
{
    int Context::get_variable(const std::string &name) const
    {
        auto it = variables_.find(name);
        return (it != variables_.end()) ? it->second : 0;
    }

    void Context::set_variable(const std::string &name, int value)
    {
        variables_[name] = value;
    }

    int Context::random_range(int min_val, int max_val)
    {
        static std::mt19937 gen(
            static_cast<std::mt19937::result_type>(
                std::chrono::high_resolution_clock::now().time_since_epoch().count()));
        std::uniform_int_distribution<> dist(min_val, max_val);
        return dist(gen);
    }

<<<<<<< HEAD
=======
    void Context::set_variable_str(const std::string &name, const std::string &value)
    {
        variables_str_[name] = value;
    }

    std::string Context::get_variable_str(const std::string &name) const
    {
        // ── Built-in time variables ──────────────────────────────────
        // Standard AHK names + descriptive aliases (A_Hour_Now, etc.)
        static const std::vector<std::string> time_vars = {
            "A_Now", "A_NowUTC",
            "A_YYYY", "A_Year", "A_Year_Now",
            "A_MM", "A_Mon", "A_Month_Now",
            "A_DD", "A_MDay", "A_Day_Now",
            "A_WDay",
            "A_YDay",
            "A_Hour", "A_Hour_Now",
            "A_Min", "A_Minute_Now",
            "A_Sec", "A_Second_Now",
            "A_MSec",
            "A_TickCount",
        };

        bool is_time_var = false;
        for (const auto &tv : time_vars)
        {
            if (name == tv) { is_time_var = true; break; }
        }

        if (is_time_var)
        {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm local_tm{};
            localtime_r(&now_c, &local_tm);

            std::ostringstream ss;

            // Full timestamps
            if (name == "A_Now")
            {
                ss << std::put_time(&local_tm, "%Y%m%d%H%M%S");
            }
            else if (name == "A_NowUTC")
            {
                std::tm utc_tm{};
                gmtime_r(&now_c, &utc_tm);
                ss << std::put_time(&utc_tm, "%Y%m%d%H%M%S");
            }
            // Year (4-digit)
            else if (name == "A_YYYY" || name == "A_Year" || name == "A_Year_Now")
            {
                ss << std::put_time(&local_tm, "%Y");
            }
            // Month (01-12)
            else if (name == "A_MM" || name == "A_Mon" || name == "A_Month_Now")
            {
                ss << std::put_time(&local_tm, "%m");
            }
            // Day of month (01-31)
            else if (name == "A_DD" || name == "A_MDay" || name == "A_Day_Now")
            {
                ss << std::put_time(&local_tm, "%d");
            }
            // Day of week (1=Sun, 7=Sat — AHK convention)
            else if (name == "A_WDay")
            {
                ss << (local_tm.tm_wday + 1); // tm_wday: 0=Sun → AHK: 1=Sun
            }
            // Day of year (1-366)
            else if (name == "A_YDay")
            {
                ss << (local_tm.tm_yday + 1); // tm_yday is 0-based
            }
            // Hour (00-23)
            else if (name == "A_Hour" || name == "A_Hour_Now")
            {
                ss << std::put_time(&local_tm, "%H");
            }
            // Minute (00-59)
            else if (name == "A_Min" || name == "A_Minute_Now")
            {
                ss << std::put_time(&local_tm, "%M");
            }
            // Second (00-59)
            else if (name == "A_Sec" || name == "A_Second_Now")
            {
                ss << std::put_time(&local_tm, "%S");
            }
            // Milliseconds (000-999)
            else if (name == "A_MSec")
            {
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()) % 1000;
                ss << std::setfill('0') << std::setw(3) << ms.count();
            }
            // Tick count (milliseconds since epoch, like GetTickCount)
            else if (name == "A_TickCount")
            {
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch());
                ss << ms.count();
            }

            return ss.str();
        }

        if (auto it = variables_str_.find(name); it != variables_str_.end())
        {
            return it->second;
        }
        return "";
    }

    std::string Context::interpolate(const std::string &input) const
    {
        // Phase 1: %VarName% substitution
        std::string after_vars;
        {
            std::regex pattern("%([^%\\s]+)%");
            auto begin = std::sregex_iterator(input.begin(), input.end(), pattern);
            auto end = std::sregex_iterator();

            std::size_t last_pos = 0;
            for (auto it = begin; it != end; ++it)
            {
                std::smatch match = *it;
                after_vars += input.substr(last_pos, match.position() - last_pos);
                after_vars += get_variable_str(match[1].str());
                last_pos = match.position() + match.length();
            }
            after_vars += input.substr(last_pos);
        }

        // Phase 2: evaluate parenthesized math expressions — (number op number)
        // Supports +, -, *, / with integer arithmetic.  Uses long long to
        // prevent overflow; clamps the result to [INT_MIN, INT_MAX].
        // Gracefully handles negative results (e.g. 00 - 1 → -1).
        std::string result;
        {
            // Match: ( optional-whitespace integer optional-whitespace op optional-whitespace integer optional-whitespace )
            std::regex math_pattern(R"(\(\s*(-?\d+)\s*([+\-*/])\s*(-?\d+)\s*\))");
            std::string current = after_vars;

            // Iteratively evaluate innermost parenthesized expressions
            // (handles nested expressions up to a reasonable depth).
            constexpr int max_passes = 16;
            for (int pass = 0; pass < max_passes; ++pass)
            {
                std::string replaced;
                std::size_t last_pos = 0;
                bool found_any = false;

                auto begin = std::sregex_iterator(current.begin(), current.end(), math_pattern);
                auto end = std::sregex_iterator();

                for (auto it = begin; it != end; ++it)
                {
                    found_any = true;
                    std::smatch m = *it;
                    replaced += current.substr(last_pos, m.position() - last_pos);

                    long long a = 0, b = 0;
                    try { a = std::stoll(m[1].str()); }
                    catch (...) { a = 0; }
                    try { b = std::stoll(m[3].str()); }
                    catch (...) { b = 0; }

                    const std::string op = m[2].str();
                    long long r = 0;
                    if (op == "+") r = a + b;
                    else if (op == "-") r = a - b;
                    else if (op == "*") r = a * b;
                    else if (op == "/") r = (b != 0) ? (a / b) : 0; // safe div-by-zero

                    // Clamp to int range to prevent downstream issues
                    constexpr long long lo = static_cast<long long>(std::numeric_limits<int>::min());
                    constexpr long long hi = static_cast<long long>(std::numeric_limits<int>::max());
                    if (r < lo) r = lo;
                    if (r > hi) r = hi;

                    replaced += std::to_string(r);
                    last_pos = m.position() + m.length();
                }
                replaced += current.substr(last_pos);

                current = std::move(replaced);
                if (!found_any) break;
            }

            result = std::move(current);
        }

        return result;
    }

>>>>>>> master
    IfCommand::IfCommand(std::string condition, CommandList true_branch, CommandList false_branch)
        : condition_(std::move(condition)),
          true_branch_(std::move(true_branch)),
          false_branch_(std::move(false_branch))
    {
    }

    void IfCommand::set_context(std::shared_ptr<Context> ctx)
    {
        context_ = std::move(ctx);
        for (const auto &cmd : true_branch_)
        {
            cmd->bind_context(context_);
        }
        for (const auto &cmd : false_branch_)
        {
            cmd->bind_context(context_);
        }
    }

    void IfCommand::bind_context(const std::shared_ptr<Context> &ctx)
    {
        set_context(ctx);
    }

    bool IfCommand::evaluate_condition() const
    {
<<<<<<< HEAD
=======
        // Safe integer parser: clamps to int range instead of throwing
        // on overflow.  Returns 0 for completely unparseable strings.
        auto safe_stoi = [](const std::string &s) -> int {
            try {
                long long v = std::stoll(s);
                if (v > std::numeric_limits<int>::max()) return std::numeric_limits<int>::max();
                if (v < std::numeric_limits<int>::min()) return std::numeric_limits<int>::min();
                return static_cast<int>(v);
            } catch (...) {
                return 0;
            }
        };

>>>>>>> master
        std::smatch match;

        // random(1,3) = 2
        const std::regex random_pattern(
            R"(^\s*random\(\s*(-?\d+)\s*,\s*(-?\d+)\s*\)\s*(==|=|!=|>=|<=|>|<)\s*(-?\d+)\s*$)",
            std::regex::icase);

        if (std::regex_match(condition_, match, random_pattern))
        {
<<<<<<< HEAD
            int min_val = std::stoi(match[1].str());
            int max_val = std::stoi(match[2].str());
            std::string op = match[3].str();
            int cmp = std::stoi(match[4].str());
=======
            int min_val = safe_stoi(match[1].str());
            int max_val = safe_stoi(match[2].str());
            std::string op = match[3].str();
            int cmp = safe_stoi(match[4].str());
>>>>>>> master

            if (min_val > max_val)
            {
                std::swap(min_val, max_val);
            }

            int value = Context::random_range(min_val, max_val);

            if (op == "=" || op == "==") return value == cmp;
            if (op == "!=") return value != cmp;
            if (op == ">") return value > cmp;
            if (op == "<") return value < cmp;
            if (op == ">=") return value >= cmp;
            if (op == "<=") return value <= cmp;
            return false;
        }

        // variable comparison: announceType = 1
        const std::regex var_pattern(
            R"(^\s*([A-Za-z_]\w*)\s*(==|=|!=|>=|<=|>|<)\s*(-?\d+)\s*$)");

        if (std::regex_match(condition_, match, var_pattern))
        {
            const std::string var_name = match[1].str();
            const std::string op = match[2].str();
<<<<<<< HEAD
            const int cmp = std::stoi(match[3].str());
=======
            const int cmp = safe_stoi(match[3].str());
>>>>>>> master

            if (!context_)
            {
                context_ = std::make_shared<Context>();
            }

            int value = context_->get_variable(var_name);

            if (op == "=" || op == "==") return value == cmp;
            if (op == "!=") return value != cmp;
            if (op == ">") return value > cmp;
            if (op == "<") return value < cmp;
            if (op == ">=") return value >= cmp;
            if (op == "<=") return value <= cmp;
            return false;
        }

        throw std::runtime_error("cannot evaluate condition: " + condition_);
    }

    void IfCommand::execute(UinputKeyboard &injector, Clipboard &clipboard) const
    {
        if (!context_)
        {
            context_ = std::make_shared<Context>();
        }

        const bool ok = evaluate_condition();
        const auto &branch = ok ? true_branch_ : false_branch_;

        for (const auto &cmd : branch)
        {
            cmd->bind_context(context_);
            cmd->execute(injector, clipboard);
        }
    }

    void IfCommand::execute_interruptible(
        UinputKeyboard &injector,
        Clipboard &clipboard,
        const std::atomic<bool> &stop_requested) const
    {
        if (stop_requested.load(std::memory_order_acquire))
        {
            return;
        }

        if (!context_)
        {
            context_ = std::make_shared<Context>();
        }

        const bool ok = evaluate_condition();
        const auto &branch = ok ? true_branch_ : false_branch_;

        for (const auto &cmd : branch)
        {
            if (stop_requested.load(std::memory_order_acquire))
            {
                return;
            }
            cmd->bind_context(context_);
            cmd->execute_interruptible(injector, clipboard, stop_requested);
        }
    }

    std::string IfCommand::describe() const
    {
        return "If (" + condition_ + ")";
    }
} // namespace ahk::cmd
