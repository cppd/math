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

#include "log_events.h"

#include <src/com/error.h>
#include <src/com/output/write.h>

#include <atomic>

namespace application
{
namespace
{
LogEvents* global_log_events = nullptr;

LogEvent::Type message_type_to_log_type(MessageEvent::Type type)
{
        switch (type)
        {
        case MessageEvent::Type::Error:
        case MessageEvent::Type::ErrorFatal:
        {
                return LogEvent::Type::Error;
        }
        case MessageEvent::Type::Information:
        {
                return LogEvent::Type::Information;
        }
        case MessageEvent::Type::Warning:
        {
                return LogEvent::Type::Warning;
        }
        }
        return LogEvent::Type::Error;
}

std::string_view log_type_to_string(LogEvent::Type type)
{
        switch (type)
        {
        case LogEvent::Type::Error:
                return "error";
        case LogEvent::Type::Information:
                return "information";
        case LogEvent::Type::Normal:
                return "";
        case LogEvent::Type::Warning:
                return "warning";
        }
        return "unknown";
}

void write_log_event(LogEvent* event)
{
        event->text = write_log(event->text, log_type_to_string(event->type));
}
}

LogEvents::LogEvents()
{
        static std::atomic_int call_counter = 0;
        if (++call_counter != 1)
        {
                error_fatal("Log events must be called once");
        }

        m_log_events = [this](LogEvent&& event)
        {
                log_event(std::move(event));
        };
        m_msg_events = [this](MessageEvent&& event)
        {
                message_event(std::move(event));
        };

        set_log_events(&m_log_events);
        set_message_events(&m_msg_events);

        global_log_events = this;
}

LogEvents::~LogEvents()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        global_log_events = nullptr;

        set_message_events(nullptr);
        set_log_events(nullptr);
}

void LogEvents::log_event(LogEvent&& event)
{
        try
        {
                std::lock_guard lg(m_lock);

                write_log_event(&event);

                for (const std::function<void(const LogEvent&)>* observer : m_log_observers)
                {
                        (*observer)(event);
                }
        }
        catch (const std::exception& e)
        {
                error_fatal(std::string("Error in log observer: ") + e.what());
        }
        catch (...)
        {
                error_fatal("Unknown error in log observer");
        }
}

void LogEvents::message_event(MessageEvent&& event)
{
        try
        {
                LogEvent log_event(event.text, message_type_to_log_type(event.type));

                std::lock_guard lg(m_lock);

                write_log_event(&log_event);

                for (const std::function<void(const LogEvent&)>* observer : m_log_observers)
                {
                        (*observer)(log_event);
                }

                for (const std::function<void(const MessageEvent&)>* observer : m_msg_observers)
                {
                        (*observer)(event);
                }
        }
        catch (const std::exception& e)
        {
                error_fatal(std::string("Error in message observer: ") + e.what());
        }
        catch (...)
        {
                error_fatal("Unknown error in message observer");
        }
}

void LogEvents::insert(const std::function<void(const LogEvent&)>* observer)
{
        std::lock_guard lg(m_lock);
        ASSERT(std::find(m_log_observers.cbegin(), m_log_observers.cend(), observer) == m_log_observers.cend());
        m_log_observers.push_back(observer);
}

void LogEvents::erase(const std::function<void(const LogEvent&)>* observer)
{
        std::lock_guard lg(m_lock);
        auto iter = std::remove(m_log_observers.begin(), m_log_observers.end(), observer);
        ASSERT(iter != m_log_observers.cend());
        m_log_observers.erase(iter, m_log_observers.cend());
}

void LogEvents::insert(const std::function<void(const MessageEvent&)>* observer)
{
        std::lock_guard lg(m_lock);
        ASSERT(std::find(m_msg_observers.cbegin(), m_msg_observers.cend(), observer) == m_msg_observers.cend());
        m_msg_observers.push_back(observer);
}

void LogEvents::erase(const std::function<void(const MessageEvent&)>* observer)
{
        std::lock_guard lg(m_lock);
        auto iter = std::remove(m_msg_observers.begin(), m_msg_observers.end(), observer);
        ASSERT(iter != m_msg_observers.cend());
        m_msg_observers.erase(iter, m_msg_observers.cend());
}

//

LogEventsObserver::LogEventsObserver(std::function<void(const LogEvent&)> observer) : m_observer(std::move(observer))
{
        global_log_events->insert(&m_observer);
}

LogEventsObserver::~LogEventsObserver()
{
        global_log_events->erase(&m_observer);
}

MessageEventsObserver::MessageEventsObserver(std::function<void(const MessageEvent&)> observer)
        : m_observer(std::move(observer))
{
        global_log_events->insert(&m_observer);
}

MessageEventsObserver::~MessageEventsObserver()
{
        global_log_events->erase(&m_observer);
}
}
