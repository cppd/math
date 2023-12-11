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

/*
Tomas Akenine-Möller, Eric Haines, Naty Hoffman,
Angelo Pesce, Michal Iwanicki, Sébastien Hillaire.
Real-Time Rendering. Fourth Edition.
CRC Press, 2018.

9.8.2 Multiple-Bounce Surface Reflection
*/

#pragma once

#include "f1_albedo.h"
#include "fresnel.h"

#include <src/geometry/shapes/sphere_integral.h>

#include <cstddef>

namespace ns::shading::ggx
{
namespace multiple_bounce_implementation
{
template <std::size_t N, typename T, typename Color>
Color multiple_bounce_surface_reflection(const Color& f0, const T rs_f1, const T rs_f1_l, const T rs_f1_v)
{
        static constexpr Color WHITE = Color(1);
        static constexpr T K = geometry::shapes::SPHERE_INTEGRATE_COSINE_FACTOR_OVER_HEMISPHERE<N, long double>;

        const Color f = fresnel_cosine_weighted_average<N>(f0);

        const Color d = f * (rs_f1 * (1 - rs_f1_l) * (1 - rs_f1_v) / (K * (1 - rs_f1)));

        return d / (WHITE - f * (1 - rs_f1));
}
}

template <std::size_t N, typename T, typename Color>
Color multiple_bounce_surface_reflection(const Color& f0, const T roughness, const T n_l, const T n_v)
{
        namespace impl = multiple_bounce_implementation;

        const T rs_f1 = f1_albedo_cosine_weighted_average<N>(roughness);
        const T rs_f1_l = f1_albedo<N>(roughness, n_l);
        const T rs_f1_v = f1_albedo<N>(roughness, n_v);

        return impl::multiple_bounce_surface_reflection<N>(f0, rs_f1, rs_f1_l, rs_f1_v);
}
}
