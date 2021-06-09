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
template <std::size_t N, typename T>
struct PainterSceneInfo
{
        int min_screen_size;
        int max_screen_size;
        bool cornell_box;

        PainterSceneInfo(int min_screen_size, int max_screen_size, bool cornell_box)
                : min_screen_size(min_screen_size), max_screen_size(max_screen_size), cornell_box(cornell_box)
        {
        }
};

template <typename T>
struct PainterSceneInfo<3, T>
{
        Vector<3, T> camera_up;
        Vector<3, T> camera_direction;
        Vector<3, T> light_direction;
        Vector<3, T> view_center;
        T view_width;
        int width;
        int height;
        bool cornell_box;

        template <typename F>
        PainterSceneInfo(
                Vector<3, F> camera_up,
                Vector<3, F> camera_direction,
                Vector<3, F> light_direction,
                Vector<3, F> view_center,
                std::type_identity_t<F> view_width,
                int width,
                int height,
                bool cornell_box)
                : camera_up(to_vector<T>(camera_up)),
                  camera_direction(to_vector<T>(camera_direction)),
                  light_direction(to_vector<T>(light_direction)),
                  view_center(to_vector<T>(view_center)),
                  view_width(view_width),
                  width(width),
                  height(height),
                  cornell_box(cornell_box)
        {
                static_assert(std::is_floating_point_v<F>);
        }
};

namespace painter_scene_implementation
{
template <typename T>
std::unique_ptr<const painter::Projector<3, T>> create_projector(const PainterSceneInfo<3, T>& info, T scene_size)
{
        Vector<3, T> camera_position = info.view_center - info.camera_direction * T(2) * scene_size;
        Vector<3, T> camera_right = cross(info.camera_direction, info.camera_up);

        std::array<Vector<3, T>, 2> screen_axes{camera_right, info.camera_up};
        std::array<int, 2> screen_size{info.width, info.height};

        T units_per_pixel = info.view_width / info.width;

        return std::make_unique<const painter::ParallelProjector<3, T>>(
                camera_position, info.camera_direction, screen_axes, units_per_pixel, screen_size);
}

template <typename T, typename Color>
std::unique_ptr<const painter::LightSource<3, T, Color>> create_light_source(
        const PainterSceneInfo<3, T>& info,
        const std::type_identity_t<typename Color::DataType>& lighting_intensity)
{
        return std::make_unique<const painter::DistantLight<3, T, Color>>(
                -info.light_direction,
                Color(lighting_intensity, lighting_intensity, lighting_intensity, color::Type::Illumination));
}

template <typename T, typename Color>
std::unique_ptr<const painter::Scene<3, T, Color>> create_scene(
        std::unique_ptr<const painter::Shape<3, T, Color>>&& shape,
        const PainterSceneInfo<3, T>& info,
        const Color& background_light,
        const typename Color::DataType& lighting_intensity)
{
        const geometry::BoundingBox<3, T> bb = shape->bounding_box();
        const T scene_size = (bb.max - bb.min).norm();

        std::vector<std::unique_ptr<const painter::LightSource<3, T, Color>>> light_sources;
        light_sources.push_back(create_light_source<T, Color>(info, lighting_intensity));

        std::vector<std::unique_ptr<const painter::Shape<3, T, Color>>> shapes;
        shapes.push_back(std::move(shape));

        return painter::create_storage_scene(
                background_light, create_projector(info, scene_size), std::move(light_sources), std::move(shapes));
}
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<const painter::Scene<N, T, Color>> create_painter_scene(
        std::unique_ptr<const painter::Shape<N, T, Color>>&& shape,
        const PainterSceneInfo<N, T>& info,
        const color::Color& background_light,
        const color::Color::DataType& lighting_intensity)
{
        const auto background = [&background_light]
        {
                Vector<3, float> rgb = background_light.rgb32();
                return Color(rgb[0], rgb[1], rgb[2], color::Type::Illumination);
        };

        if constexpr (N == 3)
        {
                namespace impl = painter_scene_implementation;

                if (!info.cornell_box)
                {
                        return impl::create_scene(std::move(shape), info, background(), lighting_intensity);
                }
                return painter::create_cornell_box_scene(
                        info.width, info.height, std::move(shape), info.camera_direction, info.camera_up);
        }
        else
        {
                if (!info.cornell_box)
                {
                        return painter::create_simple_scene(
                                background(), lighting_intensity, info.min_screen_size, info.max_screen_size,
                                std::move(shape));
                }
                return painter::create_cornell_box_scene(info.max_screen_size, std::move(shape));
        }
}
}
