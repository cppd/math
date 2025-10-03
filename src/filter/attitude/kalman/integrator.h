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

#include "quaternion.h"

#include <src/numerical/vector.h>

namespace ns::filter::attitude::kalman
{
template <typename T>
[[nodiscard]] Quaternion<T> zeroth_order_quaternion_integrator(
        const Quaternion<T>& q,
        const numerical::Vector<3, T>& w,
        T dt);

template <typename T>
[[nodiscard]] Quaternion<T> first_order_quaternion_integrator(
        const Quaternion<T>& q,
        const numerical::Vector<3, T>& w0,
        const numerical::Vector<3, T>& w1,
        T dt);
}
