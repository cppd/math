/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/com/bit/width.h>

#include <cstddef>

namespace ns::geometry::core::convex_hull
{
template <std::size_t N, std::size_t BITS>
inline constexpr int MAX_DETERMINANT_PARABOLOID = []
{
        // |x x x x*x+x*x+x*x|
        // |x x x x*x+x*x+x*x|
        // |x x x x*x+x*x+x*x|
        // |x x x x*x+x*x+x*x|
        // max = x * x * x * (x*x + x*x + x*x) * 4!
        // max = (x ^ (N + 1)) * (N - 1) * N!

        static_assert(N >= 2 && N <= 33);
        static_assert(BITS > 0);

        unsigned __int128 f = 1;
        for (std::size_t i = 2; i <= N; ++i)
        {
                f *= i;
        }

        f *= (N - 1);

        return BITS * (N + 1) + static_cast<std::size_t>(bit_width(f));
}();

template <std::size_t N, std::size_t BITS>
inline constexpr int MAX_DETERMINANT = []
{
        // |x x x x|
        // |x x x x|
        // |x x x x|
        // |x x x x|
        // max = x * x * x * x * 4!
        // max = (x ^ N) * N!

        static_assert(N >= 2 && N <= 34);
        static_assert(BITS > 0);

        unsigned __int128 f = 1;
        for (std::size_t i = 2; i <= N; ++i)
        {
                f *= i;
        }

        return BITS * N + static_cast<std::size_t>(bit_width(f));
}();

template <std::size_t N, std::size_t BITS>
inline constexpr int MAX_PARABOLOID = []
{
        // max = x*x + x*x + x*x
        // max = (x ^ 2) * (N - 1)

        static_assert(N >= 2);
        static_assert(BITS > 0);

        return BITS * 2 + bit_width(N - 2);
}();
}
