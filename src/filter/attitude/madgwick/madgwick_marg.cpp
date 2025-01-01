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

/*
Sebastian O.H. Madgwick.
An efficient orientation filter for inertial
and inertial/magnetic sensor arrays.
2010.
*/

#include "madgwick_marg.h"

#include "gradient.h"

#include <src/filter/attitude/limit.h>
#include <src/numerical/quaternion.h>

namespace ns::filter::attitude::madgwick
{
template <typename T>
bool MadgwickMarg<
        T>::update(const Vector3& w, const Vector3& a, const Vector3& m, const T beta, const T zeta, const T dt)
{
        const T a_norm = a.norm();
        if (!acc_suitable(a_norm))
        {
                // (11)
                const numerical::Quaternion<T> d = q_ * ((w - wb_) / T{2});
                // (13)
                q_ = (q_ + d * dt).normalized();
                return false;
        }

        const Vector3 an = a / a_norm;

        const T m_norm = m.norm();
        if (!mag_suitable(m_norm))
        {
                // (11)
                const numerical::Quaternion<T> d = q_ * ((w - wb_) / T{2});
                const numerical::Quaternion<T> gn = compute_gn(q_, an);
                // (42) (43) (44)
                q_ = (q_ + (d - beta * gn) * dt).normalized();
                return false;
        }

        const Vector3 mn = m / m_norm;

        const numerical::Quaternion<T> gn = compute_gn(q_, an, mn, b_x_, b_z_);

        // (47)
        const Vector3 w_err = T{2} * numerical::multiply_vec(q_.conjugate(), gn);

        // (48)
        wb_ += w_err * (dt * zeta);

        // (42) (43) (44) (49)
        const numerical::Quaternion<T> d = q_ * ((w - wb_) / T{2});
        q_ = (q_ + (d - beta * gn) * dt).normalized();

        // (45) (46)
        const Vector3 h = numerical::rotate_vector(q_, mn);
        b_x_ = std::sqrt(h[0] * h[0] + h[1] * h[1]);
        b_z_ = h[2];

        return true;
}

template class MadgwickMarg<float>;
template class MadgwickMarg<double>;
template class MadgwickMarg<long double>;
}
