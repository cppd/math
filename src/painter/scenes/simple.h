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

#pragma once

#include "storage_scene.h"

#include "../lights/ball_light.h"
#include "../objects.h"
#include "../painter/pixel_filter.h"
#include "../projectors/parallel_projector.h"
#include "../shapes/shape.h"

#include <src/com/error.h>
#include <src/com/print.h>

#include <algorithm>
#include <cmath>
#include <memory>

namespace ns::painter
{
namespace simple_scene_implementation
{
template <std::size_t N, typename T>
std::unique_ptr<const Projector<N, T>> create_projector(
        const geometry::BoundingBox<N, T>& bounding_box,
        const int max_screen_size)
{
        static constexpr int BORDER_SIZE = PixelFilter<N, T>::integer_radius();

        if (max_screen_size <= 2 * BORDER_SIZE)
        {
                error("Maximum screen size (" + to_string(max_screen_size) + ") must be greater than or equal to "
                      + to_string(1 + 2 * BORDER_SIZE));
        }

        const int max_object_size = max_screen_size - 2 * BORDER_SIZE;

        const Vector<N, T> size = bounding_box.diagonal();
        const Vector<N, T> center = bounding_box.center();

        const T max_size = [&]
        {
                static_assert(N >= 2);
                T res = 0;
                // excluding camera direction N - 1
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        if (!(size[i] > 0))
                        {
                                error("Object projection size " + to_string(size[i]) + " is not positive");
                        }
                        res = std::max(size[i], res);
                }
                return res;
        }();

        const std::array<int, N - 1> screen_size = [&]
        {
                std::array<int, N - 1> res;
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        const int size_in_pixels = std::ceil((size[i] / max_size) * max_object_size);
                        ASSERT(size_in_pixels <= max_object_size);
                        res[i] = std::max(1, size_in_pixels) + 2 * BORDER_SIZE;
                }
                return res;
        }();

        const Vector<N, T> camera_position = [&]
        {
                Vector<N, T> res(center);
                res[N - 1] = bounding_box.max()[N - 1] + size.norm();
                return res;
        }();

        const Vector<N, T> camera_direction = []
        {
                Vector<N, T> res(0);
                res[N - 1] = -1;
                return res;
        }();

        const std::array<Vector<N, T>, N - 1> screen_axes = []
        {
                std::array<Vector<N, T>, N - 1> res;
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        for (unsigned n = 0; n < N; ++n)
                        {
                                res[i][n] = (i != n) ? 0 : 1;
                        }
                }
                return res;
        }();

        const T units_per_pixel = max_size / max_object_size;

        return std::make_unique<const ParallelProjector<N, T>>(
                camera_position, camera_direction, screen_axes, units_per_pixel, screen_size);
}

template <std::size_t N, typename T, typename Color>
void create_light_sources(
        const geometry::BoundingBox<N, T>& bounding_box,
        const Color& color,
        std::vector<std::unique_ptr<const LightSource<N, T, Color>>>* const light_sources)
{
        const Vector<N, T> box_diagonal = bounding_box.diagonal();
        const Vector<N, T> center = bounding_box.min() + box_diagonal / T{2};
        const T object_size = box_diagonal.norm();

        static constexpr T DISTANCE = 100;
        static constexpr T RADIUS = DISTANCE / 100;

        const T distance = object_size * DISTANCE;
        const T radius = object_size * RADIUS;
        const Vector<N, T> position = center + box_diagonal.normalized() * distance;

        auto ptr = std::make_unique<painter::BallLight<N, T, Color>>(position, -box_diagonal, radius, color);
        ptr->set_color_for_distance(distance);

        light_sources->clear();
        light_sources->push_back(std::move(ptr));
}
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<const Scene<N, T, Color>> create_simple_scene(
        const Color& light,
        const Color& background_light,
        const int max_screen_size,
        std::unique_ptr<const Shape<N, T, Color>>&& shape,
        ProgressRatio* const progress)
{
        namespace impl = simple_scene_implementation;

        ASSERT(shape);

        const geometry::BoundingBox<N, T> bounding_box = shape->bounding_box();

        std::unique_ptr<const Projector<N, T>> projector = impl::create_projector(bounding_box, max_screen_size);

        std::vector<std::unique_ptr<const LightSource<N, T, Color>>> light_sources;
        impl::create_light_sources(bounding_box, light, &light_sources);

        std::vector<std::unique_ptr<const Shape<N, T, Color>>> shapes;
        shapes.push_back(std::move(shape));

        return create_storage_scene<N, T, Color>(
                background_light, std::move(projector), std::move(light_sources), std::move(shapes), progress);
}
}
