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
#include "ui/dialogs/path_tracing_parameters_3d.h"
#include "ui/dialogs/path_tracing_parameters_nd.h"
#include "ui/painter_window/painter_window.h"
#include "ui/support/support.h"

namespace
{
std::unique_ptr<const Projector<3, double>> create_projector(const IShow& show, int paint_width, int paint_height)
{
        vec3 camera_up, camera_direction, view_center;
        double view_width;

        show.camera_information(&camera_up, &camera_direction, &view_center, &view_width);

        vec3 camera_position = view_center - camera_direction * 2.0 * show.object_size();
        vec3 camera_right = cross(camera_direction, camera_up);

        std::array<Vector<3, double>, 2> screen_axes{{camera_right, camera_up}};
        std::array<int, 2> screen_size{{paint_width, paint_height}};

        double units_per_pixel = view_width / paint_width;

        return std::make_unique<const VisibleParallelProjector<3, double>>(camera_position, camera_direction, screen_axes,
                                                                           units_per_pixel, screen_size);
}

std::unique_ptr<const LightSource<3, double>> create_light_source(const IShow& show)
{
        vec3 light_position = show.object_position() - show.light_direction() * show.object_size() * 1000.0;

        return std::make_unique<const VisibleConstantLight<3, double>>(light_position, Color(1));
}

bool parameters(QWidget* parent_window, const IShow& show, int default_samples_per_pixel, int max_samples_per_pixel,
                int* paint_width, int* paint_height, int* thread_count, int* samples_per_pixel)
{
        show.paint_width_height(paint_width, paint_height);

        double size_coef;

        if (PathTracingParametersFor3d(parent_window)
                    .show(hardware_concurrency(), *paint_width, *paint_height, default_samples_per_pixel, max_samples_per_pixel,
                          thread_count, &size_coef, samples_per_pixel))
        {
                *paint_width = std::lround(*paint_width * size_coef);
                *paint_height = std::lround(*paint_height * size_coef);

                return true;
        }
        else
        {
                return false;
        }
}
}

void painting(QWidget* parent_window, const IShow& show, const std::shared_ptr<const Mesh<3, double>>& mesh,
              const std::string& window_title, const std::string& model_name, int default_samples_per_pixel,
              int max_samples_per_pixel, const Color& background_color, const Color& default_color, double diffuse)
{
        int paint_width, paint_height, thread_count, samples_per_pixel;

        if (!parameters(parent_window, show, default_samples_per_pixel, max_samples_per_pixel, &paint_width, &paint_height,
                        &thread_count, &samples_per_pixel))
        {
                return;
        }

        if ((true))
        {
                std::string title = window_title + " (" + model_name + ")";

                create_and_show_delete_on_close_window<PainterWindow<3, double>>(
                        title, thread_count, samples_per_pixel,
                        one_object_scene(background_color, default_color, diffuse,
                                         create_projector(show, paint_width, paint_height), create_light_source(show), mesh));
        }
        else
        {
                vec3 camera_up, camera_direction, view_center;
                double view_width;

                show.camera_information(&camera_up, &camera_direction, &view_center, &view_width);

                std::string title = window_title + " (" + model_name + " in Cornell Box)";

                create_and_show_delete_on_close_window<PainterWindow<3, double>>(
                        title, thread_count, samples_per_pixel,
                        cornell_box(paint_width, paint_height, mesh, show.object_size(), default_color, diffuse, camera_direction,
                                    camera_up));
        }
}

template <size_t N, typename T>
void painting(QWidget* parent_window, const std::shared_ptr<const Mesh<N, T>>& mesh, const std::string& window_title,
              const std::string& model_name, int default_screen_size, int min_screen_size, int max_screen_size,
              int default_samples_per_pixel, int max_samples_per_pixel, const Color& background_color, const Color& default_color,
              T diffuse)
{
        static_assert(N >= 4);

        int thread_count, min_size, max_size, samples_per_pixel;

        if (!PathTracingParametersForNd(parent_window)
                     .show(to_string(N) + "-space", hardware_concurrency(), default_screen_size, min_screen_size, max_screen_size,
                           default_samples_per_pixel, max_samples_per_pixel, &thread_count, &min_size, &max_size,
                           &samples_per_pixel))
        {
                return;
        }

        std::string title = window_title + " (" + model_name + ")";

        create_and_show_delete_on_close_window<PainterWindow<N, T>>(
                window_title, thread_count, samples_per_pixel,
                one_object_scene(background_color, default_color, diffuse, min_size, max_size, mesh));
}

template void painting(QWidget* parent_window, const std::shared_ptr<const Mesh<4, double>>& mesh,
                       const std::string& window_title, const std::string& model_name, int default_screen_size,
                       int min_screen_size, int max_screen_size, int default_samples_per_pixel, int max_samples_per_pixel,
                       const Color& background_color, const Color& default_color, double diffuse);
template void painting(QWidget* parent_window, const std::shared_ptr<const Mesh<5, double>>& mesh,
                       const std::string& window_title, const std::string& model_name, int default_screen_size,
                       int min_screen_size, int max_screen_size, int default_samples_per_pixel, int max_samples_per_pixel,
                       const Color& background_color, const Color& default_color, double diffuse);
template void painting(QWidget* parent_window, const std::shared_ptr<const Mesh<6, double>>& mesh,
                       const std::string& window_title, const std::string& model_name, int default_screen_size,
                       int min_screen_size, int max_screen_size, int default_samples_per_pixel, int max_samples_per_pixel,
                       const Color& background_color, const Color& default_color, double diffuse);
template void painting(QWidget* parent_window, const std::shared_ptr<const Mesh<4, float>>& mesh, const std::string& window_title,
                       const std::string& model_name, int default_screen_size, int min_screen_size, int max_screen_size,
                       int default_samples_per_pixel, int max_samples_per_pixel, const Color& background_color,
                       const Color& default_color, float diffuse);
template void painting(QWidget* parent_window, const std::shared_ptr<const Mesh<5, float>>& mesh, const std::string& window_title,
                       const std::string& model_name, int default_screen_size, int min_screen_size, int max_screen_size,
                       int default_samples_per_pixel, int max_samples_per_pixel, const Color& background_color,
                       const Color& default_color, float diffuse);
template void painting(QWidget* parent_window, const std::shared_ptr<const Mesh<6, float>>& mesh, const std::string& window_title,
                       const std::string& model_name, int default_screen_size, int min_screen_size, int max_screen_size,
                       int default_samples_per_pixel, int max_samples_per_pixel, const Color& background_color,
                       const Color& default_color, float diffuse);
