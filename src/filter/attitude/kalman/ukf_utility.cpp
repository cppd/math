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

#include "ukf_utility.h"

#include "quaternion.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/numerical/vector.h>

#include <cmath>

namespace ns::filter::attitude::kalman
{
namespace
{
template <typename T>
constexpr T A = 0.1;

template <typename T>
constexpr T F = 2 * (A<T> + 1);

template <typename T>
numerical::Vector<3, T> quaternion_to_error(const Quaternion<T>& q)
{
        const T c = F<T> / (A<T> + q.w());
        return c * q.vec();
}

template <typename T>
Quaternion<T> error_to_quaternion(const numerical::Vector<3, T>& p)
{
        constexpr T A1 = A<T>;
        constexpr T F1 = F<T>;

        constexpr T A2 = square(A1);
        constexpr T F2 = square(F1);

        const T n2 = p.norm_squared();

        const T w = (F1 * std::sqrt(F2 + (1 - A2) * n2) - A1 * n2) / (F2 + n2);
        ASSERT(w >= 0);

        const T c = (A1 + w) / F1;
        return {w, c * p};
}
}

template <typename T>
Quaternion<T> error_to_quaternion(const numerical::Vector<3, T>& error, const Quaternion<T>& center)
{
        ASSERT(center.is_unit());
        const Quaternion dq = error_to_quaternion(error);
        ASSERT(dq.is_unit());
        return dq * center;
}

template <typename T>
numerical::Vector<3, T> quaternion_to_error(const Quaternion<T>& q, const Quaternion<T>& center_inversed)
{
        ASSERT(q.is_unit());
        ASSERT(center_inversed.is_unit());
        const Quaternion<T> dq = q * center_inversed;
        return quaternion_to_error(dq);
}

#define TEMPLATE(T)                                                                                       \
        template Quaternion<T> error_to_quaternion(const numerical::Vector<3, T>&, const Quaternion<T>&); \
        template numerical::Vector<3, T> quaternion_to_error(const Quaternion<T>&, const Quaternion<T>&);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
