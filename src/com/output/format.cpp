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

#include "format.h"

#include "../error.h"
#include "../time.h"

#include <array>
#include <cstdio>
#include <iostream>

std::string format_log_message(const std::string& msg) noexcept
{
        try
        {
                try
                {
                        constexpr int BUFFER_SIZE = 100;
                        std::array<char, BUFFER_SIZE> buffer;
                        int char_count = std::snprintf(buffer.data(), buffer.size(), "[%011.6f]: ", time_in_seconds());
                        if (char_count < 0 || static_cast<size_t>(char_count) >= buffer.size())
                        {
                                error_fatal("message beginning length out of range");
                        }

                        const std::string_view msg_begin = buffer.data();

                        std::string result;
                        result.reserve(msg_begin.size() + msg.size());
                        result += msg_begin;
                        for (char c : msg)
                        {
                                result += c;
                                if (c == '\n')
                                {
                                        result += msg_begin;
                                }
                        }
                        return result;
                }
                catch (const std::exception& e)
                {
                        error_fatal(std::string("error format log message: ") + e.what());
                }
        }
        catch (...)
        {
                error_fatal("error format log message");
        }
}

void write_formatted_log_message(const std::string& msg) noexcept
{
        try
        {
                try
                {
                        std::cerr << msg << '\n';
                }
                catch (const std::exception& e)
                {
                        error_fatal(std::string("error writing log message to stderr: ") + e.what());
                }
        }
        catch (...)
        {
                error_fatal("error writing log message to stderr");
        }
}
