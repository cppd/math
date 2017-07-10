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

#include "error.h"
#include "time.h"

#include <algorithm>
#include <cstdio>

namespace
{
// Мьютекс для работы с этим указателем не нужен, так как установка указателя
// должна делаться один раз для одного окна в самом начале программы
ILogCallback* global_log_callback = nullptr;

void write_log_message(const std::string& msg) noexcept
{
        try
        {
                constexpr int BUF_SIZE = 100;
                char buffer[BUF_SIZE];
                int char_count = std::snprintf(buffer, BUF_SIZE, "[%011.6f]: ", get_time_seconds());
                if (char_count < 0 || char_count >= BUF_SIZE)
                {
                        error("message begin length out of range");
                }

                std::string msg_begin = buffer;

                unsigned new_line_count = std::count(msg.begin(), msg.end(), '\n');
                unsigned message_size = msg_begin.length() + msg.size() + new_line_count * msg_begin.length();

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

                if (global_log_callback)
                {
                        global_log_callback->log(message);
                }
        }
        catch (std::exception& e)
        {
                error_fatal(std::string("error log write message: ") + e.what());
        }
        catch (...)
        {
                error_fatal("error log write message");
        }
}
}

void set_log_callback(ILogCallback* callback)
{
        global_log_callback = callback;
}

void LOG(const std::string& msg) noexcept
{
        write_log_message(msg);
}
