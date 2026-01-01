/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <string_view>

namespace ns::log_implementation
{
void log(const std::string_view& msg) noexcept;
void log_error(const std::string_view& msg) noexcept;
void log_warning(const std::string_view& msg) noexcept;
void log_information(const std::string_view& msg) noexcept;
}

#define LOG(m) ::ns::log_implementation::log(m)
#define LOG_ERROR(m) ::ns::log_implementation::log_error(m)
#define LOG_WARNING(m) ::ns::log_implementation::log_warning(m)
#define LOG_INFORMATION(m) ::ns::log_implementation::log_information(m)
