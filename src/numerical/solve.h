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

#include "gauss.h"
#include "vec.h"

#include <src/com/type/trait.h>

#include <array>

namespace ns::numerical
{
namespace solve_implementation
{
template <std::size_t N, typename T>
inline constexpr std::array<Vector<N, T>, N> IDENTITY = []()
{
        std::array<Vector<N, T>, N> r;
        for (std::size_t i = 0; i < N; ++i)
        {
                r[i] = Vector<N, T>(0);
                r[i][i] = 1;
        }
        return r;
}();
}

template <std::size_t N, typename T>
constexpr Vector<N, T> linear_solve(const std::array<Vector<N, T>, N>& a, const Vector<N, T>& b)
{
        static_assert(is_floating_point<T>);

        return solve_gauss(a, b);
}

template <std::size_t N, std::size_t M, typename T>
constexpr std::array<Vector<M, T>, N> linear_solve(
        const std::array<Vector<N, T>, N>& a,
        const std::array<Vector<M, T>, N>& b)
{
        static_assert(is_floating_point<T>);

        return solve_gauss(a, b);
}

template <std::size_t N, typename T>
constexpr std::array<Vector<N, T>, N> inverse(const std::array<Vector<N, T>, N>& a)
{
        static_assert(is_floating_point<T>);

        return solve_gauss(a, solve_implementation::IDENTITY<N, T>);
}
}
