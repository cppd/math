/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "write.h"

#include <algorithm>
#include <exception>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ns
{
namespace
{
LogType message_type_to_log_type(const MessageType type)
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

std::string_view log_type_to_string(const LogType type)
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

std::string write_log_event(const std::string_view text, const LogType type)
{
        return write_log(text, log_type_to_string(type));
}

class LogEvents final
{
        std::mutex lock_;
        std::vector<const std::function<void(const LogEvent&)>*> log_observers_;
        std::vector<const std::function<void(const MessageEvent&)>*> msg_observers_;

public:
        LogEvents() = default;

        LogEvents(const LogEvents&) = delete;
        LogEvents(LogEvents&&) = delete;
        LogEvents& operator=(const LogEvents&) = delete;
        LogEvents& operator=(LogEvents&&) = delete;

        void insert(const std::function<void(const LogEvent&)>* observer);
        void erase(const std::function<void(const LogEvent&)>* observer);

        void insert(const std::function<void(const MessageEvent&)>* observer);
        void erase(const std::function<void(const MessageEvent&)>* observer);

        void log_event(std::string_view text, LogType type) noexcept;
        void log_event(std::string_view text, MessageType type) noexcept;
};

void LogEvents::log_event(const std::string_view text, const LogType type) noexcept
{
        try
        {
                const std::lock_guard lg(lock_);

                std::string log_text = write_log_event(text, type);

                if (!log_observers_.empty())
                {
                        const LogEvent event(std::move(log_text), type);
                        for (const std::function<void(const LogEvent&)>* const observer : log_observers_)
                        {
                                (*observer)(event);
                        }
                }
        }
        catch (const std::exception& e)
        {
                const std::string msg = std::string("Error in log observer: ") + e.what();
                write_log_fatal_error_and_exit(msg.c_str());
        }
        catch (...)
        {
                write_log_fatal_error_and_exit("Unknown error in log observer");
        }
}

void LogEvents::log_event(const std::string_view text, const MessageType type) noexcept
{
        try
        {
                const LogType log_type = message_type_to_log_type(type);

                const std::lock_guard lg(lock_);

                std::string log_text = write_log_event(text, log_type);

                if (!log_observers_.empty())
                {
                        const LogEvent event(std::move(log_text), log_type);
                        for (const std::function<void(const LogEvent&)>* const observer : log_observers_)
                        {
                                (*observer)(event);
                        }
                }

                if (!msg_observers_.empty())
                {
                        const MessageEvent event(text, type);
                        for (const std::function<void(const MessageEvent&)>* const observer : msg_observers_)
                        {
                                (*observer)(event);
                        }
                }
        }
        catch (const std::exception& e)
        {
                const std::string msg = std::string("Error in message observer: ") + e.what();
                write_log_fatal_error_and_exit(msg.c_str());
        }
        catch (...)
        {
                write_log_fatal_error_and_exit("Unknown error in message observer");
        }
}

void LogEvents::insert(const std::function<void(const LogEvent&)>* const observer)
{
        const std::lock_guard lg(lock_);
        if (std::ranges::find(log_observers_, observer) == log_observers_.cend())
        {
                log_observers_.push_back(observer);
        }
}

void LogEvents::erase(const std::function<void(const LogEvent&)>* const observer)
{
        const std::lock_guard lg(lock_);
        const auto range = std::ranges::remove(log_observers_, observer);
        log_observers_.erase(range.begin(), range.end());
}

void LogEvents::insert(const std::function<void(const MessageEvent&)>* const observer)
{
        const std::lock_guard lg(lock_);
        if (std::ranges::find(msg_observers_, observer) == msg_observers_.cend())
        {
                msg_observers_.push_back(observer);
        }
}

void LogEvents::erase(const std::function<void(const MessageEvent&)>* const observer)
{
        const std::lock_guard lg(lock_);
        const auto range = std::ranges::remove(msg_observers_, observer);
        msg_observers_.erase(range.begin(), range.end());
}

LogEvents& log_events()
{
        static LogEvents log_events;
        return log_events;
}
}

//

void log(const std::string_view text, const LogType type) noexcept
{
        log_events().log_event(text, type);
}

void log(const std::string_view text, const MessageType type) noexcept
{
        log_events().log_event(text, type);
}

LogEventsObserver::LogEventsObserver(std::function<void(const LogEvent&)> observer)
        : observer_(std::move(observer))
{
        log_events().insert(&observer_);
}

LogEventsObserver::~LogEventsObserver()
{
        log_events().erase(&observer_);
}

MessageEventsObserver::MessageEventsObserver(std::function<void(const MessageEvent&)> observer)
        : observer_(std::move(observer))
{
        log_events().insert(&observer_);
}

MessageEventsObserver::~MessageEventsObserver()
{
        log_events().erase(&observer_);
}
}
