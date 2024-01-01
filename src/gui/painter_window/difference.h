/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/com/chrono.h>

#include <chrono>
#include <deque>
#include <tuple>

namespace ns::gui::painter_window
{
template <typename T>
class Difference final
{
        struct Point final
        {
                T data;
                Clock::time_point time;

                template <typename V>
                Point(V&& data, const Clock::time_point& time)
                        : data(std::forward<V>(data)),
                          time(time)
                {
                }
        };

        const Clock::duration interval_;
        std::deque<Point> deque_;

public:
        explicit Difference(const std::chrono::milliseconds& interval)
                : interval_(interval)
        {
        }

        template <typename V>
        std::tuple<T, double> compute(V&& data)
        {
                const Clock::time_point now = Clock::now();
                const Clock::time_point front_time = now - interval_;

                while (!deque_.empty() && deque_.front().time < front_time)
                {
                        deque_.pop_front();
                }

                deque_.emplace_back(std::forward<V>(data), now);

                return std::make_tuple(
                        deque_.back().data - deque_.front().data, duration(deque_.front().time, deque_.back().time));
        }
};
}
