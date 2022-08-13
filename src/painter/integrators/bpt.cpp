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

#include "bpt.h"

#include "normals.h"
#include "surface_sample.h"
#include "visibility.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/settings/instantiation.h>

namespace ns::painter::integrators
{
namespace
{
template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void walk(const int max_depth, const Scene<N, T, Color>& scene, Ray<N, T> ray, PCG& engine)
{
        SurfaceIntersection<N, T, Color> surface;
        Normals<N, T> normals;

        std::tie(surface, normals) = [&]
        {
                static constexpr std::optional<Vector<N, T>> GEOMETRIC_NORMAL;
                return scene_intersect<FLAT_SHADING, N, T, Color>(scene, GEOMETRIC_NORMAL, ray);
        }();

        if (!surface)
        {
                return;
        }

        Color beta(1);

        for (int depth = 0; depth < max_depth; ++depth)
        {
                const Vector<N, T> v = -ray.dir();

                if (dot(normals.shading, v) <= 0)
                {
                        break;
                }

                const auto sample = surface_sample(surface, v, normals, engine);
                if (!sample)
                {
                        break;
                }

                beta *= sample->beta;

                ray = Ray<N, T>(surface.point(), sample->l);
                std::tie(surface, normals) = scene_intersect<FLAT_SHADING, N, T, Color>(scene, normals.geometric, ray);

                if (!surface)
                {
                        break;
                }
        }
}
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
std::optional<Color> bpt(
        const Scene<N, T, Color>& scene,
        Ray<N, T> ray,
        PCG& engine,
        LightDistribution<T>& /*light_distribution*/)
{
        walk<FLAT_SHADING>(1, scene, ray, engine);
        return {};
}

#define TEMPLATE(N, T, C)                                                           \
        template std::optional<C> bpt<true, (N), T, C>(                             \
                const Scene<(N), T, C>&, Ray<(N), T>, PCG&, LightDistribution<T>&); \
        template std::optional<C> bpt<false, (N), T, C>(                            \
                const Scene<(N), T, C>&, Ray<(N), T>, PCG&, LightDistribution<T>&);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
