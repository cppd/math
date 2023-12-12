/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "hyperplane_parallelotope.h"

#include "intersection/hyperplane_parallelotope.h"

#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/type/name.h>
#include <src/settings/instantiation.h>

#include <cmath>
#include <cstddef>

namespace ns::geometry::spatial
{
template <std::size_t N, typename T>
T HyperplaneParallelotope<N, T>::intersection_cost()
{
        static const T cost = []
        {
                const double p =
                        spatial::intersection::hyperplane_parallelotope::compute_intersections_per_second<N, T>();
                LOG("HyperplaneParallelotope<" + to_string(N) + ", " + type_name<T>()
                    + "> intersections per second = " + to_string_digit_groups(std::llround(p)));
                return 1 / p;
        }();
        return cost;
}

#define TEMPLATE(N, T) template T HyperplaneParallelotope<(N), T>::intersection_cost();

TEMPLATE_INSTANTIATION_N_T_2(TEMPLATE)
}
