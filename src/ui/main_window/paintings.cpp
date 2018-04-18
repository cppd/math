/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "paintings.h"

#include "path_tracing/scenes.h"
#include "path_tracing/visible_lights.h"
#include "path_tracing/visible_projectors.h"
#include "ui/dialogs/parameters/path_tracing_3d.h"
#include "ui/dialogs/parameters/path_tracing_nd.h"
#include "ui/painter_window/painter_window.h"
#include "ui/support/support.h"

namespace
{
std::unique_ptr<const Projector<3, double>> create_projector(const PaintingInformation3d& info, int paint_width, int paint_height)
{
        vec3 camera_position = info.view_center - info.camera_direction * 2.0 * info.object_size;
        vec3 camera_right = cross(info.camera_direction, info.camera_up);

        std::array<Vector<3, double>, 2> screen_axes{{camera_right, info.camera_up}};
        std::array<int, 2> screen_size{{paint_width, paint_height}};

        double units_per_pixel = info.view_width / paint_width;

        return std::make_unique<const VisibleParallelProjector<3, double>>(camera_position, info.camera_direction, screen_axes,
                                                                           units_per_pixel, screen_size);
}

std::unique_ptr<const LightSource<3, double>> create_light_source(const PaintingInformation3d& info)
{
        vec3 light_position = info.object_position - info.light_direction * info.object_size * 1000.0;

        return std::make_unique<const VisibleConstantLight<3, double>>(light_position, Color(1));
}
}

void painting(const std::shared_ptr<const Mesh<3, double>>& mesh, const PaintingInformation3d& info_3d,
              const PaintingInformationAll& info_all)
{
        int width, height, thread_count, samples_per_pixel;
        bool flat_facets, cornell_box;

        if (!PathTracingParametersFor3d(info_all.parent_window)
                     .show(hardware_concurrency(), info_3d.paint_width, info_3d.paint_height, info_3d.max_screen_size,
                           info_all.default_samples_per_pixel, info_all.max_samples_per_pixel, &thread_count, &width, &height,
                           &samples_per_pixel, &flat_facets, &cornell_box))
        {
                return;
        }

        std::string title;
        std::unique_ptr<const PaintObjects<3, double>> scene;

        if (!cornell_box)
        {
                title = info_all.window_title + " (" + info_all.model_name + ")";

                scene = single_object_scene(info_all.background_color, info_all.default_color, info_all.diffuse,
                                            create_projector(info_3d, width, height), create_light_source(info_3d), mesh);
        }
        else
        {
                title = info_all.window_title + " (" + info_all.model_name + " in Cornell Box)";

                scene = cornell_box_scene(width, height, mesh, info_3d.object_size, info_all.default_color, info_all.diffuse,
                                          info_3d.camera_direction, info_3d.camera_up);
        }

        create_and_show_delete_on_close_window<PainterWindow<3, double>>(title, thread_count, samples_per_pixel, !flat_facets,
                                                                         std::move(scene));
}

template <size_t N, typename T>
void painting(const std::shared_ptr<const Mesh<N, T>>& mesh, const PaintingInformationNd& info_nd,
              const PaintingInformationAll& info_all)
{
        static_assert(N >= 4);

        int min_size, max_size, thread_count, samples_per_pixel;
        bool flat_facets;

        if (!PathTracingParametersForNd(info_all.parent_window)
                     .show(N, hardware_concurrency(), info_nd.default_screen_size, info_nd.minimum_screen_size,
                           info_nd.maximum_screen_size, info_all.default_samples_per_pixel, info_all.max_samples_per_pixel,
                           &thread_count, &min_size, &max_size, &samples_per_pixel, &flat_facets))
        {
                return;
        }

        std::string title = info_all.window_title + " (" + info_all.model_name + ")";

        std::unique_ptr<const PaintObjects<N, T>> scene = single_object_scene(info_all.background_color, info_all.default_color,
                                                                              info_all.diffuse, min_size, max_size, mesh);

        create_and_show_delete_on_close_window<PainterWindow<N, T>>(title, thread_count, samples_per_pixel, !flat_facets,
                                                                    std::move(scene));
}

template void painting(const std::shared_ptr<const Mesh<4, float>>& mesh, const PaintingInformationNd& info_nd,
                       const PaintingInformationAll& info_all);
template void painting(const std::shared_ptr<const Mesh<5, float>>& mesh, const PaintingInformationNd& info_nd,
                       const PaintingInformationAll& info_all);
template void painting(const std::shared_ptr<const Mesh<6, float>>& mesh, const PaintingInformationNd& info_nd,
                       const PaintingInformationAll& info_all);

template void painting(const std::shared_ptr<const Mesh<4, double>>& mesh, const PaintingInformationNd& info_nd,
                       const PaintingInformationAll& info_all);
template void painting(const std::shared_ptr<const Mesh<5, double>>& mesh, const PaintingInformationNd& info_nd,
                       const PaintingInformationAll& info_all);
template void painting(const std::shared_ptr<const Mesh<6, double>>& mesh, const PaintingInformationNd& info_nd,
                       const PaintingInformationAll& info_all);
