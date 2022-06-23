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

#include "simple.h"

#include "storage_scene.h"

#include "../lights/ball_light.h"
#include "../painter/pixel_filter.h"
#include "../projectors/parallel_projector.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <cmath>

namespace ns::painter
{
namespace
{
template <typename T>
constexpr T DISTANCE = 100;

template <typename T>
constexpr T RADIUS = DISTANCE<T> / 100;

template <std::size_t N, typename T, typename Color>
std::unique_ptr<const LightSource<N, T, Color>> create_light_source(
        const Vector<N, T> center,
        const T distance,
        const T radius,
        const Color& color,
        const Vector<N, T>& direction,
        const T proportion)
{
        const Vector<N, T> position = center - direction.normalized() * distance;
        auto ptr = std::make_unique<BallLight<N, T, Color>>(position, direction, radius, color * proportion);
        ptr->set_color_for_distance(distance);
        return ptr;
}

template <std::size_t N, typename T, typename Color>
std::vector<std::unique_ptr<const LightSource<N, T, Color>>> create_light_sources(
        const T object_size,
        const Vector<N, T>& center,
        const Vector<N, T>& camera_direction,
        const Vector<N, T> light_direction,
        const T front_light_proportion,
        const Color& color)
{
        ASSERT(front_light_proportion >= 0 && front_light_proportion <= 1);

        const T distance = object_size * DISTANCE<T>;
        const T radius = object_size * RADIUS<T>;

        std::vector<std::unique_ptr<const LightSource<N, T, Color>>> res;

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

template <typename T>
std::unique_ptr<const Projector<3, T>> create_projector(
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

        return std::make_unique<const ParallelProjector<3, T>>(
                camera_position, camera_direction, screen_axes, units_per_pixel, screen_size);
}

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
                for (std::size_t i = 0; i < N - 1; ++i)
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
                for (std::size_t i = 0; i < N - 1; ++i)
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
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        for (std::size_t n = 0; n < N; ++n)
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

template <std::size_t N, typename T>
std::optional<Vector<N + 1, T>> create_clip_plane(
        const std::optional<T> clip_plane_position,
        const geometry::BoundingBox<N, T>& bounding_box)
{
        static_assert(N >= 1);

        if (!clip_plane_position)
        {
                return std::nullopt;
        }

        Vector<N + 1, T> res;
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
}

template <typename T, typename Color>
std::unique_ptr<const Scene<3, T, Color>> create_simple_scene(
        std::unique_ptr<const Shape<3, T, Color>>&& shape,
        const Color& light,
        const Color& background_light,
        const std::optional<Vector<4, T>>& clip_plane_equation,
        const std::type_identity_t<T> front_light_proportion,
        const int width,
        const int height,
        const Vector<3, T>& camera_up,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& light_direction,
        const Vector<3, T>& view_center,
        const std::type_identity_t<T> view_width,
        progress::Ratio* const progress)
{
        ASSERT(shape);

        const T shape_size = shape->bounding_box().diagonal().norm();

        std::unique_ptr<const Projector<3, T>> projector =
                create_projector(shape_size, camera_up, camera_direction, view_center, view_width, width, height);

        std::vector<std::unique_ptr<const LightSource<3, T, Color>>> light_sources = create_light_sources(
                shape_size, view_center, camera_direction, light_direction, front_light_proportion, light);

        std::vector<std::unique_ptr<const Shape<3, T, Color>>> shapes;
        shapes.push_back(std::move(shape));

        return create_storage_scene(
                background_light, clip_plane_equation, std::move(projector), std::move(light_sources),
                std::move(shapes), progress);
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<const Scene<N, T, Color>> create_simple_scene(
        std::unique_ptr<const Shape<N, T, Color>>&& shape,
        const Color& light,
        const Color& background_light,
        const std::optional<std::type_identity_t<T>> clip_plane_position,
        const std::type_identity_t<T> front_light_proportion,
        const int max_screen_size,
        progress::Ratio* const progress)
{
        ASSERT(shape);

        const geometry::BoundingBox<N, T> bounding_box = shape->bounding_box();

        std::unique_ptr<const Projector<N, T>> projector = create_projector(bounding_box, max_screen_size);

        const Vector<N, T> box_diagonal = bounding_box.diagonal();
        const Vector<N, T> center = bounding_box.min() + box_diagonal / T{2};
        const T shape_size = box_diagonal.norm();
        const Vector<N, T> camera_direction = []
        {
                Vector<N, T> res(0);
                res[N - 1] = -1;
                return res;
        }();
        const Vector<N, T> light_direction = -box_diagonal;

        std::vector<std::unique_ptr<const LightSource<N, T, Color>>> light_sources = create_light_sources(
                shape_size, center, camera_direction, light_direction, front_light_proportion, light);

        std::vector<std::unique_ptr<const Shape<N, T, Color>>> shapes;
        shapes.push_back(std::move(shape));

        return create_storage_scene<N, T, Color>(
                background_light, create_clip_plane(clip_plane_position, bounding_box), std::move(projector),
                std::move(light_sources), std::move(shapes), progress);
}

#define TEMPLATE_3(T, C)                                                                                          \
        template std::unique_ptr<const Scene<3, T, C>> create_simple_scene(                                       \
                std::unique_ptr<const Shape<3, T, C>>&&, const C&, const C&, const std::optional<Vector<4, T>>&,  \
                std::type_identity_t<T>, int, int, const Vector<3, T>&, const Vector<3, T>&, const Vector<3, T>&, \
                const Vector<3, T>&, std::type_identity_t<T>, progress::Ratio*);

#define TEMPLATE(N, T, C)                                                                                              \
        template std::unique_ptr<const Scene<(N), T, C>> create_simple_scene(                                          \
                std::unique_ptr<const Shape<(N), T, C>>&&, const C&, const C&, std::optional<std::type_identity_t<T>>, \
                std::type_identity_t<T>, int, progress::Ratio*);

TEMPLATE_INSTANTIATION_T_C(TEMPLATE_3)
TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
