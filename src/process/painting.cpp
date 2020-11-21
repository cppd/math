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
// Количество лучей на один пиксель в одном проходе
constexpr int PAINTER_DEFAULT_SAMPLE_COUNT = 25;
template <size_t N>
constexpr int PAINTER_MAXIMUM_SAMPLE_COUNT = power<N - 1>(10u);

// Максимальный размер экрана в пикселях для 3 измерений
constexpr int PAINTER_MAXIMUM_SCREEN_SIZE_3D = 10000;

// Размеры экрана в пикселях для 4 и более измерений
constexpr int PAINTER_DEFAULT_SCREEN_SIZE_ND = 500;
constexpr int PAINTER_MINIMUM_SCREEN_SIZE_ND = 50;
constexpr int PAINTER_MAXIMUM_SCREEN_SIZE_ND = 5000;

template <size_t N>
std::function<void(ProgressRatioList*)> action_painter_function(
        const std::vector<std::shared_ptr<const mesh::MeshObject<N>>>& mesh_objects,
        const view::info::Camera& camera,
        const Color& background_color,
        const Color::DataType& lighting_intensity)
{
        ASSERT(!mesh_objects.empty());
        long long facet_count = 0;
        for (const std::shared_ptr<const mesh::MeshObject<N>>& object : mesh_objects)
        {
                facet_count += mesh::Reading<N>(*object).mesh().facets.size();
        }
        if (facet_count == 0)
        {
                MESSAGE_WARNING("No objects to paint");
                return nullptr;
        }

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

                if (!gui::dialog::painter_parameters_for_3d(
                            hardware_concurrency(), camera.width, camera.height, PAINTER_MAXIMUM_SCREEN_SIZE_3D,
                            PAINTER_DEFAULT_SAMPLE_COUNT, PAINTER_MAXIMUM_SAMPLE_COUNT<N>, &thread_count,
                            &scene_info.width, &scene_info.height, &samples_per_pixel, &flat_facets,
                            &scene_info.cornell_box))
                {
                        return nullptr;
                }
        }
        else
        {
                if (!gui::dialog::painter_parameters_for_nd(
                            N, hardware_concurrency(), PAINTER_DEFAULT_SCREEN_SIZE_ND, PAINTER_MINIMUM_SCREEN_SIZE_ND,
                            PAINTER_MAXIMUM_SCREEN_SIZE_ND, PAINTER_DEFAULT_SAMPLE_COUNT,
                            PAINTER_MAXIMUM_SAMPLE_COUNT<N>, &thread_count, &scene_info.min_screen_size,
                            &scene_info.max_screen_size, &samples_per_pixel, &flat_facets, &scene_info.cornell_box))
                {
                        return nullptr;
                }
        }

        return [=](ProgressRatioList* progress_list)
        {
                std::unique_ptr<const painter::Shape<N, T>> shape;
                {
                        std::vector<const mesh::MeshObject<N>*> meshes;
                        meshes.reserve(mesh_objects.size());
                        for (const std::shared_ptr<const mesh::MeshObject<N>>& mesh_object : mesh_objects)
                        {
                                meshes.push_back(mesh_object.get());
                        }
                        ProgressRatio progress(progress_list);
                        shape = std::make_unique<painter::shapes::Mesh<N, T>>(meshes, &progress);
                }

                if (!shape)
                {
                        MESSAGE_WARNING("No object to paint");
                        return;
                }

                std::string name = mesh_objects.size() != 1 ? "" : mesh_objects[0]->name();

                std::unique_ptr<const painter::Scene<N, T>> scene =
                        create_painter_scene(std::move(shape), scene_info, background_color, lighting_intensity);

                gui::painter_window::create_painter_window(
                        name, thread_count, samples_per_pixel, !flat_facets, std::move(scene));
        };
}
}

std::function<void(ProgressRatioList*)> action_painter(
        const std::vector<storage::MeshObjectConst>& objects,
        const view::info::Camera& camera,
        const Color& background_color,
        const Color::DataType& lighting_intensity)
{
        std::set<size_t> dimensions;
        std::vector<storage::MeshObjectConst> visible_objects;
        for (const storage::MeshObjectConst& storage_object : objects)
        {
                std::visit(
                        [&]<size_t N>(const std::shared_ptr<const mesh::MeshObject<N>>& object)
                        {
                                if (object->visible())
                                {
                                        dimensions.insert(N);
                                        visible_objects.push_back(object);
                                }
                        },
                        storage_object);
        }
        if (visible_objects.empty())
        {
                MESSAGE_WARNING("No objects to paint");
                return nullptr;
        }
        if (dimensions.size() > 1)
        {
                MESSAGE_WARNING("Painting different dimensions is not supported: " + to_string(dimensions) + ".");
                return nullptr;
        }

        return apply_for_dimension(
                *dimensions.cbegin(),
                [&]<size_t N>(const Dimension<N>&)
                {
                        std::vector<std::shared_ptr<const mesh::MeshObject<N>>> meshes;
                        for (storage::MeshObjectConst& visible_object : visible_objects)
                        {
                                std::visit(
                                        [&]<size_t M>(std::shared_ptr<const mesh::MeshObject<M>>&& object)
                                        {
                                                if constexpr (N == M)
                                                {
                                                        meshes.push_back(std::move(object));
                                                }
                                        },
                                        std::move(visible_object));
                        }
                        return action_painter_function(meshes, camera, background_color, lighting_intensity);
                });
}
}
