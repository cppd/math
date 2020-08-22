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

#include "options.h"
#include "painter_scene.h"

#include <src/com/log.h>
#include <src/gui/dialogs/painter_3d.h>
#include <src/gui/dialogs/painter_nd.h>
#include <src/gui/painter_window/painter_window.h>
#include <src/settings/painter.h>

namespace process
{
namespace
{
// Количество лучей на один пиксель на одно измерение в одном проходе.
// Тогда для количества измерений D в пространстве экрана количество
// лучей равно std::pow(эта_величина, D).
constexpr int PAINTER_DEFAULT_SAMPLES_PER_DIMENSION = 5;
constexpr int PAINTER_MAXIMUM_SAMPLES_PER_DIMENSION = 10;

// Максимальный размер экрана в пикселях для 3 измерений
constexpr int PAINTER_MAXIMUM_SCREEN_SIZE_3D = 10000;

// Размеры экрана в пикселях для 4 и более измерений
constexpr int PAINTER_DEFAULT_SCREEN_SIZE_ND = 500;
constexpr int PAINTER_MINIMUM_SCREEN_SIZE_ND = 50;
constexpr int PAINTER_MAXIMUM_SCREEN_SIZE_ND = 5000;

template <size_t N>
std::function<void(ProgressRatioList*)> action_painter_function(
        const std::shared_ptr<const mesh::MeshObject<N>>& mesh_object,
        const view::info::Camera& camera,
        const std::string& title,
        const Color& background_color,
        const Color::DataType& lighting_intensity)
{
        ASSERT(mesh_object);

        {
                mesh::Reading reading(*mesh_object);
                if (reading.mesh().facets.empty())
                {
                        MESSAGE_WARNING("No object to paint");
                        return nullptr;
                }
        }

        const unsigned default_samples = power<N - 1>(static_cast<unsigned>(PAINTER_DEFAULT_SAMPLES_PER_DIMENSION));
        const unsigned max_samples = power<N - 1>(static_cast<unsigned>(PAINTER_MAXIMUM_SAMPLES_PER_DIMENSION));

        int thread_count;
        int samples_per_pixel;
        bool flat_facets;

        using T = settings::painter::FloatingPoint;

        PainterSceneInfo<N, T> scene_info;

        if constexpr (N == 3)
        {
                scene_info.camera_up = to_vector<T>(camera.up);
                scene_info.camera_direction = to_vector<T>(camera.forward);
                scene_info.light_direction = to_vector<T>(camera.lighting);
                scene_info.view_center = to_vector<T>(camera.view_center);
                scene_info.view_width = camera.view_width;
                scene_info.scene_size = SCENE_SIZE;

                if (!gui::dialog::painter_parameters_for_3d(
                            hardware_concurrency(), camera.width, camera.height, PAINTER_MAXIMUM_SCREEN_SIZE_3D,
                            default_samples, max_samples, &thread_count, &scene_info.width, &scene_info.height,
                            &samples_per_pixel, &flat_facets, &scene_info.cornell_box))
                {
                        return nullptr;
                }
        }
        else
        {
                if (!gui::dialog::painter_parameters_for_nd(
                            N, hardware_concurrency(), PAINTER_DEFAULT_SCREEN_SIZE_ND, PAINTER_MINIMUM_SCREEN_SIZE_ND,
                            PAINTER_MAXIMUM_SCREEN_SIZE_ND, default_samples, max_samples, &thread_count,
                            &scene_info.min_screen_size, &scene_info.max_screen_size, &samples_per_pixel, &flat_facets))
                {
                        return nullptr;
                }
        }

        return [=](ProgressRatioList* progress_list) {
                std::shared_ptr<const painter::MeshObject<N, T>> painter_mesh_object;
                {
                        mesh::Reading reading(*mesh_object);
                        if (reading.mesh().facets.empty())
                        {
                                MESSAGE_WARNING("No object to paint");
                                return;
                        }
                        ProgressRatio progress(progress_list);
                        painter_mesh_object = std::make_shared<painter::MeshObject<N, T>>(
                                reading.mesh(), reading.color(), reading.diffuse(), to_matrix<T>(reading.matrix()),
                                &progress);
                }

                if (!painter_mesh_object)
                {
                        MESSAGE_WARNING("No object to paint");
                        return;
                }

                std::string window_title = title + " (" + mesh_object->name() + ")";

                std::unique_ptr<const painter::PaintObjects<N, T>> scene =
                        create_painter_scene(painter_mesh_object, scene_info, background_color, lighting_intensity);

                gui::create_painter_window(
                        window_title, thread_count, samples_per_pixel, !flat_facets, std::move(scene));
        };
}
}

std::function<void(ProgressRatioList*)> action_painter(
        const storage::MeshObjectConst& object,
        const view::info::Camera& camera,
        const std::string& title,
        const Color& background_color,
        const Color::DataType& lighting_intensity)
{
        return std::visit(
                [&]<size_t N>(const std::shared_ptr<const mesh::MeshObject<N>>& mesh_object) {
                        return action_painter_function(
                                mesh_object, camera, title, background_color, lighting_intensity);
                },
                object);
}
}
