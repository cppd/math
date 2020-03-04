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

#include "painting.h"

#include "painter_window.h"

#include "../dialogs/parameters/painter_3d.h"
#include "../dialogs/parameters/painter_nd.h"
#include "../support/support.h"

#include <src/com/math.h>
#include <src/painter/scenes/cornell_box.h>
#include <src/painter/scenes/single_object.h>
#include <src/painter/visible_lights.h>
#include <src/painter/visible_projectors.h>

namespace
{
template <typename T>
std::unique_ptr<const Projector<3, T>> create_projector(
        const PaintingInformation3d<T>& info,
        int paint_width,
        int paint_height)
{
        Vector<3, T> camera_position = info.view_center - info.camera_direction * T(2) * info.object_size;
        Vector<3, T> camera_right = cross(info.camera_direction, info.camera_up);

        std::array<Vector<3, T>, 2> screen_axes{camera_right, info.camera_up};
        std::array<int, 2> screen_size{paint_width, paint_height};

        T units_per_pixel = info.view_width / paint_width;

        return std::make_unique<const VisibleParallelProjector<3, T>>(
                camera_position, info.camera_direction, screen_axes, units_per_pixel, screen_size);
}

template <typename T>
std::unique_ptr<const LightSource<3, T>> create_light_source(const PaintingInformation3d<T>& info)
{
        Vector<3, T> light_position = info.object_position - info.light_direction * info.object_size * T(1000);

        return std::make_unique<const VisibleConstantLight<3, T>>(light_position, Color(1));
}
}

template <typename T>
void painting(
        const std::shared_ptr<const SpatialMeshModel<3, T>>& mesh,
        const PaintingInformation3d<T>& info_3d,
        const PaintingInformationAll& info_all)
{
        ASSERT(info_all.default_samples_per_dimension > 0);
        ASSERT(info_all.max_samples_per_dimension > 0);

        const int default_sample_count = square(info_all.default_samples_per_dimension);
        const int max_sample_count = square(info_all.max_samples_per_dimension);

        int width;
        int height;
        int thread_count;
        int samples_per_pixel;
        bool flat_facets;
        bool cornell_box;

        if (!dialog::painter_parameters_for_3d(
                    info_all.parent_window, hardware_concurrency(), info_3d.paint_width, info_3d.paint_height,
                    info_3d.max_screen_size, default_sample_count, max_sample_count, &thread_count, &width, &height,
                    &samples_per_pixel, &flat_facets, &cornell_box))
        {
                return;
        }

        std::string title;
        std::unique_ptr<const PaintObjects<3, T>> scene;

        if (!cornell_box)
        {
                title = info_all.window_title + " (" + info_all.object_name + ")";

                scene = single_object_scene(
                        info_all.background_color, info_all.default_color, info_all.diffuse,
                        create_projector(info_3d, width, height), create_light_source(info_3d), mesh);
        }
        else
        {
                title = info_all.window_title + " (" + info_all.object_name + " in Cornell Box)";

                scene = cornell_box_scene(
                        width, height, mesh, info_3d.object_size, info_all.default_color, info_all.diffuse,
                        info_3d.camera_direction, info_3d.camera_up);
        }

        create_and_show_delete_on_close_window<PainterWindow<3, T>>(
                title, thread_count, samples_per_pixel, !flat_facets, std::move(scene));
}

template <size_t N, typename T>
void painting(
        const std::shared_ptr<const SpatialMeshModel<N, T>>& mesh,
        const PaintingInformationNd& info_nd,
        const PaintingInformationAll& info_all)
{
        static_assert(N >= 4);

        ASSERT(info_all.default_samples_per_dimension > 0);
        ASSERT(info_all.max_samples_per_dimension > 0);

        constexpr int screen_dimension = N - 1;
        const int default_sample_count =
                power<screen_dimension>(static_cast<unsigned>(info_all.default_samples_per_dimension));
        const int max_sample_count = power<screen_dimension>(static_cast<unsigned>(info_all.max_samples_per_dimension));

        int min_size;
        int max_size;
        int thread_count;
        int samples_per_pixel;
        bool flat_facets;

        if (!dialog::painter_parameters_for_nd(
                    info_all.parent_window, N, hardware_concurrency(), info_nd.default_screen_size,
                    info_nd.minimum_screen_size, info_nd.maximum_screen_size, default_sample_count, max_sample_count,
                    &thread_count, &min_size, &max_size, &samples_per_pixel, &flat_facets))
        {
                return;
        }

        std::string title = info_all.window_title + " (" + info_all.object_name + ")";

        std::unique_ptr<const PaintObjects<N, T>> scene = single_object_scene(
                info_all.background_color, info_all.default_color, info_all.diffuse, min_size, max_size, mesh);

        create_and_show_delete_on_close_window<PainterWindow<N, T>>(
                title, thread_count, samples_per_pixel, !flat_facets, std::move(scene));
}

template void painting(
        const std::shared_ptr<const SpatialMeshModel<3, float>>& mesh,
        const PaintingInformation3d<float>& info_3d,
        const PaintingInformationAll& info_all);
template void painting(
        const std::shared_ptr<const SpatialMeshModel<3, double>>& mesh,
        const PaintingInformation3d<double>& info_3d,
        const PaintingInformationAll& info_all);

template void painting(
        const std::shared_ptr<const SpatialMeshModel<4, float>>& mesh,
        const PaintingInformationNd& info_nd,
        const PaintingInformationAll& info_all);
template void painting(
        const std::shared_ptr<const SpatialMeshModel<5, float>>& mesh,
        const PaintingInformationNd& info_nd,
        const PaintingInformationAll& info_all);
template void painting(
        const std::shared_ptr<const SpatialMeshModel<6, float>>& mesh,
        const PaintingInformationNd& info_nd,
        const PaintingInformationAll& info_all);

template void painting(
        const std::shared_ptr<const SpatialMeshModel<4, double>>& mesh,
        const PaintingInformationNd& info_nd,
        const PaintingInformationAll& info_all);
template void painting(
        const std::shared_ptr<const SpatialMeshModel<5, double>>& mesh,
        const PaintingInformationNd& info_nd,
        const PaintingInformationAll& info_all);
template void painting(
        const std::shared_ptr<const SpatialMeshModel<6, double>>& mesh,
        const PaintingInformationNd& info_nd,
        const PaintingInformationAll& info_all);
