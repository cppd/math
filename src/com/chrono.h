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

#include <chrono>
#include <ctime>

namespace ns
{
using Clock = std::chrono::steady_clock;

inline double duration_from(const Clock::time_point& t) noexcept
{
        return std::chrono::duration<double>(Clock::now() - t).count();
}

inline double duration(const Clock::time_point& t1, const Clock::time_point& t2) noexcept
{
        return std::chrono::duration<double>(t2 - t1).count();
}

std::tm time_to_local_time(const std::chrono::system_clock::time_point& time);
}
