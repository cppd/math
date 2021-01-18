/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "../objects.h"

#include <src/color/color.h>
#include <src/numerical/vec.h>
#include <src/sampling/sphere_cosine.h>
#include <src/sampling/sphere_surface.h>

namespace ns::painter
{
template <std::size_t N, typename T>
class Shading
{
        static constexpr T DIFFUSE_REFLECTANCE = T(1) / sampling::sphere_integrate_cosine_factor_over_hemisphere(N);

public:
        Color lighting(
                T /*metalness*/,
                T /*roughness*/,
                const Color& color,
                const Vector<N, T>& n,
                const Vector<N, T>& /*v*/,
                const Vector<N, T>& l) const
        {
                return DIFFUSE_REFLECTANCE * dot(n, l) * color;
        }

        template <typename RandomEngine>
        SurfaceReflection<N, T> reflection(
                RandomEngine& random_engine,
                T /*metalness*/,
                T /*roughness*/,
                const Color& color,
                const Vector<N, T>& n,
                const Vector<N, T>& /*v*/) const
        {
                Vector<N, T> v = sampling::cosine_weighted_on_hemisphere(random_engine, n);
                return {DIFFUSE_REFLECTANCE * color, v};
        }
};
}
