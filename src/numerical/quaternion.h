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

#include "matrix.h"
#include "quaternion_object.h" // IWYU pragma: export
#include "vector.h"

#include <src/com/error.h>

#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>

namespace std
{
template <typename T, bool JPL>
struct hash<::ns::numerical::QuaternionHJ<T, JPL>> final
{
        [[nodiscard]] static size_t operator()(const ::ns::numerical::QuaternionHJ<T, JPL>& q)
        {
                return q.hash();
        }
};

template <typename T, bool JPL>
struct tuple_size<::ns::numerical::QuaternionHJ<T, JPL>> final : integral_constant<size_t, 4>
{
};
}

namespace ns::numerical
{
namespace quaternion_implementation
{
template <typename>
struct QuaternionTraits;

template <typename T, bool JPL>
struct QuaternionTraits<QuaternionHJ<T, JPL>> final
{
        using Type = T;
        static constexpr bool GLOBAL_TO_LOCAL = JPL;
};

template <typename Quaternion>
using Type = QuaternionTraits<Quaternion>::Type;

template <typename Quaternion>
inline constexpr bool GLOBAL_TO_LOCAL = QuaternionTraits<Quaternion>::GLOBAL_TO_LOCAL;

template <typename T, bool JPL>
[[nodiscard]] constexpr QuaternionHJ<T, JPL> multiply_hamilton(
        const QuaternionHJ<T, JPL>& a,
        const QuaternionHJ<T, JPL>& b)
{
        // a.w() * b.w() - dot(a.vec(), b.vec())
        // a.w() * b.vec() + b.w() * a.vec() + cross(a.vec(), b.vec())
        QuaternionHJ<T, JPL> res;
        res.w() = a.w() * b.w() - a.x() * b.x() - a.y() * b.y() - a.z() * b.z();
        res.x() = a.w() * b.x() + b.w() * a.x() + a.y() * b.z() - a.z() * b.y();
        res.y() = a.w() * b.y() + b.w() * a.y() - a.x() * b.z() + a.z() * b.x();
        res.z() = a.w() * b.z() + b.w() * a.z() + a.x() * b.y() - a.y() * b.x();
        return res;
}

template <typename T, bool JPL>
[[nodiscard]] constexpr QuaternionHJ<T, JPL> multiply_hamilton(const QuaternionHJ<T, JPL>& a, const Vector<3, T>& b)
{
        // -dot(a.vec(), b)
        // a.w() * b + cross(a.vec(), b)
        QuaternionHJ<T, JPL> res;
        res.w() = -a.x() * b[0] - a.y() * b[1] - a.z() * b[2];
        res.x() = a.w() * b[0] + a.y() * b[2] - a.z() * b[1];
        res.y() = a.w() * b[1] - a.x() * b[2] + a.z() * b[0];
        res.z() = a.w() * b[2] + a.x() * b[1] - a.y() * b[0];
        return res;
}

template <typename T, bool JPL>
[[nodiscard]] constexpr QuaternionHJ<T, JPL> multiply_hamilton(const Vector<3, T>& a, const QuaternionHJ<T, JPL>& b)
{
        // -dot(a, b.vec())
        // b.w() * a + cross(a, b.vec())
        QuaternionHJ<T, JPL> res;
        res.w() = -a[0] * b.x() - a[1] * b.y() - a[2] * b.z();
        res.x() = b.w() * a[0] + a[1] * b.z() - a[2] * b.y();
        res.y() = b.w() * a[1] - a[0] * b.z() + a[2] * b.x();
        res.z() = b.w() * a[2] + a[0] * b.y() - a[1] * b.x();
        return res;
}

template <typename T, bool JPL>
[[nodiscard]] constexpr Vector<3, T> multiply_hamilton_vec(const QuaternionHJ<T, JPL>& a, const QuaternionHJ<T, JPL>& b)
{
        // (a * b).vec()
        // a.w() * b.vec() + b.w() * a.vec() + cross(a.vec(), b.vec())
        Vector<3, T> res;
        res[0] = a.w() * b.x() + b.w() * a.x() + a.y() * b.z() - a.z() * b.y();
        res[1] = a.w() * b.y() + b.w() * a.y() - a.x() * b.z() + a.z() * b.x();
        res[2] = a.w() * b.z() + b.w() * a.z() + a.x() * b.y() - a.y() * b.x();
        return res;
}
}

template <typename T, bool JPL>
[[nodiscard]] constexpr QuaternionHJ<T, JPL> operator*(const QuaternionHJ<T, JPL>& a, const QuaternionHJ<T, JPL>& b)
{
        namespace impl = quaternion_implementation;
        if constexpr (JPL)
        {
                return impl::multiply_hamilton(b, a);
        }
        else
        {
                return impl::multiply_hamilton(a, b);
        }
}

template <typename T, bool JPL>
[[nodiscard]] constexpr QuaternionHJ<T, JPL> operator*(const QuaternionHJ<T, JPL>& a, const Vector<3, T>& b)
{
        namespace impl = quaternion_implementation;
        if constexpr (JPL)
        {
                return impl::multiply_hamilton(b, a);
        }
        else
        {
                return impl::multiply_hamilton(a, b);
        }
}

template <typename T, bool JPL>
[[nodiscard]] constexpr QuaternionHJ<T, JPL> operator*(const Vector<3, T>& a, const QuaternionHJ<T, JPL>& b)
{
        namespace impl = quaternion_implementation;
        if constexpr (JPL)
        {
                return impl::multiply_hamilton(b, a);
        }
        else
        {
                return impl::multiply_hamilton(a, b);
        }
}

template <typename T, bool JPL>
[[nodiscard]] constexpr Vector<3, T> multiply_vec(const QuaternionHJ<T, JPL>& a, const QuaternionHJ<T, JPL>& b)
{
        namespace impl = quaternion_implementation;
        if constexpr (JPL)
        {
                return impl::multiply_hamilton_vec(b, a);
        }
        else
        {
                return impl::multiply_hamilton_vec(a, b);
        }
}

template <typename T, bool JPL>
[[nodiscard]] Vector<3, T> rotate_vector(const QuaternionHJ<T, JPL>& q_unit, const Vector<3, T>& v)
{
        ASSERT(q_unit.is_unit());

        return multiply_vec(q_unit * v, q_unit.conjugate());
}

template <typename Quaternion>
[[nodiscard]] Matrix<3, 3, quaternion_implementation::Type<Quaternion>> rotation_quaternion_to_matrix(
        const Quaternion& q)
{
        namespace impl = quaternion_implementation;

        using T = impl::Type<Quaternion>;
        static constexpr bool GLOBAL_TO_LOCAL = impl::GLOBAL_TO_LOCAL<Quaternion>;

        ASSERT(q.is_unit());

        const T w = GLOBAL_TO_LOCAL ? -q.w() : q.w();
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

        return {
                {1 - yy - zz,     xy - zw,     xz + yw},
                {    xy + zw, 1 - xx - zz,     yz - xw},
                {    xz - yw,     yz + xw, 1 - xx - yy},
        };
}

template <typename Quaternion>
[[nodiscard]] Quaternion rotation_matrix_to_quaternion(
        const Matrix<3, 3, quaternion_implementation::Type<Quaternion>>& m)
{
        namespace impl = quaternion_implementation;

        using T = impl::Type<Quaternion>;
        static constexpr bool GLOBAL_TO_LOCAL = impl::GLOBAL_TO_LOCAL<Quaternion>;

        ASSERT(m.row(0).is_unit());
        ASSERT(m.row(1).is_unit());
        ASSERT(m.row(2).is_unit());

        const T m00 = m[0, 0];
        const T m01 = m[0, 1];
        const T m02 = m[0, 2];
        const T m10 = m[1, 0];
        const T m11 = m[1, 1];
        const T m12 = m[1, 2];
        const T m20 = m[2, 0];
        const T m21 = m[2, 1];
        const T m22 = m[2, 2];

        T w;
        T x;
        T y;
        T z;

        if (m22 < 0)
        {
                if (m00 > m11)
                {
                        w = m21 - m12;
                        x = 1 + m00 - m11 - m22;
                        y = m01 + m10;
                        z = m20 + m02;
                }
                else
                {
                        w = m02 - m20;
                        x = m01 + m10;
                        y = 1 - m00 + m11 - m22;
                        z = m12 + m21;
                }
        }
        else
        {
                if (m00 < -m11)
                {
                        w = m10 - m01;
                        x = m20 + m02;
                        y = m12 + m21;
                        z = 1 - m00 - m11 + m22;
                }
                else
                {
                        w = 1 + m00 + m11 + m22;
                        x = m21 - m12;
                        y = m02 - m20;
                        z = m10 - m01;
                }
        }

        const Quaternion q{
                GLOBAL_TO_LOCAL ? -w : w,
                {x, y, z}
        };

        const T norm = q.norm();

        return q / ((q.w() < 0) ? -norm : norm);
}
}
