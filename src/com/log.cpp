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

#include "log.h"

#include "error.h"
#include "thread.h"
#include "time.h"

#include <algorithm>
#include <array>
#include <cstdio>

namespace
{
std::atomic_int global_call_counter = 0;
SpinLock* global_lock = nullptr;

// В момент закрытия главного окна разные потоки могут что-нибудь записывать
// в лог, поэтому с этой переменной нужна последовательная работа.
// Использование единственной блокировки означает, что и сообщения будут
// последовательные, но это не является проблемой.
std::function<void(LogEvent&&)>* global_log_events = nullptr;
}

void log_init()
{
        if (++global_call_counter != 1)
        {
                error_fatal("Error log init");
        }
        global_lock = new SpinLock;
}

void log_exit()
{
        delete global_lock;
        global_lock = nullptr;
        --global_call_counter;
}

//

void set_log_events(const std::function<void(LogEvent&&)>& events)
{
        std::lock_guard lg(*global_lock);

        if (events)
        {
                global_log_events = new std::function<void(LogEvent &&)>(events);
        }
        else
        {
                delete global_log_events;
                global_log_events = nullptr;
        }
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

namespace
{
void log(const std::string& msg, LogMessageType type) noexcept
{
        try
        {
                try
                {
                        {
                                std::lock_guard lg(*global_lock);

                                if (global_log_events)
                                {
                                        (*global_log_events)(LogEvent::Message(msg, type));
                                        return;
                                }
                        }

                        // Здесь переменная global_log_events теоретически уже может быть
                        // установлена в не nullptr, но по имеющейся логике программы установка
                        // этой переменной в не nullptr происходит только в начале программы
                        // и только один раз. Зато так будет больше параллельности в сообщениях
                        // с установленной в nullptr переменной global_log_events.

                        write_formatted_log_messages_to_stderr(format_log_message(msg));
                }
                catch (const std::exception& e)
                {
                        error_fatal(std::string("error log write message: ") + e.what());
                }
        }
        catch (...)
        {
                error_fatal("error log write message");
        }
}
}

void LOG(const std::string& msg) noexcept
{
        log(msg, LogMessageType::Normal);
}

void LOG_ERROR(const std::string& msg) noexcept
{
        log(msg, LogMessageType::Error);
}

void LOG_WARNING(const std::string& msg) noexcept
{
        log(msg, LogMessageType::Warning);
}

void LOG_INFORMATION(const std::string& msg) noexcept
{
        log(msg, LogMessageType::Information);
}
