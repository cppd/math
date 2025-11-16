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

#include "madgwick_imu.h"

#include "gradient.h"

#include <src/filter/attitude/limit.h>
#include <src/numerical/quaternion.h>

namespace ns::filter::attitude::madgwick
{
template <typename T>
void MadgwickImu<T>::update(const Vector3& w, const Vector3& a, const T beta, const T dt)
{
        // (11)
        const numerical::Quaternion<T> d = q_ * (w / T{2});

        const T a_norm = a.norm();

        if (!acc_suitable(a_norm))
        {
                // (13)
                q_ = (q_ + d * dt).normalized();
                return;
        }

        const numerical::Quaternion<T> gn = compute_gn(q_, a / a_norm);

        // (42) (43) (44)
        q_ = (q_ + (d - beta * gn) * dt).normalized();
}

template class MadgwickImu<float>;
template class MadgwickImu<double>;
template class MadgwickImu<long double>;
}
