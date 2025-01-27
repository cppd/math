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

#include <src/filter/core/sigma_points.h>
#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::filter::attitude::kalman
{
template <typename T>
[[nodiscard]] Quaternion<T> error_to_quaternion(const numerical::Vector<3, T>& error, const Quaternion<T>& center);

template <typename T>
[[nodiscard]] numerical::Vector<3, T> quaternion_to_error(const Quaternion<T>& q, const Quaternion<T>& center_inversed);

template <std::size_t N, typename T>
[[nodiscard]] core::SigmaPoints<N, T> create_sigma_points()
{
        return core::SigmaPoints<N, T>({
                .alpha = 1,
                .beta = 0,
                .kappa = 1,
        });
}
}
