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
#include <src/com/output/format.h>

namespace gui::application
{
namespace
{
LogEvents* g_log_events = nullptr;
std::atomic_int g_log_events_count = 0;

Srgb8 event_color(LogEvent::Type type)
{
        switch (type)
        {
        case LogEvent::Type::Normal:
        {
                return Srgb8(0, 0, 0);
        }
        case LogEvent::Type::Error:
        {
                return Srgb8(255, 0, 0);
        }
        case LogEvent::Type::Warning:
        {
                return Srgb8(200, 150, 0);
        }
        case LogEvent::Type::Information:
        {
                return Srgb8(0, 0, 255);
        }
        }
        error_fatal("Unknown log event type");
}
}

LogEvents::LogEvents()
{
        if (++g_log_events_count != 1)
        {
                error_fatal("Multiple LogEvent");
        }

        m_events = [this](LogEvent&& event) {
                std::string text = format_log_text(std::move(event.text));
                write_formatted_log_text(text);
                std::lock_guard lg(m_pointer_lock);
                if (m_pointer)
                {
                        (*m_pointer)(std::move(text), event_color(event.type));
                }
        };

        set_log_events(&m_events);

        g_log_events = this;
}

LogEvents::~LogEvents()
{
        g_log_events = nullptr;

        set_log_events(nullptr);
}

void LogEvents::set_log(const std::function<void(std::string&&, const Srgb8&)>* log_ptr)
{
        if (log_ptr)
        {
                ASSERT(*log_ptr);
                std::lock_guard lg(m_pointer_lock);
                ASSERT(!m_pointer);
                m_pointer = log_ptr;
        }
        else
        {
                std::lock_guard lg(m_pointer_lock);
                ASSERT(m_pointer);
                m_pointer = nullptr;
        }
}

//

SetLogEvents::SetLogEvents(const std::function<void(std::string&&, const Srgb8&)>* log_ptr)
{
        ASSERT(log_ptr);
        ASSERT(*log_ptr);
        g_log_events->set_log(log_ptr);
}

SetLogEvents::~SetLogEvents()
{
        g_log_events->set_log(nullptr);
}
}
