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

#include <src/color/color.h>
#include <src/numerical/vec.h>
#include <src/painter/objects.h>
#include <src/painter/scenes/cornell_box.h>
#include <src/painter/scenes/single_object.h>
#include <src/painter/shapes/mesh.h>
#include <src/painter/visible_lights.h>
#include <src/painter/visible_projectors.h>

#include <memory>

template <size_t N, typename T>
struct PaintingInformation
{
        int min_screen_size;
        int max_screen_size;
};

template <typename T>
struct PaintingInformation<3, T>
{
        Vector<3, T> camera_up;
        Vector<3, T> camera_direction;
        Vector<3, T> light_direction;
        Vector<3, T> object_position;
        T object_size;
        Vector<3, T> view_center;
        T view_width;
        int width;
        int height;
        bool cornell_box;
};

struct PaintingInformationAll
{
        Color background_color;
        Color default_color;
        Color::DataType diffuse;
};

namespace painter_scene_implementation
{
template <typename T>
std::unique_ptr<const painter::Projector<3, T>> create_projector(const PaintingInformation<3, T>& info)
{
        Vector<3, T> camera_position = info.view_center - info.camera_direction * T(2) * info.object_size;
        Vector<3, T> camera_right = cross(info.camera_direction, info.camera_up);

        std::array<Vector<3, T>, 2> screen_axes{camera_right, info.camera_up};
        std::array<int, 2> screen_size{info.width, info.height};

        T units_per_pixel = info.view_width / info.width;

        return std::make_unique<const painter::VisibleParallelProjector<3, T>>(
                camera_position, info.camera_direction, screen_axes, units_per_pixel, screen_size);
}

template <typename T>
std::unique_ptr<const painter::LightSource<3, T>> create_light_source(const PaintingInformation<3, T>& info)
{
        Vector<3, T> light_position = info.object_position - info.light_direction * info.object_size * T(1000);

        return std::make_unique<const painter::VisibleConstantLight<3, T>>(light_position, Color(1));
}
}

template <size_t N, typename T>
std::unique_ptr<const painter::PaintObjects<N, T>> create_painter_scene(
        const std::shared_ptr<const painter::MeshObject<N, T>>& mesh,
        const PaintingInformation<N, T>& info,
        const PaintingInformationAll& info_all)
{
        if constexpr (N == 3)
        {
                namespace impl = painter_scene_implementation;

                if (!info.cornell_box)
                {
                        return single_object_scene(
                                info_all.background_color, info_all.default_color, info_all.diffuse,
                                impl::create_projector(info), impl::create_light_source(info), mesh);
                }
                else
                {
                        return cornell_box_scene(
                                info.width, info.height, mesh, info.object_size, info_all.default_color,
                                info_all.diffuse, info.camera_direction, info.camera_up);
                }
        }
        else
        {
                return single_object_scene(
                        info_all.background_color, info_all.default_color, info_all.diffuse, info.min_screen_size,
                        info.max_screen_size, mesh);
        }
}
