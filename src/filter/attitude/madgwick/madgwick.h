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

namespace ns::filter::attitude::madgwick
{
namespace madgwick_implementation
{
template <typename T>
inline constexpr T MIN_ACC = 9.3; // m/s/s

template <typename T>
inline constexpr T MAX_ACC = 10.3; // m/s/s

template <typename T>
inline constexpr T MIN_MAG = 20; // uT

template <typename T>
inline constexpr T MAX_MAG = 70; // uT

template <typename T>
[[nodiscard]] numerical::Quaternion<T> normalize(const numerical::Quaternion<T>& q)
{
        const T norm = q.norm();
        if (norm > 0)
        {
                return q / norm;
        }
        return q;
}

template <typename T>
[[nodiscard]] numerical::Quaternion<T> compute_gn(const numerical::Quaternion<T> q, const numerical::Vector<3, T> an)
{
        // (25)
        // f_g
        const T f_0 = 2 * q.x() * q.z() - 2 * q.y() * q.w() - an[0];
        const T f_1 = 2 * q.x() * q.w() + 2 * q.y() * q.z() - an[1];
        const T f_2 = 1 - 2 * q.x() * q.x() - 2 * q.y() * q.y() - an[2];

        // (20) (26)
        // transpose(J_g) * f_g
        const numerical::Vector<3, T> f(2 * f_0, 2 * f_1, 4 * f_2);
        const T w = q.x() * f[1] - q.y() * f[0];
        const T x = q.z() * f[0] + q.w() * f[1] - q.x() * f[2];
        const T y = q.z() * f[1] - q.w() * f[0] - q.y() * f[2];
        const T z = q.x() * f[0] + q.y() * f[1];

        return normalize(numerical::Quaternion<T>(w, x, y, z));
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
        const T f_0 = 2 * q.x() * q.z() - 2 * q.y() * q.w() - an[0];
        const T f_1 = 2 * q.x() * q.w() + 2 * q.y() * q.z() - an[1];
        const T f_2 = 1 - 2 * q.x() * q.x() - 2 * q.y() * q.y() - an[2];

        // (29)
        // f_b
        const T f_3 =
                2 * b_x * (T{0.5} - q.y() * q.y() - q.z() * q.z()) + 2 * b_z * (q.x() * q.z() - q.w() * q.y()) - mn[0];
        const T f_4 = 2 * b_x * (q.x() * q.y() - q.w() * q.z()) + 2 * b_z * (q.w() * q.x() + q.y() * q.z()) - mn[1];
        const T f_5 =
                2 * b_x * (q.w() * q.y() + q.x() * q.z()) + 2 * b_z * (T{0.5} - q.x() * q.x() - q.y() * q.y()) - mn[2];

        // (20) (26) (30) (34) (44)
        // normalize(transpose(J_g / J_b) * (f_g / f_b))

        const numerical::Vector<6, T> f(2 * f_0, 2 * f_1, 4 * f_2, 2 * f_3, 2 * f_4, 2 * f_5);

        const numerical::Quaternion<T> bxq = b_x * q;
        const numerical::Quaternion<T> bzq = b_z * q;

        const T w = q.x() * f[1] - q.y() * f[0] - bzq.y() * f[3] - (bxq.z() - bzq.x()) * f[4] + bxq.y() * f[5];
        const T x = q.z() * f[0] + q.w() * f[1] - q.x() * f[2] + bzq.z() * f[3] + (bxq.y() + bzq.w()) * f[4]
                    + (bxq.z() - 2 * bzq.x()) * f[5];
        const T y = q.z() * f[1] - -q.w() * f[0] - q.y() * f[2] - (2 * bxq.y() + bzq.w()) * f[3]
                    + (bxq.x() + bzq.z()) * f[4] + (bxq.w() - 2 * bzq.y()) * f[5];
        const T z = q.x() * f[0] + q.y() * f[1] - (2 * bxq.z() - bzq.x()) * f[3] - (bxq.w() - bzq.y()) * f[4]
                    + bxq.x() * f[5];

        return normalize(numerical::Quaternion<T>(w, x, y, z));
}

template <typename T>
[[nodiscard]] numerical::Quaternion<T> update(
        const numerical::Quaternion<T> q,
        const numerical::Vector<3, T> w,
        const numerical::Vector<3, T> a,
        const T beta,
        const T dt)
{
        // (11)
        const numerical::Quaternion<T> d = q * (w / T{2});

        const T a_norm = a.norm();
        if (!(a_norm >= MIN_ACC<T> && a_norm <= MAX_ACC<T>))
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
                const T dt)
        {
                namespace impl = madgwick_implementation;
                q_ = impl::update(q_, w, a, beta, dt);
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
                const T dt)
        {
                namespace impl = madgwick_implementation;

                const T m_norm = m.norm();
                if (!(m_norm >= impl::MIN_MAG<T> && m_norm <= impl::MAX_MAG<T>))
                {
                        q_ = impl::update(q_, w - wb_, a, beta, dt);
                        return q_;
                }

                const T a_norm = a.norm();
                if (!(a_norm >= impl::MIN_ACC<T> && a_norm <= impl::MAX_ACC<T>))
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
