/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>

namespace
{
std::mutex global_lock;

#if defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif
const std::chrono::steady_clock::time_point START_TIME = std::chrono::steady_clock::now();
#if defined(__clang__)
#pragma GCC diagnostic pop
#endif

std::string format(const std::string_view& text, const std::string_view& description, double time) noexcept
{
        std::string line_beginning;

        if (description.empty())
        {
                constexpr int BUFFER_SIZE = 100;
                std::array<char, BUFFER_SIZE> buffer;
                std::snprintf(buffer.data(), buffer.size(), "[%011.6f]: ", time);
                line_beginning = buffer.data();
        }
        else
        {
                constexpr int BUFFER_SIZE = 100;
                std::array<char, BUFFER_SIZE> buffer;
                std::snprintf(buffer.data(), buffer.size(), "[%011.6f](", time);
                line_beginning = buffer.data();
                for (char c : description)
                {
                        line_beginning += std::isalpha(static_cast<unsigned char>(c)) ? c : ' ';
                }
                line_beginning += "): ";
        }

        std::string result;
        result.reserve(line_beginning.size() + text.size() + 1);
        result += line_beginning;
        for (char c : text)
        {
                result += c;
                if (c == '\n')
                {
                        result += line_beginning;
                }
        }
        result += '\n';
        return result;
}

void write(const std::string_view& text) noexcept
{
        static std::ofstream file = []() {
                std::string name = std::string(settings::APPLICATION_NAME) + " log.txt";
                std::filesystem::path path = std::filesystem::temp_directory_path() / name;
                std::ofstream f(path);
                f << std::unitbuf;
                return f;
        }();
        std::cerr << text;
        file << text;
}

std::string write(const std::string_view& text, const std::string_view& description) noexcept
{
        double time = std::chrono::duration<double>(std::chrono::steady_clock::now() - START_TIME).count();
        std::string result = format(text, description, time);
        write(result);
        result.pop_back();
        return result;
}
}

std::string write_log(const std::string_view& text, const std::string_view& description) noexcept
{
        std::lock_guard lg(global_lock);
        return write(text, description);
}

void write_log_fatal_error_and_exit(const char* text) noexcept
{
        std::lock_guard lg(global_lock);
        write(text, "fatal error");
        std::_Exit(EXIT_FAILURE);
}
