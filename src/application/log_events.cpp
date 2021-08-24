/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "log_events.h"

#include "log.h"

#include <atomic>

namespace ns::application
{
namespace
{
LogEvents* g_log_events = nullptr;

LogType message_type_to_log_type(MessageType type)
{
        switch (type)
        {
        case MessageType::ERROR:
        case MessageType::ERROR_FATAL:
        {
                return LogType::ERROR;
        }
        case MessageType::INFORMATION:
        {
                return LogType::INFORMATION;
        }
        case MessageType::WARNING:
        {
                return LogType::WARNING;
        }
        }
        return LogType::ERROR;
}

std::string_view log_type_to_string(LogType type)
{
        switch (type)
        {
        case LogType::ERROR:
                return "error";
        case LogType::INFORMATION:
                return "information";
        case LogType::NORMAL:
                return "";
        case LogType::WARNING:
                return "warning";
        }
        return "unknown";
}

std::string write_log_event(const std::string_view& text, const LogType type)
{
        return write_log(text, log_type_to_string(type));
}
}

LogEvents::LogEvents()
{
        static std::atomic_int call_counter = 0;
        if (++call_counter != 1)
        {
                write_log_fatal_error_and_exit("Log events must be called once");
        }

        g_log_events = this;
}

LogEvents::~LogEvents()
{
        g_log_events = nullptr;
}

void LogEvents::log_event(const std::string_view& text, const LogType type) noexcept
{
        try
        {
                std::lock_guard lg(lock_);

                std::string log_text = write_log_event(text, type);

                if (!log_observers_.empty())
                {
                        const LogEvent event(std::move(log_text), type);
                        for (const std::function<void(const LogEvent&)>* observer : log_observers_)
                        {
                                (*observer)(event);
                        }
                }
        }
        catch (const std::exception& e)
        {
                std::string msg = std::string("Error in log observer: ") + e.what();
                write_log_fatal_error_and_exit(msg.c_str());
        }
        catch (...)
        {
                write_log_fatal_error_and_exit("Unknown error in log observer");
        }
}

void LogEvents::log_event(const std::string_view& text, const MessageType type) noexcept
{
        try
        {
                const LogType log_type = message_type_to_log_type(type);

                std::lock_guard lg(lock_);

                std::string log_text = write_log_event(text, log_type);

                if (!log_observers_.empty())
                {
                        const LogEvent event(std::move(log_text), log_type);
                        for (const std::function<void(const LogEvent&)>* observer : log_observers_)
                        {
                                (*observer)(event);
                        }
                }

                if (!msg_observers_.empty())
                {
                        const MessageEvent event(text, type);
                        for (const std::function<void(const MessageEvent&)>* observer : msg_observers_)
                        {
                                (*observer)(event);
                        }
                }
        }
        catch (const std::exception& e)
        {
                std::string msg = std::string("Error in message observer: ") + e.what();
                write_log_fatal_error_and_exit(msg.c_str());
        }
        catch (...)
        {
                write_log_fatal_error_and_exit("Unknown error in message observer");
        }
}

void LogEvents::insert(const std::function<void(const LogEvent&)>* observer)
{
        std::lock_guard lg(lock_);
        if (std::find(log_observers_.cbegin(), log_observers_.cend(), observer) == log_observers_.cend())
        {
                log_observers_.push_back(observer);
        }
}

void LogEvents::erase(const std::function<void(const LogEvent&)>* observer)
{
        std::lock_guard lg(lock_);
        auto iter = std::remove(log_observers_.begin(), log_observers_.end(), observer);
        log_observers_.erase(iter, log_observers_.cend());
}

void LogEvents::insert(const std::function<void(const MessageEvent&)>* observer)
{
        std::lock_guard lg(lock_);
        if (std::find(msg_observers_.cbegin(), msg_observers_.cend(), observer) == msg_observers_.cend())
        {
                msg_observers_.push_back(observer);
        }
}

void LogEvents::erase(const std::function<void(const MessageEvent&)>* observer)
{
        std::lock_guard lg(lock_);
        auto iter = std::remove(msg_observers_.begin(), msg_observers_.end(), observer);
        msg_observers_.erase(iter, msg_observers_.cend());
}

//

LogEventsObserver::LogEventsObserver(std::function<void(const LogEvent&)> observer) : observer_(std::move(observer))
{
        g_log_events->insert(&observer_);
}

LogEventsObserver::~LogEventsObserver()
{
        g_log_events->erase(&observer_);
}

MessageEventsObserver::MessageEventsObserver(std::function<void(const MessageEvent&)> observer)
        : observer_(std::move(observer))
{
        g_log_events->insert(&observer_);
}

MessageEventsObserver::~MessageEventsObserver()
{
        g_log_events->erase(&observer_);
}

//

void log_impl(const std::string_view& text, LogType type) noexcept
{
        g_log_events->log_event(text, type);
}

void log_impl(const std::string_view& text, MessageType type) noexcept
{
        g_log_events->log_event(text, type);
}
}
