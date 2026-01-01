/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "simple.h"

#include "storage.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/geometry/spatial/bounding_box.h>
#include <src/numerical/vector.h>
#include <src/painter/lights/ball_light.h>
#include <src/painter/objects.h>
#include <src/painter/pixels/pixel_filter.h>
#include <src/painter/projectors/parallel_projector.h>
#include <src/progress/progress.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

namespace ns::painter::scenes
{
namespace
{
template <typename T>
constexpr T LIGHT_SOURCE_DISTANCE = 100;

template <typename T>
constexpr T LIGHT_SOURCE_RADIUS = LIGHT_SOURCE_DISTANCE<T> / 100;

template <std::size_t N, typename T>
struct Info final
{
        numerical::Vector<N, T> light_direction;
        numerical::Vector<N, T> camera_direction;
        std::array<numerical::Vector<N, T>, N - 1> screen_axes;
        std::array<int, N - 1> screen_size;
        T units_per_pixel;
};

template <typename T>
Info<3, T> create_info(
        const int screen_width,
        const int screen_height,
        const numerical::Vector<3, T>& camera_up,
        const numerical::Vector<3, T>& camera_direction,
        const numerical::Vector<3, T>& light_direction,
        const T view_width)
{
        Info<3, T> info;
        info.light_direction = light_direction;
        info.camera_direction = camera_direction;
        info.screen_axes = {cross(camera_direction, camera_up), camera_up};
        info.screen_size = {screen_width, screen_height};
        info.units_per_pixel = view_width / screen_width;
        return info;
}

template <std::size_t N, typename T>
Info<N, T> create_info(const geometry::spatial::BoundingBox<N, T>& bounding_box, const int max_screen_size)
{
        static constexpr int BORDER_SIZE = pixels::PixelFilter<N, T>::integer_radius();

        if (max_screen_size <= 2 * BORDER_SIZE)
        {
                error("Maximum screen size (" + to_string(max_screen_size) + ") must be greater than or equal to "
                      + to_string(1 + 2 * BORDER_SIZE));
        }

        const int max_view_screen_size = max_screen_size - 2 * BORDER_SIZE;

        const numerical::Vector<N, T> view_size = bounding_box.diagonal();

        const T max_view_size = [&]
        {
                static_assert(N >= 2);
                T res = 0;
                // excluding camera direction N - 1
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        if (!(view_size[i] > 0))
                        {
                                error("Object projection size " + to_string(view_size[i]) + " is not positive");
                        }
                        res = std::max(view_size[i], res);
                }
                return res;
        }();

        Info<N, T> info;
        info.light_direction = -bounding_box.diagonal();
        info.camera_direction = []
        {
                numerical::Vector<N, T> res(0);
                res[N - 1] = -1;
                return res;
        }();
        info.screen_axes = []
        {
                std::array<numerical::Vector<N, T>, N - 1> res;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        res[i] = numerical::Vector<N, T>(0);
                        res[i][i] = 1;
                }
                return res;
        }();
        info.screen_size = [&]
        {
                std::array<int, N - 1> res;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        const int size_in_pixels = std::ceil((view_size[i] / max_view_size) * max_view_screen_size);
                        ASSERT(size_in_pixels <= max_view_screen_size);
                        res[i] = std::max(1, size_in_pixels) + 2 * BORDER_SIZE;
                }
                return res;
        }();
        info.units_per_pixel = max_view_size / max_view_screen_size;
        return info;
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<LightSource<N, T, Color>> create_light_source(
        const numerical::Vector<N, T>& center,
        const T distance,
        const T radius,
        const Color& color,
        const numerical::Vector<N, T>& direction,
        const T proportion)
{
        const numerical::Vector<N, T> position = center - direction.normalized() * distance;

        auto res = std::make_unique<lights::BallLight<N, T, Color>>(position, direction, radius, color * proportion);
        res->set_radiance_for_distance(distance);
        return res;
}

template <std::size_t N, typename T, typename Color>
std::vector<std::unique_ptr<LightSource<N, T, Color>>> create_light_sources(
        const T object_size,
        const numerical::Vector<N, T>& center,
        const Info<N, T>& info,
        const T front_light_proportion,
        const Color& color)
{
        ASSERT(front_light_proportion >= 0 && front_light_proportion <= 1);

        const T distance = object_size * LIGHT_SOURCE_DISTANCE<T>;
        const T radius = object_size * LIGHT_SOURCE_RADIUS<T>;

        std::vector<std::unique_ptr<LightSource<N, T, Color>>> res;

        if (front_light_proportion > 0)
        {
                res.push_back(create_light_source(
                        center, distance, radius, color, info.camera_direction, front_light_proportion));
        }

        const T side_light_proportion = 1 - front_light_proportion;
        if (side_light_proportion > 0)
        {
                res.push_back(create_light_source(
                        center, distance, radius, color, info.light_direction, side_light_proportion));
        }

        return res;
}

template <std::size_t N, typename T>
std::unique_ptr<const Projector<N, T>> create_projector(
        const T shape_size,
        const numerical::Vector<N, T>& center,
        const Info<N, T>& info)
{
        const numerical::Vector<N, T> camera_position = center - info.camera_direction * (2 * shape_size);

        return std::make_unique<const projectors::ParallelProjector<N, T>>(
                camera_position, info.camera_direction, info.screen_axes, info.units_per_pixel, info.screen_size);
}

