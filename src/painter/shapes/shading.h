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

namespace ns::painter
{
template <std::size_t N, typename T>
Color surface_direct_lighting(const Vector<N, T>& dir_to_light, const Vector<N, T>& normal, const Color& color)
{
        return dot(normal, dir_to_light) * color;
}

template <std::size_t N, typename T, typename RandomEngine>
SurfaceReflection<N, T> surface_reflection(const Vector<N, T>& normal, const Color& color, RandomEngine& engine)
{
        return {color, sampling::cosine_weighted_on_hemisphere(engine, normal)};
}
}
