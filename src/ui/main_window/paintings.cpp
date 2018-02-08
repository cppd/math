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

#include "path_tracing/lights/light_source.h"
#include "path_tracing/projectors/projector.h"
#include "path_tracing/scenes.h"
#include "ui/painter_window/painter_window.h"
#include "ui/support/support.h"

namespace
{
std::unique_ptr<const Projector> create_projector(const IShow& show, int paint_width, int paint_height)
{
        vec3 camera_up, camera_direction, view_center;
        double view_width;

        show.get_camera_information(&camera_up, &camera_direction, &view_center, &view_width);

        vec3 camera_position = view_center - camera_direction * 2.0 * show.get_object_size();

        return std::make_unique<const ParallelProjector>(camera_position, camera_direction, camera_up, view_width, paint_width,
                                                         paint_height);
}

std::unique_ptr<const LightSource> create_light_source(const IShow& show)
{
        vec3 light_position = show.get_object_position() - show.get_light_direction() * show.get_object_size() * 1000.0;

        return std::make_unique<const ConstantLight>(light_position, vec3(1, 1, 1));
}

bool parameters(PathTracingParameters& parameters_window, const IShow& show, int default_samples_per_pixel,
                int max_samples_per_pixel, int* paint_width, int* paint_height, int* thread_count, int* samples_per_pixel)
{
        show.get_paint_width_height(paint_width, paint_height);

        double size_coef;

        if (parameters_window.show(get_hardware_concurrency(), *paint_width, *paint_height, default_samples_per_pixel,
                                   max_samples_per_pixel, thread_count, &size_coef, samples_per_pixel))
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

void painting(PathTracingParameters&& parameters_window, const IShow& show, const std::shared_ptr<const Mesh>& mesh,
              const std::string& window_title, const std::string& model_name, int default_samples_per_pixel,
              int max_samples_per_pixel, const vec3& background_color, const vec3& default_color, double diffuse)
{
        int paint_width, paint_height, thread_count, samples_per_pixel;

        if (!parameters(parameters_window, show, default_samples_per_pixel, max_samples_per_pixel, &paint_width, &paint_height,
                        &thread_count, &samples_per_pixel))
        {
                return;
        }

        if ((true))
        {
                std::string title = window_title + " (" + model_name + ")";

                create_and_show_delete_on_close_window<PainterWindow>(
                        title, thread_count, samples_per_pixel,
                        one_object_scene(background_color, default_color, diffuse,
                                         create_projector(show, paint_width, paint_height), create_light_source(show), mesh));
        }
        else
        {
                vec3 camera_up, camera_direction, view_center;
                double view_width;

                show.get_camera_information(&camera_up, &camera_direction, &view_center, &view_width);

                std::string title = window_title + " (" + model_name + " in Cornell Box)";

                create_and_show_delete_on_close_window<PainterWindow>(title, thread_count, samples_per_pixel,
                                                                      cornell_box(paint_width, paint_height, mesh,
                                                                                  show.get_object_size(), default_color, diffuse,
                                                                                  camera_direction, camera_up));
        }
}
