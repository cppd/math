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

#pragma once

#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <cmath>
#include <type_traits>

namespace ns::filter::attitude
{
namespace madgwick_implementation
{
template <typename T>
[[nodiscard]] numerical::Quaternion<T> compute_gn(const numerical::Quaternion<T> q, const numerical::Vector<3, T> an)
{
        // (25)
        // f_g
        const T f_0 = 2 * q[1] * q[3] - 2 * q[2] * q[0] - an[0];
        const T f_1 = 2 * q[1] * q[0] + 2 * q[2] * q[3] - an[1];
        const T f_2 = 1 - 2 * q[1] * q[1] - 2 * q[2] * q[2] - an[2];

        // (20) (26)
        // transpose(J_g) * f_g
        const numerical::Vector<3, T> f(2 * f_0, 2 * f_1, 4 * f_2);
        const T w = q[1] * f[1] - q[2] * f[0];
        const T x = q[3] * f[0] + q[0] * f[1] - q[1] * f[2];
        const T y = q[3] * f[1] - q[0] * f[0] - q[2] * f[2];
        const T z = q[1] * f[0] + q[2] * f[1];
        return numerical::Quaternion<T>(w, x, y, z).normalized();
}

template <typename T>
[[nodiscard]] numerical::Quaternion<T> compute_gn(
        const numerical::Quaternion<T> q,
        const numerical::Vector<3, T> an,
        const numerical::Vector<3, T> mn,
        const T b_x,
        const T b_z)
{
        // (25)
        // f_g
        const T f_0 = 2 * q[1] * q[3] - 2 * q[2] * q[0] - an[0];
        const T f_1 = 2 * q[1] * q[0] + 2 * q[2] * q[3] - an[1];
        const T f_2 = 1 - 2 * q[1] * q[1] - 2 * q[2] * q[2] - an[2];

        // (29)
        // f_b
        const T f_3 = 2 * b_x * (T{0.5} - q[2] * q[2] - q[3] * q[3]) + 2 * b_z * (q[1] * q[3] - q[0] * q[2]) - mn[0];
        const T f_4 = 2 * b_x * (q[1] * q[2] - q[0] * q[3]) + 2 * b_z * (q[0] * q[1] + q[2] * q[3]) - mn[1];
        const T f_5 = 2 * b_x * (q[0] * q[2] + q[1] * q[3]) + 2 * b_z * (T{0.5} - q[1] * q[1] - q[2] * q[2]) - mn[2];

        // (20) (26) (30) (34) (44)
        // normalize(transpose(J_g / J_b) * (f_g / f_b))

        const numerical::Vector<6, T> f(2 * f_0, 2 * f_1, 4 * f_2, 2 * f_3, 2 * f_4, 2 * f_5);

        const numerical::Quaternion<T> bxq = b_x * q;
        const numerical::Quaternion<T> bzq = b_z * q;

        const T w = q[1] * f[1] - q[2] * f[0] - bzq[2] * f[3] - (bxq[3] - bzq[1]) * f[4] + bxq[2] * f[5];
        const T x = q[3] * f[0] + q[0] * f[1] - q[1] * f[2] + bzq[3] * f[3] + (bxq[2] + bzq[0]) * f[4]
                    + (bxq[3] - 2 * bzq[1]) * f[5];
        const T y = q[3] * f[1] - -q[0] * f[0] - q[2] * f[2] - (2 * bxq[2] + bzq[0]) * f[3] + (bxq[1] + bzq[3]) * f[4]
                    + (bxq[0] - 2 * bzq[2]) * f[5];
        const T z = q[1] * f[0] + q[2] * f[1] - (2 * bxq[3] - bzq[1]) * f[3] - (bxq[0] - bzq[2]) * f[4] + bxq[1] * f[5];

        return numerical::Quaternion<T>(w, x, y, z).normalized();
}

template <typename T>
[[nodiscard]] numerical::Quaternion<T> update(
        const numerical::Quaternion<T> q,
        const numerical::Vector<3, T> w,
        const numerical::Vector<3, T> a,
        const T beta,
        const T dt,
        const T min_accelerometer)
{
        // (11)
        const numerical::Quaternion<T> d = q * (w / T{2});

        const T a_norm = a.norm();
        if (!(a_norm >= min_accelerometer))
        {
                // (13)
                return (q + d * dt).normalized();
        }

        const numerical::Vector<3, T> an = a / a_norm;

        const numerical::Quaternion<T> gn = compute_gn(q, an);

        // (42) (43) (44)
        return (q + (d - beta * gn) * dt).normalized();
}
}

// Measurement error (rad/s) to beta
template <typename T>
[[nodiscard]] T madgwick_beta(const T error)
{
        static_assert(std::is_floating_point_v<T>);
        // (50)
        // sqrt(3.0 / 4)
        return T{0.86602540378443864676L} * error;
}

// Rate of bias drift (rad/s/s) to zeta
template <typename T>
[[nodiscard]] T madgwick_zeta(const T rate)
{
        static_assert(std::is_floating_point_v<T>);
        // (51)
        // sqrt(3.0 / 4)
        return T{0.86602540378443864676L} * rate;
}

template <typename T>
class Madgwick final
{
        numerical::Quaternion<T> q_{1, 0, 0, 0};

public:
        [[nodiscard]] numerical::Quaternion<T> update(
                const numerical::Vector<3, T> w,
                const numerical::Vector<3, T> a,
                const T beta,
                const T dt,
                const T min_accelerometer)
        {
                namespace impl = madgwick_implementation;
                q_ = impl::update(q_, w, a, beta, dt, min_accelerometer);
                return q_;
        }
};

template <typename T>
class MadgwickMarg final
{
        numerical::Quaternion<T> q_{1, 0, 0, 0};
        T b_x_{1};
        T b_z_{0};
        numerical::Vector<3, T> wb_{0, 0, 0};

public:
        [[nodiscard]] numerical::Quaternion<T> update(
                const numerical::Vector<3, T> w,
                const numerical::Vector<3, T> a,
                const numerical::Vector<3, T> m,
                const T beta,
                const T zeta,
                const T dt,
                const T min_accelerometer,
                const T min_magnetometer)
        {
                namespace impl = madgwick_implementation;

                const T m_norm = m.norm();
                if (!(m_norm >= min_magnetometer))
                {
                        q_ = impl::update(q_, w - wb_, a, beta, dt, min_accelerometer);
                        return q_;
                }

                const T a_norm = a.norm();
                if (!(a_norm >= min_accelerometer))
                {
                        // (11) (49)
                        const numerical::Quaternion<T> d = q_ * ((w - wb_) / T{2});
                        // (13)
                        q_ = (q_ + d * dt).normalized();
                        return q_;
                }

                const numerical::Vector<3, T> an = a / a_norm;
                const numerical::Vector<3, T> mn = m / m_norm;

                const numerical::Quaternion<T> gn = impl::compute_gn(q_, an, mn, b_x_, b_z_);

                // (47)
                const numerical::Vector<3, T> w_err = T{2} * numerical::multiply_vec(q_.conjugate(), gn);

                // (48)
                wb_ += w_err * (dt * zeta);

                // (42) (43) (44) (49)
                const numerical::Quaternion<T> d = q_ * ((w - wb_) / T{2});
                q_ = (q_ + (d - beta * gn) * dt).normalized();

                // (45) (46)
                const numerical::Vector<3, T> h = numerical::rotate_vector(q_, mn);
                b_x_ = std::sqrt(h[0] * h[0] + h[1] * h[1]);
                b_z_ = h[2];

                return q_;
        }
};

}
