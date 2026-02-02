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

#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <vector>

namespace ns::filter::attitude::determination
{
template <std::size_t AXIS, typename T>
[[nodiscard]] std::vector<numerical::Vector<3, T>> rotate_axis(const std::vector<numerical::Vector<3, T>>& values)
{
        std::vector<numerical::Vector<3, T>> res;
        res.reserve(values.size());
        for (const auto& v : values)
        {
                if constexpr (AXIS == 0)
                {
                        res.emplace_back(v[0], -v[1], -v[2]);
                }
                else if constexpr (AXIS == 1)
                {
                        res.emplace_back(-v[0], v[1], -v[2]);
                }
                else if constexpr (AXIS == 2)
                {
                        res.emplace_back(-v[0], -v[1], v[2]);
                }
                else
                {
                        static_assert(false);
                }
        }
        return res;
}

template <typename T>
[[nodiscard]] numerical::QuaternionHJ<T, true> rotate_axis(
        const numerical::QuaternionHJ<T, true>& q,
        const std::size_t axis)
{
        using Q = numerical::QuaternionHJ<T, true>;

        switch (axis)
        {
        case 0:
                return Q({q.w(), -q.z(), q.y()}, -q.x()); // q * ({1, 0, 0}, 0)
        case 1:
                return Q({q.z(), q.w(), -q.x()}, -q.y()); // q * ({0, 1, 0}, 0)
        case 2:
                return Q({-q.y(), q.x(), q.w()}, -q.z()); // q * ({0, 0, 1}, 0)
        default:
                return q;
        }
}
}