template <std::size_t N, typename T>
std::optional<numerical::Vector<N + 1, T>> create_clip_plane(
        const std::optional<T> clip_plane_position,
        const geometry::spatial::BoundingBox<N, T>& bounding_box)
{
        static_assert(N >= 1);

        if (!clip_plane_position)
        {
                return std::nullopt;
        }

        numerical::Vector<N + 1, T> res;
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                res[i] = 0;
        }
        res[N - 1] = -1;

        // n * point + d = 0
        // d = -n * point
        // d = point
        const T min = bounding_box.min()[N - 1];
        const T max = bounding_box.max()[N - 1];
        const T t = std::clamp(*clip_plane_position, T{0}, T{1});
        res[N] = std::lerp(min, max, t);

        return res;
}

template <std::size_t N, typename T, typename Color>
StorageScene<N, T, Color> create_simple_scene(
        std::unique_ptr<const Shape<N, T, Color>>&& shape,
        const Color& light,
        const Color& background_light,
        const std::optional<numerical::Vector<N + 1, T>>& clip_plane_equation,
        const T front_light_proportion,
        const numerical::Vector<N, T>& center,
        const T shape_size,
        const Info<N, T>& info,
        progress::Ratio* const progress)
{
        ASSERT(shape);

        std::unique_ptr<const Projector<N, T>> projector = create_projector(shape_size, center, info);

        std::vector<std::unique_ptr<LightSource<N, T, Color>>> light_sources =
                create_light_sources(shape_size, center, info, front_light_proportion, light);

        std::vector<std::unique_ptr<const Shape<N, T, Color>>> shapes;
        shapes.push_back(std::move(shape));

        return create_storage_scene<N, T, Color>(
                background_light, clip_plane_equation, std::move(projector), std::move(light_sources),
                std::move(shapes), progress);
}
}

template <typename T, typename Color>
StorageScene<3, T, Color> create_simple_scene(
        std::unique_ptr<const Shape<3, T, Color>>&& shape,
        const Color& light,
        const Color& background_light,
        const std::optional<numerical::Vector<4, T>>& clip_plane_equation,
        const std::type_identity_t<T> front_light_proportion,
        const int screen_width,
        const int screen_height,
        const numerical::Vector<3, T>& camera_up,
        const numerical::Vector<3, T>& camera_direction,
        const numerical::Vector<3, T>& light_direction,
        const numerical::Vector<3, T>& view_center,
        const std::type_identity_t<T> view_width,
        progress::Ratio* const progress)
{
        ASSERT(shape);

        const Info<3, T> info =
                create_info(screen_width, screen_height, camera_up, camera_direction, light_direction, view_width);
        const T shape_size = shape->bounding_box().diagonal().norm();

        return create_simple_scene(
                std::move(shape), light, background_light, clip_plane_equation, front_light_proportion, view_center,
                shape_size, info, progress);
}

template <std::size_t N, typename T, typename Color>
StorageScene<N, T, Color> create_simple_scene(
        std::unique_ptr<const Shape<N, T, Color>>&& shape,
        const Color& light,
        const Color& background_light,
        const std::optional<std::type_identity_t<T>> clip_plane_position,
        const std::type_identity_t<T> front_light_proportion,
        const int max_screen_size,
        progress::Ratio* const progress)
{
        ASSERT(shape);

        const geometry::spatial::BoundingBox<N, T> bounding_box = shape->bounding_box();
        const numerical::Vector<N, T> box_diagonal = bounding_box.diagonal();
        const numerical::Vector<N, T> center = bounding_box.min() + box_diagonal / T{2};

        const Info<N, T> info = create_info(bounding_box, max_screen_size);
        const T shape_size = box_diagonal.norm();
        const std::optional<numerical::Vector<N + 1, T>> clip_plane =
                create_clip_plane(clip_plane_position, bounding_box);

        return create_simple_scene(
                std::move(shape), light, background_light, clip_plane, front_light_proportion, center, shape_size, info,
                progress);
}

#define TEMPLATE_3(T, C)                                                                                        \
        template StorageScene<3, T, C> create_simple_scene(                                                     \
                std::unique_ptr<const Shape<3, T, C>>&&, const C&, const C&,                                    \
                const std::optional<numerical::Vector<4, T>>&, std::type_identity_t<T>, int, int,               \
                const numerical::Vector<3, T>&, const numerical::Vector<3, T>&, const numerical::Vector<3, T>&, \
                const numerical::Vector<3, T>&, std::type_identity_t<T>, progress::Ratio*);

#define TEMPLATE(N, T, C)                                                                                              \
        template StorageScene<N, T, C> create_simple_scene(                                                            \
                std::unique_ptr<const Shape<(N), T, C>>&&, const C&, const C&, std::optional<std::type_identity_t<T>>, \
                std::type_identity_t<T>, int, progress::Ratio*);

TEMPLATE_INSTANTIATION_T_C(TEMPLATE_3)
TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
