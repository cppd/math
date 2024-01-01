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

#include <src/com/type/concept.h>

#include <type_traits>

namespace ns::numerical
{
template <typename T, typename F>
[[nodiscard]] constexpr T integrate(const F& f, const T& from, const T& to, const int count)
{
        static_assert(FloatingPoint<T>);
        static_assert(std::is_same_v<T, decltype(f(T()))>);

        // Composite Trapezoidal Rule

        const T t_count = count;
        const T distance = to - from;
        T sum = 0;
        for (int i = 1; i < count; ++i)
        {
                const T x = from + (i / t_count) * distance;
                sum += f(x);
        }
        const T h_2 = distance / (2 * t_count);
        return (f(from) + 2 * sum + f(to)) * h_2;
}
}
