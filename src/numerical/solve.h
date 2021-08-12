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
#include "identity.h"
#include "vec.h"

#include <src/com/type/trait.h>

#include <array>

namespace ns::numerical
{
template <std::size_t N, typename T>
[[nodiscard]] constexpr Vector<N, T> linear_solve(const std::array<Vector<N, T>, N>& a, const Vector<N, T>& b)
{
        static_assert(is_floating_point<T>);

        return solve_gauss(a, b);
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr std::array<Vector<N, T>, N> inverse(const std::array<Vector<N, T>, N>& a)
{
        static_assert(is_floating_point<T>);

        return solve_gauss(a, identity_array<N, T>);
}
}
