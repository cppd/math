/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>

namespace ns
{
namespace
{
constexpr const char* LOG_DIRECTORY_NAME = "log";

class Format final
{
        static constexpr unsigned MAX_THREADS = 1'000'000;
        static constexpr unsigned THREADS_WIDTH = 6;

        std::chrono::steady_clock::time_point start_time_ = std::chrono::steady_clock::now();

        std::unordered_map<std::thread::id, unsigned> map_;
        unsigned width_ = THREADS_WIDTH;
        std::ostringstream oss_line_beginning_;
        std::string line_beginning_;
        std::string result_;

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

public:
        Format()
        {
                oss_line_beginning_ << std::fixed;
                oss_line_beginning_ << std::setfill('0');
                oss_line_beginning_ << std::right;
                oss_line_beginning_ << std::setprecision(6);
        }

        std::string format(const std::string_view text, const std::string_view description) noexcept
        {
                const double time =
                        std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time_).count();

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
                        for (const char c : description)
                        {
                                line_beginning_ += std::isalpha(static_cast<unsigned char>(c)) ? c : ' ';
                        }
                        line_beginning_ += "): ";
                }

                result_.clear();
                result_.reserve(line_beginning_.size() + text.size() + 1);
                result_ = line_beginning_;
                for (const char c : text)
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
};

template <typename T>
        requires (std::is_same_v<T, std::filesystem::path>)
std::string to_filename(const T& path)
{
        const std::u8string s = path.generic_u8string();
        return {reinterpret_cast<const char*>(s.data()), s.size()};
}

std::filesystem::path to_path(const char* const filename)
{
        return reinterpret_cast<const char8_t*>(filename);
}

void create_directory(const std::filesystem::path& directory, Format& format)
{
        try
        {
                std::filesystem::create_directory(directory);
                std::filesystem::permissions(directory, std::filesystem::perms::owner_all);
        }
        catch (const std::exception& e)
        {
                std::cerr << format.format(std::string("Failed to create log directory: ") + e.what(), "fatal error");
                std::abort();
        }
}

std::filesystem::path create_directory(Format& format)
{
        std::filesystem::path directory = std::filesystem::temp_directory_path();

        directory /= to_path(settings::APPLICATION_NAME);
        create_directory(directory, format);

        directory /= to_path(LOG_DIRECTORY_NAME);
        create_directory(directory, format);

        return directory;
}

std::string log_file_name()
{
        const std::chrono::duration<double> duration = std::chrono::system_clock::now().time_since_epoch();
        std::ostringstream oss;
        oss << std::fixed;
        oss << duration.count();
        return oss.str();
}

std::ofstream create_file(const std::filesystem::path& directory, Format& format)
{
        const std::filesystem::path path = directory / to_path(log_file_name().c_str());

        std::ofstream file(path);
        if (!file)
        {
                std::cerr << format.format("Failed to create log file \"" + to_filename(path) + "\"", "fatal error");
                std::abort();
        }

        std::filesystem::permissions(path, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
        file << std::unitbuf;
        return file;
}

class Log final
{
        std::mutex lock_;
        Format format_;
        std::ofstream file_{create_file(create_directory(format_), format_)};

        std::string write(const std::string_view text, const std::string_view description) noexcept
        {
                std::string res = format_.format(text, description);
                std::cerr << res;
                file_ << res;
                res.pop_back();
                return res;
        }

public:
        Log() noexcept
        try
        {
        }
        catch (const std::exception& e)
        {
                std::cerr << "Failed to initialize log file: " << e.what() << '\n';
                std::abort();
        }
        catch (...)
        {
                std::cerr << "Failed to initialize log file, unknown error\n";
                std::abort();
        }

        std::string write_log(const std::string_view text, const std::string_view description) noexcept
        {
                const std::lock_guard lg(lock_);
                return write(text, description);
        }

        [[noreturn]] void write_log_fatal_error_and_exit(const char* const text) noexcept
        {
                const std::lock_guard lg(lock_);
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

std::string write_log(const std::string_view text, const std::string_view description) noexcept
{
        return log().write_log(text, description);
}

void write_log_fatal_error_and_exit(const char* const text) noexcept
{
        log().write_log_fatal_error_and_exit(text);
}
}
