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

namespace gui::application
{
class LogEvents final
{
        friend class LogEventsObserver;

        const std::thread::id m_thread_id = std::this_thread::get_id();

        std::mutex m_lock;
        std::function<void(LogEvent&&)> m_events;
        std::vector<const std::function<void(const LogEvent&)>*> m_observers;

        void insert(const std::function<void(const LogEvent&)>* observer);
        void erase(const std::function<void(const LogEvent&)>* observer);

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
        LogEventsObserver(std::function<void(const LogEvent&)> observer);
        ~LogEventsObserver();

        LogEventsObserver(const LogEventsObserver&) = delete;
        LogEventsObserver(LogEventsObserver&&) = delete;
        LogEventsObserver& operator=(const LogEventsObserver&) = delete;
        LogEventsObserver& operator=(LogEventsObserver&&) = delete;
};
}
