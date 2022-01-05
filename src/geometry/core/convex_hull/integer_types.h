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

#pragma once

#include "max_values.h"

#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/type/concept.h>
#include <src/com/type/find.h>
#include <src/com/type/limit.h>

#include <sstream>

namespace ns::geometry::convex_hull
{
namespace integer_types_implementation
{
template <typename T>
std::string type_str() requires(!std::is_same_v<std::remove_cv_t<T>, mpz_class>)
{
        static_assert(Integral<T>);
        return to_string(Limits<T>::digits()) + " bits";
}

template <typename T>
std::string type_str() requires std::is_same_v<std::remove_cv_t<T>, mpz_class>
{
        return "mpz_class";
}
}

inline constexpr int CONVEX_HULL_BITS = 30;
inline constexpr int DELAUNAY_BITS = 24;

using ConvexHullSourceInteger = LeastSignedInteger<CONVEX_HULL_BITS>;
using DelaunaySourceInteger = LeastSignedInteger<DELAUNAY_BITS>;

inline constexpr ConvexHullSourceInteger MAX_CONVEX_HULL{(1ull << CONVEX_HULL_BITS) - 1};
inline constexpr DelaunaySourceInteger MAX_DELAUNAY{(1ull << DELAUNAY_BITS) - 1};

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
        namespace impl = integer_types_implementation;

        std::ostringstream oss;
        oss << "Convex hull " << space_name(N) << '\n';
        oss << "  Max: " << CONVEX_HULL_BITS << '\n';
        oss << "  Data: " << impl::type_str<ConvexHullDataType<N>>() << '\n';
        oss << "  Compute: " << impl::type_str<ConvexHullComputeType<N>>();
        return oss.str();
}

template <std::size_t N>
std::string delaunay_type_description()
{
        namespace impl = integer_types_implementation;

        std::ostringstream oss;
        oss << "Delaunay" << '\n';
        oss << "  Convex hull " << space_name(N + 1) << '\n';
        oss << "    Max: " << DELAUNAY_BITS << '\n';
        oss << "    Data: " << impl::type_str<DelaunayParaboloidDataType<N + 1>>() << '\n';
        oss << "    Compute: " << impl::type_str<DelaunayParaboloidComputeType<N + 1>>() << '\n';
        oss << "  " << space_name(N) << '\n';
        oss << "    Data: " << impl::type_str<DelaunayDataType<N>>() << '\n';
        oss << "    Compute: " << impl::type_str<DelaunayComputeType<N>>();
        return oss.str();
}
}
