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

#pragma once

#include "direct_lighting.h"
#include "normals.h"
#include "visibility.h"

#include "../objects.h"

#include <src/com/error.h>

#include <optional>
#include <random>

namespace ns::painter
{
namespace trace_implementation
{
template <std::size_t N, typename T, typename Color>
struct BrdfSample final
{
        Color beta;
        Vector<N, T> l;
};

template <std::size_t N, typename T, typename Color, typename RandomEngine>
std::optional<BrdfSample<N, T, Color>> sample_surface(
        const SurfaceIntersection<N, T, Color>& surface,
        const Vector<N, T>& v,
        const Normals<N, T>& normals,
        RandomEngine& engine)
{
        const Vector<N, T>& n = normals.shading;

        const SurfaceSample<N, T, Color> sample = surface.sample(engine, n, v);

        if (sample.pdf <= 0 || sample.brdf.is_black())
        {
                return {};
        }

        const Vector<N, T>& l = sample.l;
        ASSERT(l.is_unit());

        if (dot(l, normals.geometric) <= 0)
        {
                return {};
        }

        const T n_l = dot(n, l);
        if (n_l <= 0)
        {
                return {};
        }

        return BrdfSample<N, T, Color>{.beta = sample.brdf * (n_l / sample.pdf), .l = l};
}

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

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color, typename RandomEngine>
std::optional<Color> trace_path(const Scene<N, T, Color>& scene, Ray<N, T> ray, RandomEngine& engine)
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

                const auto sample = sample_surface(surface, v, normals, engine);
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
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color, typename RandomEngine>
std::optional<Color> trace_path(const Scene<N, T, Color>& scene, const Ray<N, T>& ray, RandomEngine& engine)
{
        return trace_implementation::trace_path<FLAT_SHADING>(scene, ray, engine);
}
}
