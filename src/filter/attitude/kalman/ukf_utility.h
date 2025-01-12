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

#include "quaternion.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/numerical/vector.h>

#include <cmath>

namespace ns::filter::attitude::kalman
{
namespace ukf_utility_implementation
{
template <typename T>
inline constexpr T A = 0.1;

template <typename T>
inline constexpr T F = 2 * (A<T> + 1);
}

template <typename T>
[[nodiscard]] numerical::Vector<3, T> quaternion_to_error(const Quaternion<T>& q)
{
        namespace impl = ukf_utility_implementation;

        constexpr T A = impl::A<T>;
        constexpr T F = impl::F<T>;

        const T c = F / (A + q.w());
        return c * q.vec();
}

template <typename T>
[[nodiscard]] Quaternion<T> error_to_quaternion(const numerical::Vector<3, T>& p)
{
        namespace impl = ukf_utility_implementation;

        constexpr T A = impl::A<T>;
        constexpr T F = impl::F<T>;

        constexpr T A2 = square(A);
        constexpr T F2 = square(F);
        const T n2 = p.norm_squared();

        const T w = (F * std::sqrt(F2 + (1 - A2) * n2) - A * n2) / (F2 + n2);
        ASSERT(w >= 0);

        const T c = (A + w) / F;
        return {w, c * p};
}
}
