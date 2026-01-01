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

#include <src/com/type/concept.h>
#include <src/geometry/core/convex_hull/integer_types.h>
#include <src/settings/instantiation.h>

#include <cstddef>
#include <type_traits>

namespace ns::geometry::core::convex_hull
{
namespace
{
template <std::size_t N>
constexpr bool CHECK_NON_CLASS = (N <= 4);

template <std::size_t N>
struct CheckDelaunay final
{
        static_assert(!CHECK_NON_CLASS<N> || !std::is_class_v<DelaunayParaboloidDataType<N>>);
        static_assert(!CHECK_NON_CLASS<N> || !std::is_class_v<DelaunayParaboloidComputeType<N>>);
        static_assert(!CHECK_NON_CLASS<N> || !std::is_class_v<DelaunayDataType<N>>);
        static_assert(!CHECK_NON_CLASS<N> || !std::is_class_v<DelaunayComputeType<N>>);
        static_assert(Integral<DelaunayParaboloidDataType<N>>);
        static_assert(Integral<DelaunayParaboloidComputeType<N>>);
        static_assert(Integral<DelaunayDataType<N>>);
        static_assert(Integral<DelaunayComputeType<N>>);
};

template <std::size_t N>
struct CheckConvexHull final
{
        static_assert(!CHECK_NON_CLASS<N> || !std::is_class_v<ConvexHullDataType<N>>);
        static_assert(!CHECK_NON_CLASS<N> || !std::is_class_v<ConvexHullComputeType<N>>);
        static_assert(Integral<ConvexHullDataType<N>>);
        static_assert(Integral<ConvexHullComputeType<N>>);
};

#define TEMPLATE_DELAUNAY(N) template struct CheckDelaunay<(N)>;
#define TEMPLATE_CONVEX_HULL(N) template struct CheckConvexHull<(N)>;

TEMPLATE_INSTANTIATION_N_2(TEMPLATE_DELAUNAY)
TEMPLATE_INSTANTIATION_N_2_A(TEMPLATE_CONVEX_HULL)
}
}
