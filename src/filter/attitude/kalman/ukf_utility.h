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

#include <src/com/exponent.h>
#include <src/numerical/vector.h>

#include <cmath>

namespace ns::filter::attitude::kalman
{
template <typename T>
[[nodiscard]] numerical::Vector<3, T> quaternion_to_error(const Quaternion<T>& q, const T a, const T f)
{
        const T c = f / (a + q.w());
        return c * q.vec();
}

template <typename T>
[[nodiscard]] Quaternion<T> error_to_quaternion(const numerical::Vector<3, T>& p, const T a, const T f)
{
        const T a2 = square(a);
        const T f2 = square(f);
        const T n2 = p.norm_squared();
        const T w = (f * std::sqrt(f2 + (1 - a2) * n2) - a * n2) / (f2 + n2);
        const T c = (a + w) / f;
        return {w, c * p};
}
}
