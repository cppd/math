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

#include "painter_scene.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/painter/lights/ball_light.h>
#include <src/painter/objects.h>
#include <src/painter/projectors/parallel_projector.h>
#include <src/painter/scenes/cornell_box.h>
#include <src/painter/scenes/simple.h>
#include <src/painter/scenes/storage_scene.h>
#include <src/settings/instantiation.h>

namespace ns::process
{
namespace
{
template <typename T>
std::unique_ptr<const painter::Projector<3, T>> create_projector(
        const T& shape_size,
        const Vector<3, T>& camera_up,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& view_center,
        const T view_width,
        const int width,
        const int height)
{
        const Vector<3, T> camera_position = view_center - camera_direction * (2 * shape_size);
        const Vector<3, T> camera_right = cross(camera_direction, camera_up);

        const std::array<Vector<3, T>, 2> screen_axes{camera_right, camera_up};
        const std::array<int, 2> screen_size{width, height};

        const T units_per_pixel = view_width / width;

        return std::make_unique<const painter::ParallelProjector<3, T>>(
                camera_position, camera_direction, screen_axes, units_per_pixel, screen_size);
}

template <typename T, typename Color>
std::unique_ptr<const painter::LightSource<3, T, Color>> create_light_source(
        const Vector<3, T>& center,
        const T distance,
        const T radius,
        const Color& color,
        const Vector<3, T>& direction,
        const T proportion)
{
        const Vector<3, T> position = center - direction.normalized() * distance;
        auto ptr = std::make_unique<painter::BallLight<3, T, Color>>(position, direction, radius, color * proportion);
        ptr->set_color_for_distance(distance);
        return ptr;
}

template <typename T, typename Color>
std::vector<std::unique_ptr<const painter::LightSource<3, T, Color>>> create_light_sources(
        const T& shape_size,
        const Vector<3, T>& center,
        const Vector<3, T>& light_direction,
        const Vector<3, T>& camera_direction,
        const T front_light_proportion,
        const Color& color)
{
        ASSERT(front_light_proportion >= 0 && front_light_proportion <= 1);

        static constexpr T DISTANCE = 100;
        static constexpr T RADIUS = DISTANCE / 100;

        const T distance = shape_size * DISTANCE;
        const T radius = shape_size * RADIUS;

        std::vector<std::unique_ptr<const painter::LightSource<3, T, Color>>> res;

        if (front_light_proportion > 0)
        {
                res.push_back(
                        create_light_source(center, distance, radius, color, camera_direction, front_light_proportion));
        }

        const T side_light_proportion = 1 - front_light_proportion;
        if (side_light_proportion > 0)
        {
                res.push_back(
                        create_light_source(center, distance, radius, color, light_direction, side_light_proportion));
        }

        return res;
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
        const std::optional<Vector<4, T>>& clip_plane_equation,
        const std::type_identity_t<T> front_light_proportion,
        const int width,
        const int height,
        const bool cornell_box,
        const Color& light,
        const Color& background_light,
        progress::Ratio* const progress)
{
        if (cornell_box)
        {
                return painter::create_cornell_box_scene(
                        light, background_light, {width, height}, std::move(shape), progress);
        }

        const T shape_size = shape->bounding_box().diagonal().norm();

        std::unique_ptr<const painter::Projector<3, T>> projector =
                create_projector(shape_size, camera_up, camera_direction, view_center, view_width, width, height);

        std::vector<std::unique_ptr<const painter::LightSource<3, T, Color>>> light_sources = create_light_sources(
                shape_size, view_center, light_direction, camera_direction, front_light_proportion, light);

        std::vector<std::unique_ptr<const painter::Shape<3, T, Color>>> shapes;
        shapes.push_back(std::move(shape));

        return painter::create_storage_scene(
                background_light, clip_plane_equation, std::move(projector), std::move(light_sources),
                std::move(shapes), progress);
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<const painter::Scene<N, T, Color>> create_painter_scene(
        std::unique_ptr<const painter::Shape<N, T, Color>>&& shape,
        const int max_screen_size,
        const bool cornell_box,
        const Color& light,
        const Color& background_light,
        const std::optional<std::type_identity_t<T>> clip_plane_position,
        const std::type_identity_t<T> front_light_proportion,
        progress::Ratio* const progress)
{
        if (!cornell_box)
        {
                return painter::create_simple_scene(
                        light, background_light, clip_plane_position, front_light_proportion, max_screen_size,
                        std::move(shape), progress);
        }

        std::array<int, N - 1> screen_size;
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                screen_size[i] = max_screen_size;
        }
        return painter::create_cornell_box_scene(light, background_light, screen_size, std::move(shape), progress);
}

#define TEMPLATE_3(T, C)                                                                                               \
        template std::unique_ptr<const painter::Scene<3, T, C>> create_painter_scene(                                  \
                std::unique_ptr<const painter::Shape<3, T, C>>&&, const Vector<3, T>&, const Vector<3, T>&,            \
                const Vector<3, T>&, const Vector<3, T>&, std::type_identity_t<T>, const std::optional<Vector<4, T>>&, \
                std::type_identity_t<T>, int, int, bool, const C&, const C&, progress::Ratio*);

#define TEMPLATE(N, T, C)                                                                          \
        template std::unique_ptr<const painter::Scene<(N), T, C>> create_painter_scene(            \
                std::unique_ptr<const painter::Shape<(N), T, C>>&&, int, bool, const C&, const C&, \
                std::optional<std::type_identity_t<T>>, std::type_identity_t<T>, progress::Ratio*);

TEMPLATE_INSTANTIATION_T_C(TEMPLATE_3)
TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
