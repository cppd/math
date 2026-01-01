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

/*
Samuel R. Buss.
3D Computer Graphics. A Mathematical Introduction with OpenGL.
Cambridge University Press, 2003.

The OpenGL® Shading Language.
*/

#pragma once

#include "vector.h"

#include <src/com/exponent.h>

#include <cstddef>
#include <optional>

namespace ns::numerical
{
template <std::size_t N, typename T>
[[nodiscard]] Vector<N, T> reflect(const Vector<N, T>& v, const Vector<N, T>& normal)
{
        static_assert(std::is_floating_point_v<T>);

        return v - (2 * dot(v, normal)) * normal;
}

template <std::size_t N, typename T>
[[nodiscard]] Vector<N, T> reflect_vn(const Vector<N, T>& v, const Vector<N, T>& normal)
{
        static_assert(std::is_floating_point_v<T>);

        return (2 * dot(v, normal)) * normal - v;
}

// The OpenGL® Shading Language, Geometric Functions, Description.
template <std::size_t N, typename T>
[[nodiscard]] std::optional<Vector<N, T>> refract(const Vector<N, T>& v, const Vector<N, T>& normal, const T eta)
{
        static_assert(std::is_floating_point_v<T>);

        const T cos1 = dot(normal, v);
        // sin2 = eta * sin1
        const T cos2_squared = 1 - square(eta) * (1 - square(cos1));
        if (cos2_squared > 0)
        {
                return v * eta - normal * (eta * cos1 + std::sqrt(cos2_squared));
                // return eta * (v  - normal * dot(v, normal)) - normal * std::sqrt(cos2_squared);
        }
        return std::nullopt;
}

// 3D Computer Graphics. A Mathematical Introduction with OpenGL.
template <std::size_t N, typename T>
[[nodiscard]] std::optional<Vector<N, T>> refract2(const Vector<N, T>& v, const Vector<N, T>& normal, const T eta)
{
        static_assert(std::is_floating_point_v<T>);

        const Vector<N, T> t_lat = eta * (v - normal * dot(v, normal));
        const T sin_square = dot(t_lat, t_lat);
        if (sin_square < 1)
        {
                return t_lat - normal * std::sqrt(1 - sin_square);
        }
        return std::nullopt;
}
}
