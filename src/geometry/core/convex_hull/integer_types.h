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

#include "max_values.h"

#include <src/com/names.h>
#include <src/com/type/find.h>
#include <src/com/type/name.h>
#include <src/com/type/name_gmp.h>

#include <cstddef>
#include <sstream>
#include <string>

namespace ns::geometry::core::convex_hull
{
inline constexpr int CONVEX_HULL_BITS = 30;
inline constexpr int DELAUNAY_BITS = 24;

//

template <std::size_t N>
using ConvexHullComputeType = LeastSignedInteger<MAX_DETERMINANT<N, CONVEX_HULL_BITS>>;

template <std::size_t N>
using ConvexHullDataType = LeastSignedInteger<CONVEX_HULL_BITS>;

//

template <std::size_t N>
using DelaunayParaboloidComputeType = LeastSignedInteger<MAX_DETERMINANT_PARABOLOID<N, DELAUNAY_BITS>>;

template <std::size_t N>
using DelaunayParaboloidDataType = LeastSignedInteger<MAX_PARABOLOID<N, DELAUNAY_BITS>>;

template <std::size_t N>
using DelaunayComputeType = LeastSignedInteger<MAX_DETERMINANT<N, DELAUNAY_BITS>>;

template <std::size_t N>
using DelaunayDataType = LeastSignedInteger<DELAUNAY_BITS>;

//

template <std::size_t N>
std::string convex_hull_type_description()
{
        std::ostringstream oss;
        oss << "Convex hull " << space_name(N) << '\n';
        oss << "  Max: " << CONVEX_HULL_BITS << '\n';
        oss << "  Data: " << type_bit_name<ConvexHullDataType<N>>() << '\n';
        oss << "  Compute: " << type_bit_name<ConvexHullComputeType<N>>();
        return oss.str();
}

template <std::size_t N>
std::string delaunay_type_description()
{
        std::ostringstream oss;
        oss << "Delaunay" << '\n';
        oss << "  Convex hull " << space_name(N + 1) << '\n';
        oss << "    Max: " << DELAUNAY_BITS << '\n';
        oss << "    Data: " << type_bit_name<DelaunayParaboloidDataType<N + 1>>() << '\n';
        oss << "    Compute: " << type_bit_name<DelaunayParaboloidComputeType<N + 1>>() << '\n';
        oss << "  " << space_name(N) << '\n';
        oss << "    Data: " << type_bit_name<DelaunayDataType<N>>() << '\n';
        oss << "    Compute: " << type_bit_name<DelaunayComputeType<N>>();
        return oss.str();
}
}
