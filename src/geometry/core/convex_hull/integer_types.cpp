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

#include "integer_types.h"

#include <src/com/type/concept.h>

namespace ns::geometry::convex_hull
{
namespace
{
template <std::size_t N, bool CHECK_NON_CLASS>
struct Check final
{
        static_assert(!CHECK_NON_CLASS || !std::is_class_v<ConvexHullDataType<N>>);
        static_assert(!CHECK_NON_CLASS || !std::is_class_v<ConvexHullComputeType<N>>);
        static_assert(!CHECK_NON_CLASS || !std::is_class_v<DelaunayParaboloidDataType<N>>);
        static_assert(!CHECK_NON_CLASS || !std::is_class_v<DelaunayParaboloidComputeType<N>>);
        static_assert(!CHECK_NON_CLASS || !std::is_class_v<DelaunayDataType<N>>);
        static_assert(!CHECK_NON_CLASS || !std::is_class_v<DelaunayComputeType<N>>);
        static_assert(Integral<ConvexHullDataType<N>>);
        static_assert(Integral<ConvexHullComputeType<N>>);
        static_assert(Integral<DelaunayParaboloidDataType<N>>);
        static_assert(Integral<DelaunayParaboloidComputeType<N>>);
        static_assert(Integral<DelaunayDataType<N>>);
        static_assert(Integral<DelaunayComputeType<N>>);
};

template struct Check<2, true>;
template struct Check<3, true>;
template struct Check<4, true>;
template struct Check<5, false>;
template struct Check<6, false>;
}
}
