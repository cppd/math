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

#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <optional>

namespace ns::filter::attitude::kalman
{
template <typename T>
[[nodiscard]] Quaternion<T> initial_quaternion(const numerical::Vector<3, T>& acc);

template <typename T>
[[nodiscard]] Quaternion<T> initial_quaternion(const numerical::Vector<3, T>& acc, const numerical::Vector<3, T>& mag);

template <typename T>
struct MagMeasurement
{
        numerical::Vector<3, T> y;
        T variance;
};

template <typename T>
[[nodiscard]] std::optional<MagMeasurement<T>> mag_measurement(
        const numerical::Vector<3, T>& z_unit,
        const numerical::Vector<3, T>& m_unit,
        T variance);

template <typename T>
[[nodiscard]] numerical::Vector<3, T> global_to_local(
        const Quaternion<T>& q_unit,
        const numerical::Vector<3, T>& global)
{
        return numerical::rotate_vector(q_unit.q().conjugate(), global);
}
}
