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

#include "matrix.h"
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
template <typename T>
[[nodiscard]] Matrix<3, 3, T> unit_quaternion_to_rotation_matrix(const Quaternion<T>& q)
{
        const T w = q.w();
        const T x = q.x();
        const T y = q.y();
        const T z = q.z();

        const T xw = 2 * x * w;
        const T xx = 2 * x * x;
        const T xy = 2 * x * y;
        const T xz = 2 * x * z;
        const T yw = 2 * y * w;
        const T yy = 2 * y * y;
        const T yz = 2 * y * z;
        const T zw = 2 * z * w;
        const T zz = 2 * z * z;

        Matrix<3, 3, T> res;
        res[0, 0] = 1 - yy - zz;
        res[0, 1] = xy + zw;
        res[0, 2] = xz - yw;
        res[1, 0] = xy - zw;
        res[1, 1] = 1 - xx - zz;
        res[1, 2] = yz + xw;
        res[2, 0] = xz + yw;
        res[2, 1] = yz - xw;
        res[2, 2] = 1 - xx - yy;
        return res;
}

template <typename T>
[[nodiscard]] Quaternion<T> rotation_matrix_to_unit_quaternion(const Matrix<3, 3, T>& m)
{
        const T m00 = m[0, 0];
        const T m01 = m[0, 1];
        const T m02 = m[0, 2];
        const T m10 = m[1, 0];
        const T m11 = m[1, 1];
        const T m12 = m[1, 2];
        const T m20 = m[2, 0];
        const T m21 = m[2, 1];
        const T m22 = m[2, 2];

        Quaternion<T> q;

        if (m22 < 0)
        {
                if (m00 > m11)
                {
                        const T t = 1 + m00 - m11 - m22;
                        q = {m12 - m21, t, m01 + m10, m20 + m02};
                }
                else
                {
                        const T t = 1 - m00 + m11 - m22;
                        q = {m20 - m02, m01 + m10, t, m12 + m21};
                }
        }
        else
        {
                if (m00 < -m11)
                {
                        const T t = 1 - m00 - m11 + m22;
                        q = {m01 - m10, m20 + m02, m12 + m21, t};
                }
                else
                {
                        const T t = 1 + m00 + m11 + m22;
                        q = {t, m12 - m21, m20 - m02, m01 - m10};
                }
        }

        T norm = q.norm();
        if (q.w() < 0)
        {
                norm = -norm;
        }

        return q / norm;
}

}
