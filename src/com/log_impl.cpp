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

#include "log_impl.h"

#include "error.h"
#include "time.h"

#include <algorithm>
#include <array>
#include <cstdio>

namespace
{
// Установка этих переменных происходит в основном потоке при отсутствии
// работы других потоков, поэтому с ними можно работать без блокировок.
const std::function<void(LogEvent&&)>* global_log_events = nullptr;
const std::function<void(MessageEvent&&)>* global_message_events = nullptr;
}

void set_log_events(const std::function<void(LogEvent&&)>* events)
{
        global_log_events = events;
}

void set_message_events(const std::function<void(MessageEvent&&)>* events)
{
        global_message_events = events;
}

std::vector<std::string> format_log_message(const std::string& msg) noexcept
{
        try
        {
                try
                {
                        constexpr int BUF_SIZE = 100;
                        std::array<char, BUF_SIZE> buffer;
                        int char_count = std::snprintf(buffer.data(), BUF_SIZE, "[%011.6f]: ", time_in_seconds());
                        if (char_count < 0 || char_count >= BUF_SIZE)
                        {
                                error("message begin length out of range");
                        }

                        std::string msg_begin = buffer.data();
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

void write_formatted_log_messages_to_stderr(const std::vector<std::string>& lines) noexcept
{
        try
        {
                try
                {
                        // Вывод всех строк одним вызовом функции std::fprintf для работы при многопоточности
                        std::string s;
                        for (const std::string& line : lines)
                        {
                                s += line;
                                s += '\n';
                        }
                        std::fprintf(stderr, "%s", s.c_str());
                        std::fflush(stderr);
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

void log_impl(const std::string& msg, LogEvent::Type type) noexcept
{
        try
        {
                try
                {
                        if (global_log_events)
                        {
                                (*global_log_events)(LogEvent(msg, type));
                        }
                        else
                        {
                                write_formatted_log_messages_to_stderr(format_log_message(msg));
                        }
                }
                catch (const std::exception& e)
                {
                        error_fatal(std::string("error writing log message: ") + e.what());
                }
        }
        catch (...)
        {
                error_fatal("error writing log message");
        }
}

void message_impl(const std::string& msg, MessageEvent::Type type) noexcept
{
        try
        {
                try
                {
                        if (global_message_events)
                        {
                                (*global_message_events)(MessageEvent(msg, type));
                        }
                        else
                        {
                                write_formatted_log_messages_to_stderr(format_log_message(msg));
                        }
                }
                catch (const std::exception& e)
                {
                        error_fatal(std::string("error writing message: ") + e.what());
                }
        }
        catch (...)
        {
                error_fatal("error writing message");
        }
}
