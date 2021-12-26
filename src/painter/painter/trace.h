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

#include "../objects.h"

#include <src/com/error.h>

#include <optional>
#include <random>
#include <tuple>

namespace ns::painter
{
namespace trace_implementation
{
template <std::size_t N, typename T, typename Color>
Normals<N, T> compute_normals(
        const bool smooth_normals,
        const SurfacePoint<N, T, Color>& surface,
        const Vector<N, T>& v)
{
        const Vector<N, T> g_normal = surface.geometric_normal();
        ASSERT(g_normal.is_unit());
        const bool flip = dot(v, g_normal) < 0;
        const Vector<N, T> geometric = flip ? -g_normal : g_normal;
        if (smooth_normals)
        {
                const auto s_normal = surface.shading_normal();
                if (s_normal)
                {
                        ASSERT(s_normal->is_unit());
                        return {.geometric = geometric, .shading = (flip ? -*s_normal : *s_normal), .smooth = true};
                }
        }
        return {.geometric = geometric, .shading = geometric, .smooth = false};
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
std::optional<std::tuple<Color, Vector<N, T>>> sample_brdf(
        const SurfacePoint<N, T, Color>& surface,
        const Vector<N, T>& v,
        const Normals<N, T>& normals,
        RandomEngine& engine)
{
        const Vector<N, T>& n = normals.shading;

        const Sample<N, T, Color> sample = surface.sample_brdf(engine, n, v);

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

        std::optional<std::tuple<Color, Vector<N, T>>> res;
        res.emplace(sample.brdf * (n_l / sample.pdf), l);
        return res;
}

template <typename Color, typename RandomEngine>
bool terminate(RandomEngine& engine, const int depth, Color* const beta)
{
        if (depth < 4)
        {
                return false;
        }

        const auto luminance = beta->luminance();
        if (luminance > 0)
        {
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
        return true;
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
std::optional<Color> trace_path(
        const Scene<N, T, Color>& scene,
        const bool smooth_normals,
        Ray<N, T> ray,
        RandomEngine& engine)
{
        SurfacePoint surface = [&]
        {
                static constexpr std::optional<Vector<N, T>> GEOMETRIC_NORMAL;
                return scene.intersect(GEOMETRIC_NORMAL, ray);
        }();

        if (!surface)
        {
                if (const auto c = directly_visible_light_sources(scene, surface, ray))
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
                const Normals<N, T> normals = compute_normals(smooth_normals, surface, v);

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

                const auto sample = sample_brdf(surface, v, normals, engine);
                if (!sample)
                {
                        break;
                }

                beta *= std::get<0>(*sample);

                if (terminate(engine, depth, &beta))
                {
                        break;
                }

                ray = Ray<N, T>(surface.point(), std::get<1>(*sample));
                surface = scene.intersect(normals.geometric, ray);
                if (!surface)
                {
                        color.multiply_add(beta, scene.background_light());
                        break;
                }
        }

        return color;
}
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
std::optional<Color> trace_path(
        const Scene<N, T, Color>& scene,
        const bool smooth_normals,
        const Ray<N, T>& ray,
        RandomEngine& engine)
{
        return trace_implementation::trace_path<N, T, Color>(scene, smooth_normals, ray, engine);
}
}
