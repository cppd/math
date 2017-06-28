/*
Copyright (C) 2017 Topological Manifold

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

#include <string>

namespace LogImplementation
{
void write_log_message(const std::string& msg) noexcept;
}

#if 1
// Необязательные сообщения
inline void LOG(const std::string& msg) noexcept
{
        LogImplementation::write_log_message(msg);
}
#else
#define LOG(expr) (static_cast<void>(0))
#endif

// Обязательно выводимые сообщения
inline void LOG_ERROR(const std::string& msg) noexcept
{
        LogImplementation::write_log_message(msg);
}
