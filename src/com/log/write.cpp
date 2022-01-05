/*
Copyright (C) 2017-2022 Topological Manifold

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "write.h"

#include <src/settings/name.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>

namespace ns
{
namespace
{
class Log final
{
        static constexpr unsigned MAX_THREADS = 1'000'000;
        static constexpr unsigned THREADS_WIDTH = 6;

        std::chrono::steady_clock::time_point start_time_ = std::chrono::steady_clock::now();

        std::mutex lock_;
        std::unordered_map<std::thread::id, unsigned> map_;
        unsigned width_ = THREADS_WIDTH;
        std::ostringstream oss_line_beginning_;
        std::string line_beginning_;
        std::string result_;

        std::ofstream file_;

        void write_thread_id(const std::thread::id& thread_id) noexcept
        {
                auto iter = map_.find(thread_id);
                if (iter == map_.cend())
                {
                        if (map_.size() < MAX_THREADS)
                        {
                                iter = map_.emplace(thread_id, map_.size()).first;
                        }
                        else
                        {
                                width_ = 18;
                                oss_line_beginning_ << "0x" << std::hex << std::setw(16) << thread_id;
                                return;
                        }
                }
                oss_line_beginning_ << std::setw(width_) << iter->second;
        }

        std::string format(
                const std::string_view& text,
                const std::string_view& description,
                const double time) noexcept
        {
                oss_line_beginning_.str(std::string());
                oss_line_beginning_ << "[" << std::setw(11) << time << "][";
                write_thread_id(std::this_thread::get_id());
                oss_line_beginning_ << "]";

                line_beginning_ = oss_line_beginning_.str();
                if (description.empty())
                {
                        line_beginning_ += ": ";
                }
                else
                {
                        line_beginning_ += "(";
                        for (char c : description)
                        {
                                line_beginning_ += std::isalpha(static_cast<unsigned char>(c)) ? c : ' ';
                        }
                        line_beginning_ += "): ";
                }

                result_.clear();
                result_.reserve(line_beginning_.size() + text.size() + 1);
                result_ = line_beginning_;
                for (char c : text)
                {
                        result_ += c;
                        if (c == '\n')
                        {
                                result_ += line_beginning_;
                        }
                }
                result_ += '\n';
                return result_;
        }

        void write(const std::string_view& text) noexcept
        {
                std::cerr << text;
                file_ << text;
        }

        std::string write(const std::string_view& text, const std::string_view& description) noexcept
        {
                const double time =
                        std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time_).count();
                std::string result = format(text, description, time);
                write(result);
                result.pop_back();
                return result;
        }

public:
        Log()
        {
                oss_line_beginning_ << std::fixed;
                oss_line_beginning_ << std::setfill('0');
                oss_line_beginning_ << std::right;
                oss_line_beginning_ << std::setprecision(6);

                std::string directory_name = std::string(settings::APPLICATION_NAME) + " Log";
                std::filesystem::path directory = std::filesystem::temp_directory_path() / directory_name;
                std::filesystem::create_directory(directory);
                std::filesystem::permissions(directory, std::filesystem::perms::owner_all);

                std::chrono::duration<double> duration = std::chrono::system_clock::now().time_since_epoch();
                std::ostringstream name;
                name << std::fixed;
                name << duration.count();

                std::filesystem::path file = directory / name.str();
                file_ = std::ofstream(file);
                std::filesystem::permissions(
                        file, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
                file_ << std::unitbuf;
        }

        std::string write_log(const std::string_view& text, const std::string_view& description) noexcept
        {
                std::lock_guard lg(lock_);
                return write(text, description);
        }

        [[noreturn]] void write_log_fatal_error_and_exit(const char* const text) noexcept
        {
                std::lock_guard lg(lock_);
                write(text, "fatal error");
                std::abort();
        }
};

Log& log()
{
        static Log log;
        return log;
}
}

std::string write_log(const std::string_view& text, const std::string_view& description) noexcept
{
        return log().write_log(text, description);
}

void write_log_fatal_error_and_exit(const char* const text) noexcept
{
        log().write_log_fatal_error_and_exit(text);
}
}
