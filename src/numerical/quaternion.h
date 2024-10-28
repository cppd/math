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

#include "quaternion_object.h" // IWYU pragma: export
#include "vector.h"

#include <cmath>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>

namespace std
{
template <typename T>
struct hash<::ns::numerical::Quaternion<T>> final
{
        [[nodiscard]] static size_t operator()(const ::ns::numerical::Quaternion<T>& q)
        {
                return q.hash();
        }
};

template <typename T>
struct tuple_size<::ns::numerical::Quaternion<T>> final : integral_constant<size_t, 4>
{
};
}

namespace ns::numerical
{
template <typename T>
[[nodiscard]] constexpr bool operator==(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return a.coeffs() == b.coeffs();
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator+(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return Quaternion<T>(a.coeffs() + b.coeffs());
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator-(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return Quaternion<T>(a.coeffs() - b.coeffs());
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator*(const Quaternion<T>& a, const T b)
{
        return Quaternion<T>(a.coeffs() * b);
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator*(const T b, const Quaternion<T>& a)
{
        return Quaternion<T>(a.coeffs() * b);
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator/(const Quaternion<T>& a, const T b)
{
        return Quaternion<T>(a.coeffs() / b);
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator*(const Quaternion<T>& a, const Quaternion<T>& b)
{
        // a[0] * b[0] - dot(a.vec(), b.vec())
        // a[0] * b.vec() + b[0] * a.vec() + cross(a.vec(), b.vec())
        Quaternion<T> res;
        res[0] = a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3];
        res[1] = a[0] * b[1] + b[0] * a[1] + a[2] * b[3] - a[3] * b[2];
        res[2] = a[0] * b[2] + b[0] * a[2] - a[1] * b[3] + a[3] * b[1];
        res[3] = a[0] * b[3] + b[0] * a[3] + a[1] * b[2] - a[2] * b[1];
        return res;
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator*(const Quaternion<T>& a, const Vector<3, T>& b)
{
        // -dot(a.vec(), b)
        // a[0] * b + cross(a.vec(), b)
        Quaternion<T> res;
        res[0] = -a[1] * b[0] - a[2] * b[1] - a[3] * b[2];
        res[1] = a[0] * b[0] + a[2] * b[2] - a[3] * b[1];
        res[2] = a[0] * b[1] - a[1] * b[2] + a[3] * b[0];
        res[3] = a[0] * b[2] + a[1] * b[1] - a[2] * b[0];
        return res;
}

template <typename T>
[[nodiscard]] constexpr Vector<3, T> multiply_vec(const Quaternion<T>& a, const Quaternion<T>& b)
{
        // (a * b).vec()
        // a[0] * b.vec() + b[0] * a.vec() + cross(a.vec(), b.vec())
        Vector<3, T> res;
        res[0] = a[0] * b[1] + b[0] * a[1] + a[2] * b[3] - a[3] * b[2];
        res[1] = a[0] * b[2] + b[0] * a[2] - a[1] * b[3] + a[3] * b[1];
        res[2] = a[0] * b[3] + b[0] * a[3] + a[1] * b[2] - a[2] * b[1];
        return res;
}

template <typename T>
[[nodiscard]] Vector<3, T> rotate_vector(const Quaternion<T>& q_unit, const Vector<3, T>& v)
{
        return (q_unit * v * q_unit.conjugate()).vec();
}

template <typename T>
[[nodiscard]] Quaternion<T> unit_quaternion_for_rotation(const Vector<3, T>& axis, const T angle)
{
        return {std::cos(angle / 2), std::sin(angle / 2) * axis.normalized()};
}

template <typename T>
[[nodiscard]] Vector<3, T> rotate_vector(const Vector<3, T>& axis, const T angle, const Vector<3, T>& v)
{
        const Quaternion q = unit_quaternion_for_rotation(axis, angle);
        return rotate_vector(q, v);
}
}
