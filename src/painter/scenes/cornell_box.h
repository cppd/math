/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "../objects.h"
#include "../projectors/perspective_projector.h"
#include "../shapes/hyperplane_parallelotope.h"
#include "../shapes/parallelotope.h"
#include "../shapes/shape.h"

#include <src/color/colors.h>
#include <src/numerical/vec.h>

#include <memory>

namespace painter
{
template <typename T>
std::unique_ptr<const Scene<3, T>> cornell_box_scene(
        int width,
        int height,
        std::unique_ptr<const Shape<3, T>>&& shape,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& camera_up)
{
        std::vector<std::unique_ptr<const Shape<3, T>>> shapes;
        std::vector<std::unique_ptr<const LightSource<3, T>>> light_sources;

        shapes.push_back(std::move(shape));

        const BoundingBox bb = shapes.front()->bounding_box();
        const T size = (bb.max - bb.min).norm() * T(1.1);
        const Vector<3, T> center = (bb.max + bb.min) * T(0.5);

        const Vector<3, T> dir = size * camera_direction.normalized();
        const Vector<3, T> right = size * cross(camera_direction, camera_up).normalized();
        const Vector<3, T> up = size * cross(right, dir).normalized();

        const T box_size = 0.16;
        const T box_space = 0.08;
        const T near = 0.7;
        const T depth = near + 0.5 + box_size + 2 * box_space;

        const Vector<3, T> lower_left = center - near * dir + T(0.5) * (-right - up);
        const Vector<3, T> lower_right = center - near * dir + T(0.5) * (right - up);
        const Vector<3, T> upper_left = center - near * dir + T(0.5) * (-right + up);

        const Color::DataType DIFFUSE = 1;
        const Color::DataType ALPHA = 1;

        // back
        shapes.push_back(std::make_unique<shapes::HyperplaneParallelotope<3, T>>(
                colors::WHITE, DIFFUSE, ALPHA, lower_left + depth * dir, right, up));

        // top
        shapes.push_back(std::make_unique<shapes::HyperplaneParallelotope<3, T>>(
                colors::WHITE, DIFFUSE, ALPHA, upper_left, depth * dir, right));

        // bottom
        shapes.push_back(std::make_unique<shapes::HyperplaneParallelotope<3, T>>(
                colors::WHITE, DIFFUSE, ALPHA, lower_left, depth * dir, right));

        // left
        shapes.push_back(std::make_unique<shapes::HyperplaneParallelotope<3, T>>(
                colors::RED, DIFFUSE, ALPHA, lower_left, depth * dir, up));

        // right
        shapes.push_back(std::make_unique<shapes::HyperplaneParallelotope<3, T>>(
                colors::GREEN, DIFFUSE, ALPHA, lower_right, depth * dir, up));

        // box
        shapes.push_back(std::make_unique<shapes::Parallelotope<3, T>>(
                colors::MAGENTA, DIFFUSE, ALPHA,
                lower_left + (near + T(0.5) + box_space) * dir + (1 - box_space - box_size) * right + box_space * up,
                box_size * right, (1 - 2 * box_space) * up, box_size * dir));

        // lamp
        {
                const T lamp_size = 0.2;
                std::unique_ptr<shapes::HyperplaneParallelotope<3, T>> lamp =
                        std::make_unique<shapes::HyperplaneParallelotope<3, T>>(
                                colors::WHITE, DIFFUSE, ALPHA,
                                center + T(0.499) * up - (lamp_size / 2) * dir - (lamp_size / 2) * right,
                                lamp_size * right, lamp_size * dir);
                lamp->set_light_source(Color(50));
                shapes.push_back(std::move(lamp));
        }

        const std::array<int, 2> screen_sizes{width, height};
        const std::array<Vector<3, T>, 2> screen_axes{right, up};
        const Vector<3, T> view_point = center - dir;
        std::unique_ptr<Projector<3, T>> projector =
                std::make_unique<PerspectiveProjector<3, T>>(view_point, dir, screen_axes, 70, screen_sizes);
        // std::make_unique<ParallelProjector<3, T>>(view_point, dir, screen_axes, size / width, screen_sizes);
        // std::make_unique<SphericalProjector<3, T>>(view_point, dir, screen_axes, 80, screen_sizes);

        const Color background_color = colors::BLACK;

        return std::make_unique<StorageScene<3, T>>(
                background_color, std::move(projector), std::move(light_sources), std::move(shapes));
}
}
