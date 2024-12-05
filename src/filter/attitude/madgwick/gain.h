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

#include <type_traits>

namespace ns::filter::attitude::madgwick
{
// Measurement error (rad/s) to beta
template <typename T>
[[nodiscard]] T madgwick_beta(const T error)
{
        static_assert(std::is_floating_point_v<T>);

        // (50)
        // sqrt(3.0 / 4)
        return T{0.86602540378443864676L} * error;
}

// Rate of bias drift (rad/s/s) to zeta
template <typename T>
[[nodiscard]] T madgwick_zeta(const T rate)
{
        static_assert(std::is_floating_point_v<T>);

        // (51)
        // sqrt(3.0 / 4)
        return T{0.86602540378443864676L} * rate;
}
}
