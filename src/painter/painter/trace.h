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
*/

#pragma once

#include "visibility.h"

#include "../objects.h"

#include <src/com/error.h>

namespace ns::painter
{
namespace trace_implementation
{
constexpr int MAX_DEPTH = 5;

static_assert(std::is_floating_point_v<Color::DataType>);

template <std::size_t N, typename T>
struct Normals final
{
        Vector<N, T> geometric;
        Vector<N, T> shading;
        bool smooth;
};

template <std::size_t N, typename T>
Normals<N, T> compute_normals(const bool smooth_normals, const Surface<N, T>* const surface, const Vector<N, T>& v)
{
        const Vector<N, T> g_normal = surface->geometric_normal();
        ASSERT(g_normal.is_unit());

        const std::optional<Vector<N, T>> s_normal = surface->shading_normal();

        const bool smooth = smooth_normals && s_normal.has_value();

        // Определять по реальной нормали, так как видимая нормаль может
        // показать, что пересечение находится с другой стороны объекта.
        const bool flip = dot(v, g_normal) < 0;

        const Vector<N, T> geometric = flip ? -g_normal : g_normal;
        const Vector<N, T> shading = [&]
        {
                if (smooth)
                {
                        ASSERT(s_normal->is_unit());
                        return flip ? -*s_normal : *s_normal;
                }
                return geometric;
        }();

        return {.geometric = geometric, .shading = shading, .smooth = smooth};
}

template <std::size_t N, typename T>
std::optional<Color> trace_path(
        const Scene<N, T>& scene,
        const bool smooth_normals,
        const Ray<N, T>& ray,
        const int depth,
        RandomEngine<T>& engine)
{
        const Surface<N, T>* const surface = scene.intersect(ray);
        if (!surface)
        {
                if (depth > 0)
                {
                        return scene.background_light();
                }
                return std::nullopt;
        }

        const Vector<N, T> v = -ray.dir();
        const Vector<N, T>& point = surface->point();
        const Normals<N, T> normals = compute_normals(smooth_normals, surface, v);
        const Vector<N, T>& n = normals.shading;

        if (dot(n, v) <= 0)
        {
                return Color(0);
        }

        Color color_sum(0);

        [&]
        {
                const std::optional<Color> surface_light_source = surface->light_source();
                if (!surface_light_source)
                {
                        return;
                }

                color_sum = *surface_light_source;
        }();

        for (const LightSource<N, T>* const light_source : scene.light_sources())
        {
                const LightSourceSample<N, T> sample = light_source->sample(point);

                if (sample.L.is_black() || sample.pdf <= 0)
                {
                        continue;
                }

                const Vector<N, T>& l = sample.l;
                ASSERT(l.is_unit());

                const T n_l = dot(n, l);
                if (n_l <= 0)
                {
                        continue;
                }

                if (occluded(scene, normals.geometric, normals.smooth, Ray<N, T>(point, l), sample.distance))
                {
                        continue;
                }

                const Color brdf = surface->brdf(n, v, l);
                color_sum += brdf * sample.L * (n_l / sample.pdf);
        }

        if (depth >= MAX_DEPTH)
        {
                return color_sum;
        }

        [&]
        {
                const BrdfSample<N, T> sample = surface->sample_brdf(engine, n, v);

                if (sample.brdf.is_black() || sample.pdf <= 0)
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

                const Color L = *trace_path(scene, smooth_normals, Ray<N, T>(point, l), depth + 1, engine);
                color_sum += sample.brdf * L * (n_l / sample.pdf);
        }();

        return color_sum;
}
}

template <std::size_t N, typename T>
std::optional<Color> trace_path(
        const Scene<N, T>& scene,
        const bool smooth_normals,
        const Ray<N, T>& ray,
        RandomEngine<T>& random_engine)
{
        return trace_implementation::trace_path(scene, smooth_normals, ray, 0 /*depth*/, random_engine);
}
}
