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

/*
Tomas Akenine-Möller, Eric Haines, Naty Hoffman,
Angelo Pesce, Michal Iwanicki, Sébastien Hillaire.
Real-Time Rendering. Fourth Edition.
CRC Press, 2018.

9.3 The BRDF
Reflectance equation (9.3)
*/

/*
Matt Pharr, Wenzel Jakob, Greg Humphreys.
Physically Based Rendering. From theory to implementation. Third edition.
Elsevier, 2017.

14.5 Path tracing
*/

#include "path_tracing.h"

#include "surface_sample.h"

#include "../direct_lighting.h"
#include "../normals.h"
#include "../visibility.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/settings/instantiation.h>

#include <optional>

namespace ns::painter::integrators::path_tracing
{
namespace
{
template <typename Color, typename RandomEngine>
bool terminate(RandomEngine& engine, const int depth, Color* const beta)
{
        if (depth < 4)
        {
                return false;
        }

        const auto luminance = beta->luminance();
        if (!(luminance > 0))
        {
                return true;
        }

        using T = decltype(luminance);
        static constexpr T MIN = 0.05;
        static constexpr T MAX = 0.95;
        const T p = std::clamp(1 - luminance, MIN, MAX);
        if (std::bernoulli_distribution(p)(engine))
        {
                return true;
        }
        *beta /= 1 - p;
        return false;
}
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
std::optional<Color> path_tracing(const Scene<N, T, Color>& scene, Ray<N, T> ray, PCG& engine)
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
                if (const auto c = directly_visible_light_sources(scene, ray))
                {
                        return *c + scene.background_light();
                }
                return {};
        }

        Color color(0);

        if (const auto c = directly_visible_light_sources(scene, surface, ray))
        {
                color = *c;
        }

        Color beta(1);

        for (int depth = 0;; ++depth)
        {
                const Vector<N, T> v = -ray.dir();

                if (dot(normals.shading, v) <= 0)
                {
                        break;
                }

                if (const auto& c = surface.light_source())
                {
                        color.multiply_add(beta, *c);
                }

                if (const auto c = direct_lighting(scene, surface, v, normals, engine))
                {
                        color.multiply_add(beta, *c);
                }

                const auto sample = surface_sample(surface, v, normals, engine);
                if (!sample)
                {
                        break;
                }

                beta *= sample->beta;

                if (terminate(engine, depth, &beta))
                {
                        break;
                }

                ray = Ray<N, T>(surface.point(), sample->l);
                std::tie(surface, normals) = scene_intersect<FLAT_SHADING, N, T, Color>(scene, normals.geometric, ray);

                if (!surface)
                {
                        color.multiply_add(beta, scene.background_light());
                        break;
                }
        }

        return color;
}

#define TEMPLATE(N, T, C)                                                                                    \
        template std::optional<C> path_tracing<true, (N), T, C>(const Scene<(N), T, C>&, Ray<(N), T>, PCG&); \
        template std::optional<C> path_tracing<false, (N), T, C>(const Scene<(N), T, C>&, Ray<(N), T>, PCG&);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
