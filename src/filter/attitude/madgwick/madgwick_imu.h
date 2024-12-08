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

#include "gradient.h"

#include <src/filter/attitude/limit.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <cmath>

namespace ns::filter::attitude::madgwick
{
template <typename T>
class MadgwickImu final
{
        numerical::Quaternion<T> q_{1, 0, 0, 0};

public:
        bool update(const numerical::Vector<3, T>& w, const numerical::Vector<3, T>& a, const T beta, const T dt)
        {
                // (11)
                const numerical::Quaternion<T> d = q_ * (w / T{2});

                const T a_norm = a.norm();

                if (!acc_suitable(a_norm))
                {
                        // (13)
                        q_ = (q_ + d * dt).normalized();
                        return false;
                }

                const numerical::Quaternion<T> gn = compute_gn(q_, a / a_norm);
                // (42) (43) (44)
                q_ = (q_ + (d - beta * gn) * dt).normalized();
                return true;
        }

        [[nodiscard]] const numerical::Quaternion<T>& attitude() const
        {
                return q_;
        }
};
}
