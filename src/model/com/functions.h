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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/vector.h>

#include <cmath>
#include <cstddef>
#include <tuple>

namespace ns::model
{
template <std::size_t N, typename T>
std::tuple<numerical::Vector<N, T>, T> center_and_length_for_min_max(
        const numerical::Vector<N, T>& min,
        const numerical::Vector<N, T>& max)
{
        static_assert(FloatingPoint<T>);

        for (std::size_t i = 0; i < N; ++i)
        {
                if (!(min[i] < max[i]))
                {
                        error("Object min must be less than max, min = " + to_string(min)
                              + ", max = " + to_string(max));
                }
        }

        const numerical::Vector<N, T> center = min + (max - min) / T{2};
        const T len = (max - min).norm_stable();

        if (!is_finite(center))
        {
                error("Object center is not finite");
        }
        if (!std::isfinite(len))
        {
                error("Object length is not finite");
        }
        if (!(len > 0))
        {
                error("Object length " + to_string(len) + " is not positive");
        }

        return {center, len};
}
}
