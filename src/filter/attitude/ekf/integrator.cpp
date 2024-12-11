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

#include "integrator.h"

#include "constant.h"
#include "quaternion.h"

#include <src/numerical/vector.h>

#include <cmath>

namespace ns::filter::attitude::ekf
{
namespace
{
template <typename T>
Quaternion<T> zeroth_order_theta(const numerical::Vector<3, T>& w, const T dt)
{
        const T wn = w.norm();

        if (wn < W_THRESHOLD<T>)
        {
                const Quaternion<T> theta{1, (dt / 2) * w};
                return theta;
        }

        const T k = wn * dt / 2;
        const Quaternion<T> theta{std::cos(k), (std::sin(k) / wn) * w};
        return theta;
}
}

template <typename T>
Quaternion<T> zeroth_order_quaternion_integrator(const Quaternion<T>& q, const numerical::Vector<3, T>& w, const T dt)
{
        const Quaternion<T> theta = zeroth_order_theta(w, dt);
        return theta * q;
}

template <typename T>
Quaternion<T> first_order_quaternion_integrator(
        const Quaternion<T>& q,
        const numerical::Vector<3, T>& w0,
        const numerical::Vector<3, T>& w1,
        const T dt)
{
        const numerical::Vector<3, T> wa = (w0 + w1) / T{2};
        const Quaternion<T> q0 = zeroth_order_theta(wa, dt);

        const Quaternion<T> qw0 = Quaternion<T>(0, w0);
        const Quaternion<T> qw1 = Quaternion<T>(0, w1);
        const Quaternion<T> q1 = (qw1 * qw0 - qw0 * qw1) * (dt * dt / 48);

        return (q0 + q1) * q;
}

#define TEMPLATE(T)                                                       \
        template Quaternion<T> zeroth_order_quaternion_integrator(        \
                const Quaternion<T>&, const numerical::Vector<3, T>&, T); \
        template Quaternion<T> first_order_quaternion_integrator(         \
                const Quaternion<T>&, const numerical::Vector<3, T>&, const numerical::Vector<3, T>&, T);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)

}
