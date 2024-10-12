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

#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

namespace ns::filter::core
{
template <typename T>
class Madgwick final
{
        numerical::Vector<4, T> q_{1, 0, 0, 0};

public:
        [[nodiscard]] numerical::Quaternion<T> update(
                const numerical::Vector<3, T> w,
                const numerical::Vector<3, T> a,
                const T beta,
                const T dt)
        {
                const numerical::Vector<4, T> d = [&]
                {
                        numerical::Vector<4, T> r;
                        r[0] = -q_[1] * w[0] - q_[2] * w[1] - q_[3] * w[2];
                        r[1] = q_[0] * w[0] + q_[2] * w[2] - q_[3] * w[1];
                        r[2] = q_[0] * w[1] - q_[1] * w[2] + q_[3] * w[0];
                        r[3] = q_[0] * w[2] + q_[1] * w[1] - q_[2] * w[0];
                        return r / T{2};
                }();

                const T a_norm = a.norm();

                if (a_norm == 0)
                {
                        q_ = (q_ + d * dt).normalized();
                        return numerical::Quaternion(q_);
                }

                const T f_1 = 2 * q_[1] * q_[3] - 2 * q_[0] * q_[2] - a[0] / a_norm;
                const T f_2 = 2 * q_[0] * q_[1] + 2 * q_[2] * q_[3] - a[1] / a_norm;
                const T f_3 = 1 - 2 * q_[1] * q_[1] - 2 * q_[2] * q_[2] - a[2] / a_norm;

                const numerical::Vector<4, T> g = [&]
                {
                        numerical::Vector<4, T> r;
                        r[0] = 2 * q_[1] * f_2 - 2 * q_[2] * f_1;
                        r[1] = 2 * q_[3] * f_1 + 2 * q_[0] * f_2 - 4 * q_[1] * f_3;
                        r[2] = 2 * q_[3] * f_2 - 4 * q_[2] * f_3 - 2 * q_[0] * f_1;
                        r[3] = 2 * q_[1] * f_1 + 2 * q_[2] * f_2;
                        return r.normalized();
                }();

                q_ = (q_ + (d - beta * g) * dt).normalized();
                return numerical::Quaternion(q_);
        }
};
}
