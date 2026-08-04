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

#pragma once

#include "matrix.h"
#include "quaternion.h"
#include "vector.h"

#include <src/com/error.h>

#include <cmath>

namespace ns::numerical
{
template <typename Quaternion>
[[nodiscard]] Quaternion rotation_vector_to_quaternion(
        const QuaternionType<Quaternion> angle,
        const Vector<3, QuaternionType<Quaternion>>& axis)
{
        using T = QuaternionType<Quaternion>;

        T sin = std::sin(angle / 2);
        T cos = std::cos(angle / 2);

        if (cos < 0)
        {
                sin = -sin;
                cos = -cos;
        }

        return {
                sin * axis.normalized(),
                cos,
        };
}

template <bool GLOBAL_TO_LOCAL, typename T>
[[nodiscard]] Matrix<3, 3, T> rotation_vector_to_matrix(const T angle, const Vector<3, T>& axis)
{
        const T s = std::sin(GLOBAL_TO_LOCAL ? -angle : angle);
        const T c = 1 - std::cos(angle);

        const Vector<3, T> vn = axis.normalized();

        const T v0 = vn[0];
        const T v1 = vn[1];
        const T v2 = vn[2];

        const T s0 = s * v0;
        const T s1 = s * v1;
        const T s2 = s * v2;

        const T c0 = c * v0;
        const T c1 = c * v1;
        const T c2 = c * v2;

        const T c00 = c0 * v0;
        const T c01 = c0 * v1;
        const T c02 = c0 * v2;
        const T c11 = c1 * v1;
        const T c12 = c1 * v2;
        const T c22 = c2 * v2;

        return {
                {1 - c11 - c22,      c01 - s2,      c02 + s1},
                {     c01 + s2, 1 - c00 - c22,      c12 - s0},
                {     c02 - s1,      c12 + s0, 1 - c00 - c11},
        };
}

template <bool JPL, typename T>
[[nodiscard]] constexpr Matrix<3, 3, T> rotation_quaternion_to_matrix(const QuaternionHJ<T, JPL>& q)
{
        static constexpr bool GLOBAL_TO_LOCAL = JPL;

        ASSERT(q.is_unit());

        const T x = q.x();
        const T y = q.y();
        const T z = q.z();
        const T w = GLOBAL_TO_LOCAL ? -q.w() : q.w();

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
[[nodiscard]] Quaternion rotation_matrix_to_quaternion(const Matrix<3, 3, QuaternionType<Quaternion>>& m)
{
        using T = QuaternionType<Quaternion>;
        static constexpr bool GLOBAL_TO_LOCAL = QuaternionTraits<Quaternion>::JPL;

        ASSERT(m.is_rotation());

        const auto result = [](const T& x, const T& y, const T& z, const T& w)
        {
                const Quaternion q{
                        {x, y, z},
                        GLOBAL_TO_LOCAL ? -w : w
                };
                return q.normalized();
        };

        const T m00 = m[0, 0];
        const T m01 = m[0, 1];
        const T m02 = m[0, 2];
        const T m10 = m[1, 0];
        const T m11 = m[1, 1];
        const T m12 = m[1, 2];
        const T m20 = m[2, 0];
        const T m21 = m[2, 1];
        const T m22 = m[2, 2];

        if (m22 < 0)
        {
                if (m00 > m11)
                {
                        const T x = 1 + m00 - m11 - m22;
                        const T y = m01 + m10;
                        const T z = m20 + m02;
                        const T w = m21 - m12;

                        return result(x, y, z, w);
                }

                const T x = m01 + m10;
                const T y = 1 - m00 + m11 - m22;
                const T z = m12 + m21;
                const T w = m02 - m20;

                return result(x, y, z, w);
        }

        if (m00 < -m11)
        {
                const T x = m20 + m02;
                const T y = m12 + m21;
                const T z = 1 - m00 - m11 + m22;
                const T w = m10 - m01;

                return result(x, y, z, w);
        }

        const T x = m21 - m12;
        const T y = m02 - m20;
        const T z = m10 - m01;
        const T w = 1 + m00 + m11 + m22;

        return result(x, y, z, w);
}
}
