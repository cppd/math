/*
Copyright (C) 2017 Topological Manifold

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

#include "log.h"

#include "time.h"

#include <algorithm>
#include <cstdio>

namespace LogImplementation
{
void write_log_message(const std::string& msg) noexcept try
{
        constexpr int BUF_SIZE = 100;

        char msg_begin[BUF_SIZE];

        int msg_begin_length = std::snprintf(msg_begin, BUF_SIZE, "[%011.6f]: ", get_time_seconds());

        if (msg_begin_length < 0 || msg_begin_length >= BUF_SIZE)
        {
                throw "";
        }

        int new_line_count = std::count(msg.begin(), msg.end(), '\n');
        unsigned message_size = msg_begin_length + msg.size() + new_line_count * msg_begin_length;

        std::string message;

        message.reserve(message_size);

        if (new_line_count == 0)
        {
                message += msg_begin;
                message += msg;
        }
        else
        {
                message += msg_begin;
                for (char c : msg)
                {
                        if (c != '\n')
                        {
                                message += c;
                        }
                        else
                        {
                                message += '\n';
                                message += msg_begin;
                        }
                }
        }

        // Вывод текста одним вызовом функции для работы при многопоточности
        std::fprintf(stderr, "%s\n", message.c_str());
        std::fflush(stderr);
}
catch (...)
{
        try
        {
                std::fprintf(stderr, "error log write\n");
                std::fflush(stderr);
        }
        catch (...)
        {
        }

        std::_Exit(EXIT_FAILURE);
}
}
