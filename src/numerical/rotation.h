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

#include <src/com/error.h>

#include <cmath>
#include <cstddef>

namespace ns::numerical
{
template <
        typename T,
        bool JPL,
        template <std::size_t, typename> typename Vector,
        template <typename, bool> typename Quaternion>
[[nodiscard]] Quaternion<T, JPL> rotation_vector_to_quaternion(const Vector<3, T>& v)
{
        const T norm = v.norm();

        if (norm > 0)
        {
                return {
                        (std::sin(norm / 2) / norm) * v,
                        std::cos(norm / 2),
                };
        }

        return {
                {0, 0, 0},
                1,
        };
}

template <
        typename T,
        bool JPL,
        template <std::size_t, std::size_t, typename> typename Matrix,
        template <typename, bool> typename Quaternion>
[[nodiscard]] constexpr Matrix<3, 3, T> rotation_quaternion_to_matrix(const Quaternion<T, JPL>& q)
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

template <
        typename T,
        bool JPL,
        template <std::size_t, std::size_t, typename> typename Matrix,
        template <typename, bool> typename Quaternion>
[[nodiscard]] Quaternion<T, JPL> rotation_matrix_to_quaternion(const Matrix<3, 3, T>& m)
{
        static constexpr bool GLOBAL_TO_LOCAL = JPL;

        ASSERT(m.is_rotation());

        const T m00 = m[0, 0];
        const T m01 = m[0, 1];
        const T m02 = m[0, 2];
        const T m10 = m[1, 0];
        const T m11 = m[1, 1];
        const T m12 = m[1, 2];
        const T m20 = m[2, 0];
        const T m21 = m[2, 1];
        const T m22 = m[2, 2];

        T x;
        T y;
        T z;
        T w;

        if (m22 < 0)
        {
                if (m00 > m11)
                {
                        x = 1 + m00 - m11 - m22;
                        y = m01 + m10;
                        z = m20 + m02;
                        w = m21 - m12;
                }
                else
                {
                        x = m01 + m10;
                        y = 1 - m00 + m11 - m22;
                        z = m12 + m21;
                        w = m02 - m20;
                }
        }
        else
        {
                if (m00 < -m11)
                {
                        x = m20 + m02;
                        y = m12 + m21;
                        z = 1 - m00 - m11 + m22;
                        w = m10 - m01;
                }
                else
                {
                        x = m21 - m12;
                        y = m02 - m20;
                        z = m10 - m01;
                        w = 1 + m00 + m11 + m22;
                }
        }

        const Quaternion<T, JPL> q{
                {x, y, z},
                GLOBAL_TO_LOCAL ? -w : w
        };

        return q.normalized();
}
}
