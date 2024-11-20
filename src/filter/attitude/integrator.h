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

#include "quaternion.h"

#include <src/numerical/vector.h>

#include <cmath>

namespace ns::filter::attitude
{
template <typename T>
[[nodiscard]] Quaternion<T> zeroth_order_quaternion_integrator(
        const Quaternion<T>& q,
        const numerical::Vector<3, T>& w,
        const T dt)
{
        const T wn = w.norm();

        if (wn < T{1e-5})
        {
                const Quaternion<T> theta{1, (dt / 2) * w};
                return theta * q;
        }

        const T k = wn * dt / 2;
        const Quaternion<T> theta{std::cos(k), (std::sin(k) / wn) * w};

        return theta * q;
}

template <typename T>
[[nodiscard]] Quaternion<T> first_order_quaternion_integrator(
        const Quaternion<T>& q,
        const numerical::Vector<3, T>& w0,
        const numerical::Vector<3, T>& w1,
        const T dt)
{
        const numerical::Vector<3, T> wa = (w0 + w1) / T{2};
        const Quaternion<T> q1 = zeroth_order_quaternion_integrator(q, wa, dt);

        const Quaternion<T> qw0 = Quaternion<T>(0, w0);
        const Quaternion<T> qw1 = Quaternion<T>(0, w1);
        const Quaternion<T> q2 = (qw1 * qw0 - qw0 * qw1) * (dt * dt / 48);

        return (q1 + q2) * q;
}
}
