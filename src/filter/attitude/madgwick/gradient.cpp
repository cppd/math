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

/*
Sebastian O.H. Madgwick.
An efficient orientation filter for inertial
and inertial/magnetic sensor arrays.
2010.
*/

#include "gradient.h"

#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

namespace ns::filter::attitude::madgwick
{
namespace
{
template <typename T>
[[nodiscard]] numerical::Quaternion<T> normalize(const T w, const T x, const T y, const T z)
{
        // normalizing without changing the sign of w
        const numerical::Quaternion<T> q(w, x, y, z);
        const T norm = q.norm();
        if (norm > 0)
        {
                return q / norm;
        }
        return q;
}
}

template <typename T>
numerical::Quaternion<T> compute_gn(const numerical::Quaternion<T>& q, const numerical::Vector<3, T>& an)
{
        const T w = q.w();
        const T x = q.x();
        const T y = q.y();
        const T z = q.z();

        // (25)
        // f_g
        const T f0 = 2 * x * z - 2 * y * w - an[0];
        const T f1 = 2 * x * w + 2 * y * z - an[1];
        const T f2 = 1 - 2 * x * x - 2 * y * y - an[2];

        // (20) (26)
        // transpose(J_g) * f_g

        const numerical::Vector<3, T> f(2 * f0, 2 * f1, 4 * f2);

        const T rw = x * f[1] - y * f[0];
        const T rx = z * f[0] + w * f[1] - x * f[2];
        const T ry = z * f[1] - w * f[0] - y * f[2];
        const T rz = x * f[0] + y * f[1];

        return normalize(rw, rx, ry, rz);
}

template <typename T>
numerical::Quaternion<T> compute_gn(
        const numerical::Quaternion<T>& q,
        const numerical::Vector<3, T>& an,
        const numerical::Vector<3, T>& mn,
        const T bx,
        const T bz)
{
        const T w = q.w();
        const T x = q.x();
        const T y = q.y();
        const T z = q.z();

        // (25)
        // f_g
        const T f0 = 2 * x * z - 2 * y * w - an[0];
        const T f1 = 2 * x * w + 2 * y * z - an[1];
        const T f2 = 1 - 2 * x * x - 2 * y * y - an[2];

        // (29)
        // f_b
        const T f3 = 2 * bx * (T{0.5} - y * y - z * z) + 2 * bz * (x * z - w * y) - mn[0];
        const T f4 = 2 * bx * (x * y - w * z) + 2 * bz * (w * x + y * z) - mn[1];
        const T f5 = 2 * bx * (w * y + x * z) + 2 * bz * (T{0.5} - x * x - y * y) - mn[2];

        // (20) (26) (30) (34) (44)
        // normalize(transpose(J_g / J_b) * (f_g / f_b))

        const numerical::Vector<6, T> f(2 * f0, 2 * f1, 4 * f2, 2 * f3, 2 * f4, 2 * f5);

        const numerical::Quaternion<T> bxq = bx * q;
        const numerical::Quaternion<T> bzq = bz * q;

        const T w0 = x * f[1] - y * f[0];
        const T x0 = z * f[0] + w * f[1] - x * f[2];
        const T y0 = z * f[1] - w * f[0] - y * f[2];
        const T z0 = x * f[0] + y * f[1];

        const T w1 = -bzq.y() * f[3] - (bxq.z() - bzq.x()) * f[4] + bxq.y() * f[5];
        const T x1 = bzq.z() * f[3] + (bxq.y() + bzq.w()) * f[4] + (bxq.z() - 2 * bzq.x()) * f[5];
        const T y1 = -(2 * bxq.y() + bzq.w()) * f[3] + (bxq.x() + bzq.z()) * f[4] + (bxq.w() - 2 * bzq.y()) * f[5];
        const T z1 = -(2 * bxq.z() - bzq.x()) * f[3] - (bxq.w() - bzq.y()) * f[4] + bxq.x() * f[5];

        return normalize(w0 + w1, x0 + x1, y0 + y1, z0 + z1);
}

#define TEMPLATE(T)                                                                                                    \
        template numerical::Quaternion<T> compute_gn(const numerical::Quaternion<T>&, const numerical::Vector<3, T>&); \
        template numerical::Quaternion<T> compute_gn(                                                                  \
                const numerical::Quaternion<T>&, const numerical::Vector<3, T>&, const numerical::Vector<3, T>&, T,    \
                T);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
