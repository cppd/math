/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "bounding_box.h"

#include "intersection/bounding_box.h"

#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/type/name.h>

#include <cmath>

namespace ns::geometry
{
template <std::size_t N, typename T>
T BoundingBox<N, T>::intersection_cost()
{
        static const T cost = []
        {
                const double p = spatial::intersection::bounding_box::compute_intersections_per_second<N, T>();
                LOG("BoundingBox<" + to_string(N) + ", " + type_name<T>()
                    + "> #1 intersections per second = " + to_string_digit_groups(std::llround(p)));
                return 1 / p;
        }();
        return cost;
}

template <std::size_t N, typename T>
T BoundingBox<N, T>::intersection_r_cost()
{
        static const T cost = []
        {
                const double p = spatial::intersection::bounding_box::compute_intersections_r_per_second<N, T>();
                LOG("BoundingBox<" + to_string(N) + ", " + type_name<T>()
                    + "> #2 intersections per second = " + to_string_digit_groups(std::llround(p)));
                return 1 / p;
        }();
        return cost;
}

#define FUNCTION_INSTANTIATION_N_T(N, T)                     \
        template T BoundingBox<(N), T>::intersection_cost(); \
        template T BoundingBox<(N), T>::intersection_r_cost();

#define FUNCTION_INSTANTIATION_N(N)            \
        FUNCTION_INSTANTIATION_N_T((N), float) \
        FUNCTION_INSTANTIATION_N_T((N), double)

FUNCTION_INSTANTIATION_N(2)
FUNCTION_INSTANTIATION_N(3)
FUNCTION_INSTANTIATION_N(4)
FUNCTION_INSTANTIATION_N(5)
FUNCTION_INSTANTIATION_N(6)
}
