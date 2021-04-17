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
std::optional<Color> trace_path(
        const Scene<N, T>& scene,
        const bool smooth_normals,
        const Ray<N, T>& ray,
        const int depth,
        RandomEngine<T>& random_engine)
{
        const std::optional<Intersection<N, T>> intersection = scene.intersect(ray);
        if (!intersection)
        {
                if (depth > 0)
                {
                        return scene.background_light();
                }
                return std::nullopt;
        }

        const Vector<N, T> v = -ray.dir();
        const Vector<N, T>& point = intersection->point;

        const auto [use_smooth_normal, geometric_normal, n] = [&]()
        {
                const Vector<N, T> g_normal = intersection->surface->geometric_normal(point, intersection->data);
                ASSERT(g_normal.is_unit());

                const std::optional<Vector<N, T>> s_normal =
                        intersection->surface->shading_normal(point, intersection->data);

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

                return std::make_tuple(smooth, geometric, shading);
        }();

        if (dot(n, v) <= 0)
        {
                return Color(0);
        }

        Color color_sum(0);

        if (const std::optional<Color> surface_light_source =
                    intersection->surface->light_source(point, intersection->data);
            surface_light_source)
        {
                color_sum = *surface_light_source;
        }

        for (const LightSource<N, T>* const light_source : scene.light_sources())
        {
                const LightSourceSample<N, T> sample = light_source->sample(point);

                if (sample.color.is_black() || sample.pdf <= 0)
                {
                        continue;
                }

                const Vector<N, T>& l = sample.l;
                ASSERT(l.is_unit());

                if (dot(n, l) <= 0)
                {
                        continue;
                }

                if (occluded(scene, geometric_normal, use_smooth_normal, Ray<N, T>(point, l), sample.distance))
                {
                        continue;
                }

                const Color direct_lighting = intersection->surface->shade(point, intersection->data, n, v, l);
                color_sum += direct_lighting * sample.color / sample.pdf;
        }

        if (depth >= MAX_DEPTH)
        {
                return color_sum;
        }

        {
                const ShadeSample<N, T> reflection = intersection->surface->sample_shade(
                        point, intersection->data, random_engine, ShadeType::Reflection, n, v);
                if (!reflection.color.is_black())
                {
                        const Vector<N, T>& l = reflection.l;
                        ASSERT(l.is_unit());
                        if (dot(l, geometric_normal) > 0)
                        {
                                const Color reflected = *trace_path(
                                        scene, smooth_normals, Ray<N, T>(point, l), depth + 1, random_engine);
                                color_sum += reflection.color * reflected;
                        }
                }
        }
        {
                const ShadeSample<N, T> transmission = intersection->surface->sample_shade(
                        point, intersection->data, random_engine, ShadeType::Transmission, n, v);
                if (!transmission.color.is_black())
                {
                        const Vector<N, T>& l = transmission.l;
                        ASSERT(l.is_unit());
                        if (dot(l, geometric_normal) < 0)
                        {
                                const Color transmitted = *trace_path(
                                        scene, smooth_normals, Ray<N, T>(point, l), depth + 1, random_engine);
                                color_sum += transmission.color * transmitted;
                        }
                }
        }

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
