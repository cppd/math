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

#include "sphere_area.h"

#include <src/com/exponent.h>

#include <type_traits>

namespace ns::geometry::shapes
{
template <unsigned N, typename T>
inline constexpr T BALL_VOLUME = []
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        return SPHERE_AREA<N, long double> / N;
}();

template <unsigned N, typename T>
constexpr T ball_volume(const std::type_identity_t<T> radius)
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        return BALL_VOLUME<N, T> * power<N>(radius);
}
}
