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

#pragma once

#include <src/com/output/event.h>

#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace application
{
class LogEvents final
{
        friend class LogEventsObserver;
        friend class MessageEventsObserver;

        const std::thread::id m_thread_id = std::this_thread::get_id();

        std::mutex m_lock;
        std::function<void(LogEvent&&)> m_log_events;
        std::function<void(MessageEvent&&)> m_msg_events;
        std::vector<const std::function<void(const LogEvent&)>*> m_log_observers;
        std::vector<const std::function<void(const MessageEvent&)>*> m_msg_observers;

        void insert(const std::function<void(const LogEvent&)>* observer);
        void erase(const std::function<void(const LogEvent&)>* observer);

        void insert(const std::function<void(const MessageEvent&)>* observer);
        void erase(const std::function<void(const MessageEvent&)>* observer);

        void log_event(LogEvent&& event);
        void message_event(MessageEvent&& event);

public:
        LogEvents();
        ~LogEvents();

        LogEvents(const LogEvents&) = delete;
        LogEvents(LogEvents&&) = delete;
        LogEvents& operator=(const LogEvents&) = delete;
        LogEvents& operator=(LogEvents&&) = delete;
};

class LogEventsObserver final
{
        std::function<void(const LogEvent&)> m_observer;

public:
        explicit LogEventsObserver(std::function<void(const LogEvent&)> observer);
        ~LogEventsObserver();

        LogEventsObserver(const LogEventsObserver&) = delete;
        LogEventsObserver(LogEventsObserver&&) = delete;
        LogEventsObserver& operator=(const LogEventsObserver&) = delete;
        LogEventsObserver& operator=(LogEventsObserver&&) = delete;
};

class MessageEventsObserver final
{
        std::function<void(const MessageEvent&)> m_observer;

public:
        explicit MessageEventsObserver(std::function<void(const MessageEvent&)> observer);
        ~MessageEventsObserver();

        MessageEventsObserver(const MessageEventsObserver&) = delete;
        MessageEventsObserver(MessageEventsObserver&&) = delete;
        MessageEventsObserver& operator=(const MessageEventsObserver&) = delete;
        MessageEventsObserver& operator=(MessageEventsObserver&&) = delete;
};

}
