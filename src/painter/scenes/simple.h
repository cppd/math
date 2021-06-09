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

#include "storage_scene.h"

#include "../lights/distant_light.h"
#include "../objects.h"
#include "../projectors/parallel_projector.h"
#include "../shapes/shape.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/type/limit.h>

#include <algorithm>
#include <cmath>
#include <memory>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
std::unique_ptr<const Scene<N, T, Color>> create_simple_scene(
        const Color& background_light,
        const typename Color::DataType& lighting_intensity,
        int min_screen_size,
        int max_screen_size,
        std::unique_ptr<const Shape<N, T, Color>>&& shape)
{
        ASSERT(shape);

        if (min_screen_size < 3)
        {
                error("Min screen size (" + to_string(min_screen_size) + ") is too small");
        }

        if (min_screen_size > max_screen_size)
        {
                error("Wrong min and max screen sizes: min = " + to_string(min_screen_size)
                      + ", max = " + to_string(max_screen_size));
        }

        const geometry::BoundingBox<N, T> bb = shape->bounding_box();
        const Vector<N, T> object_size = bb.max - bb.min;
        const Vector<N, T> center = bb.min + object_size / T(2);

        //

        T max_projected_object_size = limits<T>::lowest();
        for (unsigned i = 0; i < N - 1; ++i) // кроме последнего измерения, по которому находится камера
        {
                max_projected_object_size = std::max(object_size[i], max_projected_object_size);
        }
        if (max_projected_object_size == 0)
        {
                error("Object is a point on the screen");
        }

        std::array<int, N - 1> screen_size;
        for (unsigned i = 0; i < N - 1; ++i)
        {
                int size_in_pixels = std::lround((object_size[i] / max_projected_object_size) * max_screen_size);
                screen_size[i] = std::clamp(size_in_pixels, min_screen_size, max_screen_size);
        }

        Vector<N, T> camera_position(center);
        camera_position[N - 1] = bb.max[N - 1] + object_size.norm();

        Vector<N, T> camera_direction(0);
        camera_direction[N - 1] = -1;

        std::array<Vector<N, T>, N - 1> screen_axes;
        for (unsigned i = 0; i < N - 1; ++i)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        screen_axes[i][n] = (i != n) ? 0 : 1;
                }
        }

        T units_per_pixel = max_projected_object_size / max_screen_size;

        std::unique_ptr<const Projector<N, T>> projector = std::make_unique<const ParallelProjector<N, T>>(
                camera_position, camera_direction, screen_axes, units_per_pixel, screen_size);

        //

        Vector<N, T> light_direction(bb.max - center);

        std::vector<std::unique_ptr<const LightSource<N, T, Color>>> light_sources;
        light_sources.push_back(std::make_unique<const DistantLight<N, T, Color>>(
                light_direction,
                Color(lighting_intensity, lighting_intensity, lighting_intensity, color::Type::Illumination)));

        //

        std::vector<std::unique_ptr<const Shape<N, T, Color>>> shapes;
        shapes.push_back(std::move(shape));

        return create_storage_scene<N, T, Color>(
                background_light, std::move(projector), std::move(light_sources), std::move(shapes));
}
}
