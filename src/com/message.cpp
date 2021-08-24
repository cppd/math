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

#include "message.h"

#include <src/application/log_events.h>

namespace ns
{
void message_error(const std::string_view& msg) noexcept
{
        application::log_impl(msg, application::MessageType::ERROR);
}

void message_error_fatal(const std::string_view& msg) noexcept
{
        application::log_impl(msg, application::MessageType::ERROR_FATAL);
}

void message_warning(const std::string_view& msg) noexcept
{
        application::log_impl(msg, application::MessageType::WARNING);
}

void message_information(const std::string_view& msg) noexcept
{
        application::log_impl(msg, application::MessageType::INFORMATION);
}
}
