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

#include <src/color/color.h>
#include <src/com/output/event.h>
#include <src/com/thread.h>

#include <functional>
#include <string>
#include <vector>

namespace gui::application
{
class LogEvents final
{
        friend class SetLogEvents;

        std::function<void(LogEvent&&)> m_events;

        const std::function<void(std::string&&, const Srgb8&)>* m_pointer = nullptr;
        SpinLock m_pointer_lock;

        void set_log(const std::function<void(std::string&&, const Srgb8&)>* log_ptr);

public:
        LogEvents();
        ~LogEvents();

        LogEvents(const LogEvents&) = delete;
        LogEvents(LogEvents&&) = delete;
        LogEvents& operator=(const LogEvents&) = delete;
        LogEvents& operator=(LogEvents&&) = delete;
};

class SetLogEvents final
{
public:
        SetLogEvents(const std::function<void(std::string&&, const Srgb8&)>* log_ptr);
        ~SetLogEvents();

        SetLogEvents(const SetLogEvents&) = delete;
        SetLogEvents(SetLogEvents&&) = delete;
        SetLogEvents& operator=(const SetLogEvents&) = delete;
        SetLogEvents& operator=(SetLogEvents&&) = delete;
};
}
