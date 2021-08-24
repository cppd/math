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

#include "log.h"

#include <src/application/log_events.h>

namespace ns::log_implementation
{
void log(const std::string_view& msg) noexcept
{
        application::log_impl(msg, application::LogType::NORMAL);
}

void log_error(const std::string_view& msg) noexcept
{
        application::log_impl(msg, application::LogType::ERROR);
}

void log_warning(const std::string_view& msg) noexcept
{
        application::log_impl(msg, application::LogType::WARNING);
}

void log_information(const std::string_view& msg) noexcept
{
        application::log_impl(msg, application::LogType::INFORMATION);
}
}
