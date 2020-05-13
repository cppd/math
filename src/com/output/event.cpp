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

#include "event.h"

#include "format.h"

#include "../error.h"

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
                                write_formatted_log_text(format_log_text(msg));
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
                                write_formatted_log_text(format_log_text(msg));
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
