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

/*
Matt Pharr, Wenzel Jakob, Greg Humphreys.
Physically Based Rendering. From theory to implementation. Third edition.
Elsevier, 2017.

14.5 Path tracing
*/

#include "pt.h"

#include "direct_lighting.h"

#include <src/com/random/pcg.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/painter/integrators/com/normals.h>
#include <src/painter/integrators/com/surface_sample.h>
#include <src/painter/integrators/com/visibility.h>
#include <src/painter/objects.h>
#include <src/settings/instantiation.h>

#include <cstddef>
#include <optional>
#include <random>

namespace ns::painter::integrators::pt
{
namespace
{
template <typename Color>
[[nodiscard]] bool terminate(const int depth, Color* const beta, PCG& engine)
{
        using T = Color::DataType;

        if (depth < 4)
        {
                return false;
        }

        const T luminance = beta->luminance();
        if (!(luminance > 0))
        {
                return true;
        }

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

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
[[nodiscard]] Color pt(
        PCG& engine,
        const Scene<N, T, Color>& scene,
        numerical::Ray<N, T> ray,
        SurfaceIntersection<N, T, Color> surface,
        Normals<N, T> normals,
        Color color)
{
        Color beta(1);

        for (int depth = 0;; ++depth)
        {
                const Vector<N, T> v = -ray.dir();

                if (dot(normals.shading, v) <= 0)
                {
                        break;
                }

                if (const auto& c = direct_lighting(scene, surface, v, normals, engine))
                {
                        color.multiply_add(beta, *c);
                }

                const auto& sample = surface_sample(surface, v, normals, engine);
                if (!sample)
                {
                        break;
                }

                beta *= sample->beta;

                if (terminate(depth, &beta, engine))
                {
                        break;
                }

                ray = {surface.point(), sample->l};
                std::tie(surface, normals) = scene_intersect<FLAT_SHADING, N, T, Color>(scene, normals.geometric, ray);

                if (!surface)
                {
                        break;
                }
        }

        return color;
}
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
std::optional<Color> pt(const Scene<N, T, Color>& scene, const numerical::Ray<N, T>& ray, PCG& engine)
{
        const auto [surface, normals] = [&]
        {
                static constexpr std::optional<Vector<N, T>> GEOMETRIC_NORMAL;
                return scene_intersect<FLAT_SHADING, N, T, Color>(scene, GEOMETRIC_NORMAL, ray);
        }();

        if (!surface)
        {
                return {};
        }

        const Color color = [&]
        {
                if (const auto* const light = surface.light_source())
                {
                        if (const auto& radiance = light->leave_radiance(-ray.dir()))
                        {
                                return *radiance;
                        }
                }
                return Color(0);
        }();

        return pt<FLAT_SHADING>(engine, scene, ray, surface, normals, color);
}

#define TEMPLATE(N, T, C)                                                                                            \
        template std::optional<C> pt<true, (N), T, C>(const Scene<(N), T, C>&, const numerical::Ray<(N), T>&, PCG&); \
        template std::optional<C> pt<false, (N), T, C>(const Scene<(N), T, C>&, const numerical::Ray<(N), T>&, PCG&);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
