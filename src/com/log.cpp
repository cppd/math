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
}

void set_log_callback(ILogCallback* callback) noexcept
{
        global_log_callback = callback;
}

std::vector<std::string> format_log_message(const std::string& msg) noexcept
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
                std::vector<std::string> res;

                if (std::count(msg.begin(), msg.end(), '\n') == 0)
                {
                        res.push_back(msg_begin + msg);
                        return res;
                }

                std::string message = msg_begin;

                for (char c : msg)
                {
                        if (c != '\n')
                        {
                                message += c;
                        }
                        else
                        {
                                res.push_back(message);
                                message = msg_begin;
                        }
                }

                res.push_back(message);

                return res;
        }
        catch (std::exception& e)
        {
                error_fatal(std::string("error format log message: ") + e.what());
        }
        catch (...)
        {
                error_fatal("error format log message");
        }
}

void write_formatted_log_messages_to_stderr(const std::vector<std::string>& lines)
{
        // Вывод всех строк одним вызовом функции std::fprintf для работы при многопоточности
        std::string m;
        for (const std::string& s : lines)
        {
                m += s + "\n";
        }
        std::fprintf(stderr, "%s", m.c_str());
        std::fflush(stderr);
}

void LOG(const std::string& msg) noexcept
{
        try
        {
                if (global_log_callback)
                {
                        global_log_callback->log(msg);
                }
                else
                {
                        write_formatted_log_messages_to_stderr(format_log_message(msg));
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
