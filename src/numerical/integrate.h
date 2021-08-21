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

#include <cmath>

namespace ns::numerical
{
template <typename T, typename F>
T integrate(const F& f, T from, T to, int count)
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(std::is_same_v<T, decltype(f(T()))>);

        const T t_count = count;
        // Composite Trapezoidal Rule
        T sum = 0;
        for (T i = 1; i < t_count; ++i)
        {
                T x = std::lerp(from, to, i / t_count);
                sum += f(x);
        }
        T h_2 = (to - from) / (2 * t_count);
        return (f(from) + 2 * sum + f(to)) * h_2;
}
}
