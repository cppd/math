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

#pragma once

#include <src/com/time.h>

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
                TimePoint time;

                template <typename V>
                Point(V&& data, const TimePoint& time) : data(std::forward<V>(data)), time(time)
                {
                }
        };

        const TimeDuration m_interval;
        std::deque<Point> m_deque;

public:
        explicit Difference(std::chrono::milliseconds interval) : m_interval(interval)
        {
        }

        template <typename V>
        std::tuple<T, double> compute(V&& data)
        {
                TimePoint now = time();
                TimePoint front_time = now - m_interval;

                while (!m_deque.empty() && m_deque.front().time < front_time)
                {
                        m_deque.pop_front();
                }

                m_deque.emplace_back(std::forward<V>(data), now);

                return std::make_tuple(
                        m_deque.back().data - m_deque.front().data,
                        duration(m_deque.front().time, m_deque.back().time));
        }
};
}
