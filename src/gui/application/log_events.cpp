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
LogEvents::LogEvents()
{
        m_empty_window_log = [](std::string&&, const Srgb8&) {};
        m_window_log_ptr = &m_empty_window_log;

        m_events = [this](LogEvent&& event) {
                // Здесь без вызовов функции LOG, так как начнёт вызывать сама себя

                std::string text = format_log_text(event.text);

                write_formatted_log_text(text);

                switch (event.type)
                {
                case LogEvent::Type::Normal:
                        (*m_window_log_ptr)(std::move(text), Srgb8(0, 0, 0));
                        break;
                case LogEvent::Type::Error:
                        (*m_window_log_ptr)(std::move(text), Srgb8(255, 0, 0));
                        break;
                case LogEvent::Type::Warning:
                        (*m_window_log_ptr)(std::move(text), Srgb8(200, 150, 0));
                        break;
                case LogEvent::Type::Information:
                        (*m_window_log_ptr)(std::move(text), Srgb8(0, 0, 255));
                        break;
                }
        };

        set_log_events(&m_events);
}

LogEvents::~LogEvents()
{
        set_log_events(nullptr);
}

void LogEvents::set_window_log(const std::function<void(std::string&&, const Srgb8&)>* window_log_ptr)
{
        if (window_log_ptr)
        {
                ASSERT(*window_log_ptr);
                ASSERT(m_window_log_ptr == &m_empty_window_log);

                m_window_log_ptr = window_log_ptr;
        }
        else
        {
                m_window_log_ptr = &m_empty_window_log;
        }
}
}
