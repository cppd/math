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
                        return scene.background_light_source_color();
                }
                return std::nullopt;
        }

        const Vector<N, T> v = -ray.dir();
        const Vector<N, T>& point = intersection->point;
        const SurfaceProperties surface_properties = intersection->surface->properties(point, intersection->data);
        ASSERT(surface_properties.geometric_normal.is_unit());

        const bool use_smooth_normal = smooth_normals && surface_properties.shading_normal.has_value();

        // Определять по реальной нормали, так как видимая нормаль может
        // показать, что пересечение находится с другой стороны объекта.
        const bool flip_normals = dot(v, surface_properties.geometric_normal) < 0;

        const Vector<N, T> geometric_normal =
                flip_normals ? -surface_properties.geometric_normal : surface_properties.geometric_normal;
        const Vector<N, T> n = [&]
        {
                if (use_smooth_normal)
                {
                        ASSERT(surface_properties.shading_normal->is_unit());
                        return flip_normals ? -*surface_properties.shading_normal : *surface_properties.shading_normal;
                }
                return geometric_normal;
        }();

        if (dot(n, v) <= 0)
        {
                return Color(0);
        }

        Color color_sum(0);

        if (surface_properties.light_source_color)
        {
                color_sum = *surface_properties.light_source_color;
        }

        for (const LightSource<N, T>* light_source : scene.light_sources())
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
                        random_engine, point, intersection->data, ShadeType::Reflection, n, v);
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
                        random_engine, point, intersection->data, ShadeType::Transmission, n, v);
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
