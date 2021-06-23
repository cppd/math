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

#include <src/color/color.h>
#include <src/numerical/vec.h>
#include <src/painter/lights/distant_light.h>
#include <src/painter/objects.h>
#include <src/painter/projectors/parallel_projector.h>
#include <src/painter/scenes/cornell_box.h>
#include <src/painter/scenes/simple.h>
#include <src/painter/scenes/storage_scene.h>
#include <src/painter/shapes/mesh.h>

#include <memory>

namespace ns::process
{
namespace painter_scene_implementation
{
template <typename T>
std::unique_ptr<const painter::Projector<3, T>> create_projector(
        const geometry::BoundingBox<3, T>& bounding_box,
        const Vector<3, T>& camera_up,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& view_center,
        const T view_width,
        const int width,
        const int height)
{
        const T scene_size = (bounding_box.max - bounding_box.min).norm();

        Vector<3, T> camera_position = view_center - camera_direction * T(2) * scene_size;
        Vector<3, T> camera_right = cross(camera_direction, camera_up);

        std::array<Vector<3, T>, 2> screen_axes{camera_right, camera_up};
        std::array<int, 2> screen_size{width, height};

        T units_per_pixel = view_width / width;

        return std::make_unique<const painter::ParallelProjector<3, T>>(
                camera_position, camera_direction, screen_axes, units_per_pixel, screen_size);
}

template <typename Color>
Color light_color(const double intensity)
{
        return Color::illuminant(intensity, intensity, intensity);
}

template <typename Color>
Color background_light_color(const color::Color& background_light)
{
        return background_light.to_illuminant<Color>();
}

template <typename T, typename Color>
std::unique_ptr<const painter::LightSource<3, T, Color>> create_light_source(
        const Vector<3, T>& direction,
        const Color& color)
{
        return std::make_unique<const painter::DistantLight<3, T, Color>>(-direction, color);
}
}

template <typename T, typename Color>
std::unique_ptr<const painter::Scene<3, T, Color>> create_painter_scene(
        std::unique_ptr<const painter::Shape<3, T, Color>>&& shape,
        const Vector<3, T>& camera_up,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& light_direction,
        const Vector<3, T>& view_center,
        const std::type_identity_t<T> view_width,
        const int width,
        const int height,
        const bool cornell_box,
        const color::Color& background_light,
        const double lighting_intensity)
{
        if (cornell_box)
        {
                return painter::create_cornell_box_scene(width, height, std::move(shape), camera_direction, camera_up);
        }

        namespace impl = painter_scene_implementation;

        std::unique_ptr<const painter::Projector<3, T>> projector;
        projector = impl::create_projector(
                shape->bounding_box(), camera_up, camera_direction, view_center, view_width, width, height);

        std::vector<std::unique_ptr<const painter::LightSource<3, T, Color>>> light_sources;
        light_sources.push_back(
                impl::create_light_source(light_direction, impl::light_color<Color>(lighting_intensity)));

        std::vector<std::unique_ptr<const painter::Shape<3, T, Color>>> shapes;
        shapes.push_back(std::move(shape));

        return painter::create_storage_scene(
                impl::background_light_color<Color>(background_light), std::move(projector), std::move(light_sources),
                std::move(shapes));
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<const painter::Scene<N, T, Color>> create_painter_scene(
        std::unique_ptr<const painter::Shape<N, T, Color>>&& shape,
        const int min_screen_size,
        const int max_screen_size,
        const bool cornell_box,
        const color::Color& background_light,
        const double lighting_intensity)
{
        static_assert(N >= 4);

        if (cornell_box)
        {
                return painter::create_cornell_box_scene(max_screen_size, std::move(shape));
        }

        namespace impl = painter_scene_implementation;

        return painter::create_simple_scene(
                impl::light_color<Color>(lighting_intensity), impl::background_light_color<Color>(background_light),
                min_screen_size, max_screen_size, std::move(shape));
}
}
