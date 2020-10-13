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

#include "../time.h"

#include <array>
#include <cstdio>
#include <iostream>
#include <mutex>

namespace
{
std::mutex global_lock;

std::string format(const std::string& text, const std::string_view& description) noexcept
{
        std::string line_beginning;

        if (description.empty())
        {
                constexpr int BUFFER_SIZE = 100;
                std::array<char, BUFFER_SIZE> buffer;
                std::snprintf(buffer.data(), buffer.size(), "[%011.6f]: ", time_in_seconds());
                line_beginning = buffer.data();
        }
        else
        {
                constexpr int BUFFER_SIZE = 100;
                std::array<char, BUFFER_SIZE> buffer;
                std::snprintf(buffer.data(), buffer.size(), "[%011.6f](", time_in_seconds());
                line_beginning = buffer.data();
                for (char c : description)
                {
                        line_beginning += std::isalpha(static_cast<unsigned char>(c)) ? c : ' ';
                }
                line_beginning += "): ";
        }

        std::string result;
        result.reserve(line_beginning.size() + text.size());
        result += line_beginning;
        for (char c : text)
        {
                result += c;
                if (c == '\n')
                {
                        result += line_beginning;
                }
        }
        return result;
}

void write(const std::string& text) noexcept
{
        std::cerr << text;
}
}

std::string write_log(const std::string& text, const std::string_view& description) noexcept
{
        std::lock_guard lg(global_lock);
        try
        {
                std::string result = format(text, description);
                result += '\n';
                write(result);
                result.pop_back();
                return result;
        }
        catch (const std::exception& e)
        {
                write(std::string("Error writing to log: ").append(e.what()).append("\n"));
                return format(text, description);
        }
        catch (...)
        {
                write("Error writing to log\n");
                return format(text, description);
        }
}
