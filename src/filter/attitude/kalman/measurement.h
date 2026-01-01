/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/numerical/vector.h>

#include <optional>

namespace ns::filter::attitude::kalman
{
template <typename T>
struct MagMeasurement
{
        numerical::Vector<3, T> y;
        T variance;
};

template <typename T>
[[nodiscard]] std::optional<MagMeasurement<T>> mag_measurement(
        const numerical::Vector<3, T>& z_unit,
        const numerical::Vector<3, T>& mag_unit,
        T variance);
}
