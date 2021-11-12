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

13.10 Importance sampling
13.10.1 Multiple importance sampling
14.3.1 Estimating the direct lighting integral
*/

#pragma once

#include "visibility.h"

#include "../objects.h"

#include <src/com/error.h>
#include <src/sampling/mis.h>

namespace ns::painter
{
namespace trace_implementation
{
constexpr int MAX_DEPTH = 5;

template <std::size_t N, typename T>
struct Normals final
{
        Vector<N, T> geometric;
        Vector<N, T> shading;
        bool smooth;
};

template <std::size_t N, typename T, typename Color>
Normals<N, T> compute_normals(
        const bool smooth_normals,
        const Vector<N, T>& point,
        const Surface<N, T, Color>* const surface,
        const Vector<N, T>& v)
{
        const Vector<N, T> g_normal = surface->geometric_normal(point);
        ASSERT(g_normal.is_unit());
        const bool flip = dot(v, g_normal) < 0;
        const Vector<N, T> geometric = flip ? -g_normal : g_normal;
        if (smooth_normals)
        {
                const auto s_normal = surface->shading_normal(point);
                if (s_normal)
                {
                        ASSERT(s_normal->is_unit());
                        return {.geometric = geometric, .shading = (flip ? -*s_normal : *s_normal), .smooth = true};
                }
        }
        return {.geometric = geometric, .shading = geometric, .smooth = false};
}

template <std::size_t N, typename T, typename Color>
void add_surface(const Surface<N, T, Color>& surface, Color* const color_sum)
{
        const std::optional<Color> surface_light_source = surface.light_source();
        if (!surface_light_source)
        {
                return;
        }
        *color_sum = *surface_light_source;
}

template <typename T>
T mis_heuristic(const int f_n, const T f_pdf, const int g_n, const T g_pdf)
{
        return sampling::mis::power_heuristic(f_n, f_pdf, g_n, g_pdf);
}

template <std::size_t N, typename T, typename Color>
void sample_light_source_with_mis(
        const LightSource<N, T, Color>& light,
        const Scene<N, T, Color>& scene,
        const Surface<N, T, Color>& surface,
        const Vector<N, T>& point,
        const Vector<N, T>& v,
        const Normals<N, T>& normals,
        Color* const color_sum,
        RandomEngine<T>& engine)
{
        const Vector<N, T>& n = normals.shading;

        const LightSourceSample<N, T, Color> sample = light.sample(engine, point);
        if (sample.pdf <= 0 || sample.radiance.is_black())
        {
                return;
        }

        const Vector<N, T>& l = sample.l;
        ASSERT(l.is_unit());

        const T n_l = dot(n, l);
        if (n_l <= 0)
        {
                return;
        }

        if (occluded(scene, normals.geometric, normals.smooth, Ray<N, T>(point, l), sample.distance))
        {
                return;
        }

        const Color brdf = surface.brdf(point, n, v, l);
        if (light.is_delta())
        {
                *color_sum += brdf * sample.radiance * (n_l / sample.pdf);
        }
        else
        {
                const T pdf = surface.pdf(point, n, v, l);
                const T weight = mis_heuristic(1, sample.pdf, 1, pdf);
                *color_sum += brdf * sample.radiance * (weight * n_l / sample.pdf);
        }
}

template <std::size_t N, typename T, typename Color>
void sample_brdf_with_mis(
        const LightSource<N, T, Color>& light,
        const Scene<N, T, Color>& scene,
        const Surface<N, T, Color>& surface,
        const Vector<N, T>& point,
        const Vector<N, T>& v,
        const Normals<N, T>& normals,
        Color* const color_sum,
        RandomEngine<T>& engine)
{
        if (light.is_delta())
        {
                return;
        }

        const Vector<N, T>& n = normals.shading;

        const Sample<N, T, Color> sample = surface.sample_brdf(engine, point, n, v);
        if (sample.pdf <= 0 || sample.brdf.is_black())
        {
                return;
        }

        const Vector<N, T>& l = sample.l;
        ASSERT(l.is_unit());

        const T n_l = dot(n, l);
        if (n_l <= 0)
        {
                return;
        }

        LightSourceInfo<T, Color> light_info = light.info(point, l);
        if (light_info.pdf <= 0 || light_info.radiance.is_black())
        {
                return;
        }

        if (occluded(scene, normals.geometric, normals.smooth, Ray<N, T>(point, l), light_info.distance))
        {
                return;
        }

        if (sample.specular)
        {
                *color_sum += sample.brdf * light_info.radiance * (n_l / sample.pdf);
        }
        else
        {
                const T weight = mis_heuristic(1, sample.pdf, 1, light_info.pdf);
                *color_sum += sample.brdf * light_info.radiance * (weight * n_l / sample.pdf);
        }
}

template <std::size_t N, typename T, typename Color>
void add_light_sources(
        const Scene<N, T, Color>& scene,
        const Surface<N, T, Color>& surface,
        const Vector<N, T>& point,
        const Vector<N, T>& v,
        const Normals<N, T>& normals,
        Color* const color_sum,
        RandomEngine<T>& engine)
{
        for (const LightSource<N, T, Color>* const light : scene.light_sources())
        {
                sample_light_source_with_mis(*light, scene, surface, point, v, normals, color_sum, engine);
                sample_brdf_with_mis(*light, scene, surface, point, v, normals, color_sum, engine);
        }
}

template <std::size_t N, typename T, typename Color>
void add_light_sources(
        const Scene<N, T, Color>& scene,
        const T& distance,
        const Surface<N, T, Color>* const surface,
        const Ray<N, T>& ray,
        Color* const color_sum)
{
        for (const LightSource<N, T, Color>* const light : scene.light_sources())
        {
                LightSourceInfo<T, Color> light_info = light->info(ray.org(), ray.dir());
                if (light_info.pdf <= 0 || light_info.radiance.is_black())
                {
                        continue;
                }
                if (surface_before_distance(distance, surface, light_info.distance))
                {
                        continue;
                }
                *color_sum += light_info.radiance;
        }
}

template <std::size_t N, typename T, typename Color>
std::optional<Color> trace_path(
        const Scene<N, T, Color>& scene,
        bool smooth_normals,
        const Ray<N, T>& ray,
        int depth,
        RandomEngine<T>& engine);

template <std::size_t N, typename T, typename Color>
void add_reflected(
        const Scene<N, T, Color>& scene,
        const bool smooth_normals,
        const int depth,
        const Surface<N, T, Color>& surface,
        const Vector<N, T>& point,
        const Vector<N, T>& v,
        const Normals<N, T>& normals,
        Color* const color_sum,
        RandomEngine<T>& engine)
{
        const Vector<N, T>& n = normals.shading;

        const Sample<N, T, Color> sample = surface.sample_brdf(engine, point, n, v);

        if (sample.pdf <= 0 || sample.brdf.is_black())
        {
                return;
        }

        const Vector<N, T>& l = sample.l;
        ASSERT(l.is_unit());

        if (dot(l, normals.geometric) <= 0)
        {
                return;
        }

        const T n_l = dot(n, l);
        if (n_l <= 0)
        {
                return;
        }

        const Color radiance = *trace_path<N, T, Color>(scene, smooth_normals, Ray<N, T>(point, l), depth + 1, engine);
        *color_sum += sample.brdf * radiance * (n_l / sample.pdf);
}

template <std::size_t N, typename T, typename Color>
std::optional<Color> trace_path(
        const Scene<N, T, Color>& scene,
        const bool smooth_normals,
        const Ray<N, T>& ray,
        const int depth,
        RandomEngine<T>& engine)
{
        const auto [distance, surface] = scene.intersect(ray);

        if (depth > 0 && !surface)
        {
                return scene.background_light();
        }

        Color color_sum(0);

        if (depth == 0)
        {
                add_light_sources(scene, distance, surface, ray, &color_sum);
                if (!surface)
                {
                        if (color_sum.is_black())
                        {
                                return std::nullopt;
                        }
                        color_sum += scene.background_light();
                        return color_sum;
                }
        }

        const Vector<N, T> v = -ray.dir();
        const Vector<N, T> point = surface->point(ray, distance);
        const Normals<N, T> normals = compute_normals(smooth_normals, point, surface, v);
        const Vector<N, T>& n = normals.shading;

        if (dot(n, v) <= 0)
        {
                return color_sum;
        }

        add_surface(*surface, &color_sum);

        add_light_sources(scene, *surface, point, v, normals, &color_sum, engine);

        if (depth >= MAX_DEPTH)
        {
                return color_sum;
        }

        add_reflected(scene, smooth_normals, depth, *surface, point, v, normals, &color_sum, engine);

        return color_sum;
}
}

template <std::size_t N, typename T, typename Color>
std::optional<Color> trace_path(
        const Scene<N, T, Color>& scene,
        const bool smooth_normals,
        const Ray<N, T>& ray,
        RandomEngine<T>& random_engine)
{
        return trace_implementation::trace_path<N, T, Color>(scene, smooth_normals, ray, 0 /*depth*/, random_engine);
}
}
