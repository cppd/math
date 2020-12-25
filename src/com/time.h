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

#include <chrono>

namespace ns
{
using TimePoint = std::chrono::steady_clock::time_point;
using TimeDuration = std::chrono::steady_clock::duration;

inline TimePoint time() noexcept
{
        return std::chrono::steady_clock::now();
}

inline double duration_from(const TimePoint& t) noexcept
{
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - t).count();
}

inline double duration(const TimePoint& t1, const TimePoint& t2) noexcept
{
        return std::chrono::duration<double>(t2 - t1).count();
}
}
