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

#include <src/application/log_events.h>

namespace ns
{
void LOG(const std::string& msg) noexcept
{
        application::log_impl(application::LogEvent(msg, application::LogEvent::Type::Normal));
}

void LOG_ERROR(const std::string& msg) noexcept
{
        application::log_impl(application::LogEvent(msg, application::LogEvent::Type::Error));
}

void LOG_WARNING(const std::string& msg) noexcept
{
        application::log_impl(application::LogEvent(msg, application::LogEvent::Type::Warning));
}

void LOG_INFORMATION(const std::string& msg) noexcept
{
        application::log_impl(application::LogEvent(msg, application::LogEvent::Type::Information));
}
}
